
module tdoa_core #(
    parameter CH_NUM = 2,
    parameter TIME_W = 16,
    parameter WIDTH_W = 16,

    parameter MIN_WIDTH    = 50,
    parameter WIDTH_TOL    = 20,
    parameter EVENT_WINDOW = 2000
)(
    input clk,
    input rst,
    input [CH_NUM-1:0] sensor_in,

    output reg result_valid,
    //output reg [TIME_W-1:0] tdoa [CH_NUM-1:0]
    output reg [CH_NUM*TIME_W-1:0] tdoa_i,
    output [31:0] stats_out
);

reg [TIME_W-1:0] global_time;
reg [TIME_W-1:0] tdoa [CH_NUM-1:0];
reg [15:0] pulse_count;
reg [15:0] missed_pulse_count;

assign stats_out = {pulse_count,missed_pulse_count};

always @(posedge clk) begin
    if (rst) begin
        global_time <= 0;
        pulse_count <= 0;
        missed_pulse_count <= 0;
    end else
        global_time <= global_time + 1;
end

//  pulse capture

reg sensor_d [CH_NUM-1:0];

reg [TIME_W-1:0] rise_time [CH_NUM-1:0];
reg [WIDTH_W-1:0] pulse_width [CH_NUM-1:0];
reg [CH_NUM-1:0] hit_mask;
reg [CH_NUM-1:0] first_hit;

reg event_valid [CH_NUM-1:0];
reg [TIME_W-1:0] event_time [CH_NUM-1:0];
reg [WIDTH_W-1:0] event_width [CH_NUM-1:0];


integer i;

always @(posedge clk) begin
    for (i = 0; i < CH_NUM; i = i + 1) begin
        sensor_d[i] <= sensor_in[i];
    end
end

wire [CH_NUM-1:0] rise;
wire [CH_NUM-1:0] fall;

generate
genvar g;
for (g = 0; g < CH_NUM; g = g + 1) begin
    assign rise[g] =  sensor_in[g] & ~sensor_d[g];
    assign fall[g] = ~sensor_in[g] &  sensor_d[g];
end
endgenerate

always @(posedge clk) begin
    if (rst | (state == S_OUT)) begin
        for (i = 0; i < CH_NUM; i = i + 1) begin
            event_valid[i] <= 0;
        end
    end else begin
            // record rise time
        for (i = 0; i < CH_NUM; i = i + 1) begin
            if (rise[i] && !event_valid[i]) begin
                rise_time[i] <= global_time;
            end

            if (fall[i] && !event_valid[i]) begin

                if ((global_time - rise_time[i]) >= MIN_WIDTH) begin
                    event_time[i]  <= rise_time[i];
                    event_width[i] <= global_time - rise_time[i];
                    event_valid[i] <= 1;
                end
            end
        end
    end
end

localparam S_IDLE    = 0;
localparam S_COLLECT = 1;
localparam S_CALC    = 2;
localparam S_OUT     = 3;

reg [1:0] state;

reg [TIME_W-1:0] time_buf [CH_NUM-1:0];
reg [WIDTH_W-1:0] width_ref;
reg [TIME_W-1:0] t_ref;
reg [TIME_W-1:0] window_end;

integer k;

always @(posedge clk) begin
    if (rst) begin
        state <= S_IDLE;
        result_valid <= 0;
        hit_mask <= 0;
        first_hit <=0;

        for (k = 0; k < CH_NUM; k = k + 1)
            tdoa[k] <= {TIME_W{1'b1}};

    end else begin
        result_valid <= 0;

        case (state)

        S_IDLE: begin

            for (k = 0; k < CH_NUM; k = k + 1) begin
                if (event_valid[k]) begin
                    width_ref   <= event_width[k];
                    t_ref       <= event_time[k];
                    time_buf[k] <= event_time[k];
                    hit_mask[k] <= 1;
                    first_hit[k] <= 1;

                    window_end <= global_time + EVENT_WINDOW;
                    state <= S_COLLECT;
                end
            end
        end

        S_COLLECT: begin
            for (k = 0; k < CH_NUM; k = k + 1) begin
                if (event_valid[k] && !hit_mask[k]) begin
                    if (
                        (event_width[k] > width_ref - WIDTH_TOL) &&
                        (event_width[k] < width_ref + WIDTH_TOL)
                    ) begin
                        time_buf[k] <= event_time[k];
                        hit_mask[k] <= 1;
                    end
                end
            end

            if ( (hit_mask == {CH_NUM{1'b1}}) || (global_time == window_end) )
                state <= S_CALC;
        end

        S_CALC: begin
            if (hit_mask == first_hit) begin
                state <= S_IDLE;
                hit_mask <= 0;
                first_hit <= 0;
                missed_pulse_count <= missed_pulse_count + 1;
            end else begin
                pulse_count <= pulse_count + 1;
                for (k = 0; k < CH_NUM; k = k + 1)
                    if (hit_mask[k])
                        tdoa[k] <= time_buf[k] - t_ref;
                    else
                        tdoa[k] <= {TIME_W{1'b1}};
                state <= S_OUT;
            end
        end

        S_OUT: begin
            result_valid <= 1;
            state <= S_IDLE;
            hit_mask <= 0;
        end

        endcase
    end
end

genvar gi;
generate
for (gi = 0; gi < CH_NUM; gi = gi + 1) begin
    always @(*) begin
        tdoa_i[gi*TIME_W +: TIME_W] = tdoa[gi];
    end
end
endgenerate

endmodule

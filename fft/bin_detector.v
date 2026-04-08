`timescale 1ns / 1ps
module bin_detector #(
    parameter TARGET_BIN = 72,
    parameter THRESHOLD  = 100000,
    parameter PIPE_DELAY = 2
)(
    input  wire        clk,
    input  wire        reset_n,
    input  wire        clear_flag,
    input  wire        s_axis_tvalid_raw,
    input  wire        s_axis_tlast_raw,
    input  wire [31:0] s_axis_tdata_delayed,
    output reg  [31:0] bin_magnitude,
    output reg         detection_flag
);

    reg [PIPE_DELAY-1:0] v_delay;
    reg [PIPE_DELAY-1:0] l_delay;

    wire v_aligned = v_delay[PIPE_DELAY-1];
    wire l_aligned = l_delay[PIPE_DELAY-1];

    reg [10:0] bin_count_raw;
    reg [10:0] bin_count_d1;
    reg [10:0] bin_count_d2;

    wire [10:0] bin_count_aligned = bin_count_d2;

    always @(posedge clk) begin
        if (!reset_n) begin
            v_delay        <= 0;
            l_delay        <= 0;
            bin_count_raw  <= 0;
            bin_count_d1   <= 0;
            bin_count_d2   <= 0;
            bin_magnitude  <= 0;
            detection_flag <= 0;

        end else begin

            v_delay <= {v_delay[PIPE_DELAY-2:0], s_axis_tvalid_raw};
            l_delay <= {l_delay[PIPE_DELAY-2:0], s_axis_tlast_raw};

            if (s_axis_tvalid_raw) begin
                if (s_axis_tlast_raw)
                    bin_count_raw <= 0;
                else
                    bin_count_raw <= bin_count_raw + 1;

                bin_count_d1 <= bin_count_raw;
            end

            if (v_delay[0]) begin
                bin_count_d2 <= bin_count_d1;
            end

            if (clear_flag) begin
                detection_flag <= 1'b0;

            end else if (v_aligned) begin
                if (bin_count_aligned == TARGET_BIN) begin
                    bin_magnitude <= s_axis_tdata_delayed;
                    if (s_axis_tdata_delayed > THRESHOLD)
                        detection_flag <= 1'b1;
                end

                if (l_aligned)
                    bin_count_raw <= 0;
            end
        end
    end
endmodule

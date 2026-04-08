`timescale 1ns / 1ps

module TD_axi_tb;

parameter CH_NUM = 10;
parameter TIME_W = 16;
parameter WIDTH_W = 16;

parameter MIN_WIDTH    = 6;
parameter WIDTH_TOL    = 40;
parameter EVENT_WINDOW = 80;

reg clk;
reg rst;
reg [CH_NUM*16-1:0] sensor_bus;

logic [TIME_W-1:0] tdoa [CH_NUM-1:0];
reg [CH_NUM*TIME_W-1:0] tdoa_i;

TDOA_module_v1_0_S00_AXI #(
    .CH_NUM(CH_NUM),
    .TIME_W(TIME_W),
    .WIDTH_W(WIDTH_W),
    .MIN_WIDTH(MIN_WIDTH),
    .WIDTH_TOL(WIDTH_TOL),
    .EVENT_WINDOW(EVENT_WINDOW)
) dut (
    .S_AXI_ACLK(clk),
    .S_AXI_ARESETN(rst),
    .sensor_bus(sensor_bus)
);

//////////////////////////////////////////////////
// clock
//////////////////////////////////////////////////

initial begin
    clk = 0;
    forever #5 clk = ~clk; // 100 MHz
end

initial begin
    rst = 0;
    sensor_bus = 0;
    #100;
    rst = 1;
end

//////////////////////////////////////////////////
// pulse task
//////////////////////////////////////////////////

/*
task automatic send_pulse;
    input int ch;
    input int delay_cycles;
    input int width_cycles;
    int i;
begin
    repeat(delay_cycles) @(negedge clk);

    sensor_in[ch] = 1;
    repeat(width_cycles) @(negedge clk);
    sensor_in[ch] = 0;
end
endtask
*/

//////////////////////////////////////////////////
// stimulus
//////////////////////////////////////////////////

initial begin
    wait(rst);

    repeat(10) @(posedge clk);
    @(negedge clk);

    for (int i=0; i<10; i=i+1) begin
        sensor_bus <= (sensor_bus << 16) + 16'h00FF;
        repeat(2) @(negedge clk);
    end

    for (int i=0; i<10; i=i+1) begin
        sensor_bus <= sensor_bus << 16;
        repeat(2) @(negedge clk);
    end

    repeat(40) @(posedge clk);

    $finish;
end

endmodule

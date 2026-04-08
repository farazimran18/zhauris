`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Module Name: tutorial
//////////////////////////////////////////////////////////////////////////////////


module pulse_detector(
    input [15:0] signal,
    input clk,
    input rst,
    output pulse
    );

    localparam average_shift = 8;

    reg [(average_shift+16-1):0] average_r;
    reg [15:0] peak;
    reg  pulse_r;
    wire [15:0]average;

    assign average = average_r[(average_shift+16-1):average_shift];

    always @(posedge clk) begin
        if (rst) begin
            average_r <= 0;
            peak      <= 0;
            pulse_r   <= 0;
        end else begin
            //moving average
            average_r <= average_r - average + signal;
            //peak capture
            peak <= (signal > peak)? signal : peak - (peak >> 4);
            //pulse detection
            pulse_r <= (peak > (average << 2));
        end
    end

    assign pulse = pulse_r;
endmodule

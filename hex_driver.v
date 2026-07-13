module hex_driver (
    input  wire [31:0] rpm,
    output wire  [6:0] HEX0,
    output wire  [6:0] HEX1,
    output wire  [6:0] HEX2,
    output wire  [6:0] HEX3,
    output wire  [6:0] HEX4,
    output wire  [6:0] HEX5
);

// Extract individual digits from RPM
wire [3:0] digit0 = (rpm / 1)     % 10;  // units
wire [3:0] digit1 = (rpm / 10)    % 10;  // tens
wire [3:0] digit2 = (rpm / 100)   % 10;  // hundreds
wire [3:0] digit3 = (rpm / 1000)  % 10;  // thousands
wire [3:0] digit4 = (rpm / 10000) % 10;  // ten-thousands
wire [3:0] digit5 = 4'd0;                // always blank

// 7-segment decoder function
// Segments: gfedcba (active LOW on DE1-SoC)
function [6:0] seg7;
    input [3:0] digit;
    case (digit)
        4'd0: seg7 = 7'b1000000;
        4'd1: seg7 = 7'b1111001;
        4'd2: seg7 = 7'b0100100;
        4'd3: seg7 = 7'b0110000;
        4'd4: seg7 = 7'b0011001;
        4'd5: seg7 = 7'b0010010;
        4'd6: seg7 = 7'b0000010;
        4'd7: seg7 = 7'b1111000;
        4'd8: seg7 = 7'b0000000;
        4'd9: seg7 = 7'b0010000;
        default: seg7 = 7'b1111111; // blank
    endcase
endfunction

assign HEX0 = seg7(digit0);
assign HEX1 = seg7(digit1);
assign HEX2 = seg7(digit2);
assign HEX3 = seg7(digit3);
assign HEX4 = seg7(digit4);
assign HEX5 = seg7(digit5);

endmodule
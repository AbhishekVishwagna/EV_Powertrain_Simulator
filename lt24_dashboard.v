module lt24_dashboard (
    input  wire        clk,
    input  wire        rst_n,
    input  wire [31:0] rpm,
    input  wire [31:0] soc,
    input  wire [1:0]  mode,
    input  wire        fault,

    output wire        LT24Wr_n,
    output wire        LT24Rd_n,
    output wire        LT24CS_n,
    output wire        LT24RS,
    output wire        LT24Reset_n,
    output wire [15:0] LT24Data,
    output wire        LT24LCDOn
);

localparam BLACK  = 16'h0000;
localparam WHITE  = 16'hFFFF;
localparam RED    = 16'hF800;
localparam GREEN  = 16'h07E0;
localparam BLUE   = 16'h001F;
localparam ORANGE = 16'hFD20;

localparam WIDTH  = 240;
localparam HEIGHT = 320;

reg [7:0]  x;
reg [8:0]  y;
reg [15:0] pixel_colour;
reg        pixel_write;
wire       pixel_ready;
wire       reset_app;

wire [15:0] mode_colour = (mode == 2'd0) ? GREEN  :
                          (mode == 2'd1) ? BLUE   :
                          (mode == 2'd2) ? ORANGE :
                                           RED;

// RPM bar height (0 to 200 pixels based on RPM 0-8000)
wire [8:0] rpm_bar = (rpm * 200) / 8000;

// SoC bar height (0 to 80 pixels based on SoC 0-100)
wire [6:0] soc_bar = (soc * 80) / 100;

LT24Display #(
    .CLOCK_FREQ(50000000),
    .WIDTH(240),
    .HEIGHT(320)
) display (
    .clock        (clk),
    .globalReset  (!rst_n),
    .resetApp     (reset_app),
    .xAddr        (x),
    .yAddr        (y),
    .pixelData    (pixel_colour),
    .pixelWrite   (pixel_write),
    .pixelReady   (pixel_ready),
    .pixelRawMode (1'b0),
    .cmdData      (8'b0),
    .cmdWrite     (1'b0),
    .cmdDone      (1'b0),
    .cmdReady     (),
    .LT24Wr_n     (LT24Wr_n),
    .LT24Rd_n     (LT24Rd_n),
    .LT24CS_n     (LT24CS_n),
    .LT24RS       (LT24RS),
    .LT24Reset_n  (LT24Reset_n),
    .LT24Data     (LT24Data),
    .LT24LCDOn    (LT24LCDOn)
);

// Continuous pixel drawing
always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        x <= 0;
        y <= 0;
        pixel_write <= 0;
        pixel_colour <= BLACK;
    end else if (!reset_app) begin
        if (pixel_ready) begin
            pixel_write <= 1;

            // Top bar — mode colour (y < 40)
            if (y < 40)
                pixel_colour <= mode_colour;

            // RPM bar area (y 40-240) — white bar grows with RPM
            else if (y >= 40 && y < 240) begin
                if (x < 20)
                    pixel_colour <= WHITE; // left border
                else if (x >= 220)
                    pixel_colour <= WHITE; // right border
                else if (y >= (240 - rpm_bar))
                    pixel_colour <= (mode == 2'd2) ? ORANGE : BLUE; // RPM fill
                else
                    pixel_colour <= BLACK;
            end

            // SoC bar area (y 240-320) — green bar shows battery
            else begin
                if (x < (soc_bar * 2))
                    pixel_colour <= (soc < 20) ? RED : GREEN; // battery fill
                else
                    pixel_colour <= BLACK;
            end

            // Advance pixel position
            if (x == WIDTH - 1) begin
                x <= 0;
                if (y == HEIGHT - 1)
                    y <= 0;
                else
                    y <= y + 1;
            end else begin
                x <= x + 1;
            end

        end else begin
            pixel_write <= 0;
        end
    end else begin
        // Hold in reset
        x <= 0;
        y <= 0;
        pixel_write <= 0;
    end
end

endmodule
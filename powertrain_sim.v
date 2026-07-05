module powertrain_sim (
    input  wire        clk,
    input  wire        rst_n,
    input  wire [31:0] throttle_in,
    output reg  [31:0] rpm_out,
    output reg  [31:0] soc_out,
    output reg         fault_out
);

parameter MAX_SOC = 32'd100000;
parameter MAX_RPM = 32'd8000;

reg [31:0] charge;
reg [31:0] rpm;
reg [31:0] clk_div;

initial begin
    charge    = MAX_SOC;
    rpm       = 32'd0;
    fault_out = 1'b0;
    clk_div   = 32'd0;
end

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        charge    <= MAX_SOC;
        rpm       <= 32'd0;
        fault_out <= 1'b0;
        clk_div   <= 32'd0;
    end else begin
        clk_div <= clk_div + 1;

        // Only update physics every 500,000 cycles (~10ms at 50MHz)
        if (clk_div == 32'd500000) begin
            clk_div <= 32'd0;

            // Battery model
            if (charge > 0 && throttle_in > 0)
                charge <= charge - throttle_in;
            else if (charge == 0)
                fault_out <= 1'b1;

            // Motor model
            if (throttle_in > 0 && charge > 0) begin
                if (rpm + throttle_in < MAX_RPM)
                    rpm <= rpm + throttle_in;
                else
                    rpm <= MAX_RPM;
            end else begin
                if (rpm > 10)
                    rpm <= rpm - 32'd10;
                else
                    rpm <= 32'd0;
            end

            soc_out <= (charge * 100) / MAX_SOC;
            rpm_out <= rpm;
        end
    end
end

endmodule
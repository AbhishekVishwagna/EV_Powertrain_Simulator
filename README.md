# HIL EV Powertrain Simulator

A Hardware in the Loop (HIL) Electric Vehicle Powertrain Simulator built on the Terasic DE1-SoC board, where the FPGA fabric acts as a real-time virtual powertrain and the ARM HPS running Linux acts as the Electronic Control Unit (ECU).

---

## Project Objectives

- Design and implement a real time virtual EV powertrain physics engine on FPGA fabric using Verilog HDL
- Establish reliable bidirectional communication between the FPGA and ARM HPS over the Lightweight AXI bridge
- Implement a PI cruise control algorithm on embedded Linux that reads motor RPM and adjusts throttle in real time
- Enable physical interaction via onboard push buttons to switch between drive modes (ECO / NORMAL / SPORT / BRAKE)
- Display real-time system state on the 7-segment displays, LED bar graph, and LT24 2.4" LCD
- Validate the system through a multi-phase drive cycle test covering acceleration, cruising, and braking
- Log all system data to CSV and visualise control system response using Python matplotlib

---

## Hardware and Software Requirements

### Hardware

| Component | Details |
|-----------|---------|
| Development Board | Terasic DE1-SoC |
| FPGA | Intel Cyclone V (5CSEMA5F31C6) |
| Processor | Dual-core ARM Cortex-A9 HPS @ 925MHz |
| Memory | 1GB DDR3 SDRAM |
| Storage | 64GB microSD card |
| OS | Linux (Terasic Console Image) |
| Display | Terasic LT24 2.4" LCD (connected via GPIO0) |
| Programming Interface | USB Blaster (JTAG) |
| Serial Interface | USB-UART (J4) |

### Software

| Tool | Purpose |
|------|---------|
| Quartus Prime Lite 22.1 | FPGA synthesis, place & route, programming |
| Platform Designer (Qsys) | HPS-FPGA AXI bridge and register configuration |
| WSL (Ubuntu) | HDL generation and ARM cross-compilation |
| arm-linux-gnueabihf-gcc | ARM cross-compiler for HPS software |
| Python / matplotlib | CSV data logging visualisation |
| GitHub Desktop | Version control |

---

## System Design and Methodology

### System Architecture

```
┌──────────────────────────┐      Lightweight AXI Bridge       ┌──────────────────────────┐
│      ARM HPS (Linux)     │  ◄──────────────────────────────► │      FPGA Fabric         │
│                          │                                   │                          │
│  • PI Cruise Controller  │   Throttle Command (0xFF200010)   │  • Motor Physics Engine  │
│  • Drive Cycle Test      │ ──────────────────────────────►   │  • RPM Accumulator       │
│  • Button Mode Selection │                                   │  • Battery SoC Model     │
│  • CSV Data Logger       │ ◄──────────────────────────────   │  • 50MHz Clock Divider   │
└──────────────────────────┘      RPM Feedback (0xFF200000)    └──────────────────────────┘
         │                                                               │
         │                                                     ┌─────────────────────┐
         │                                                     │  Output Peripherals  │
         │                                                     │  • HEX0-5: RPM      │
         └──── KEY0-KEY3 (Mode Select) ──────────────────────► │  • LEDR: Battery %  │
                                                               │  • LT24 LCD         │
                                                               └─────────────────────┘
```

### FPGA — Virtual Powertrain Physics Engine

The FPGA implements a simplified DC motor model in Verilog. A 50MHz clock divider creates a ~10ms physics timestep. On each timestep:

- **Motor model:** RPM increments proportionally to throttle input, capped at 8000 RPM. RPM decrements naturally when throttle is zero
- **Battery model:** State of Charge depletes proportionally to throttle demand. A fault flag is raised when SoC reaches zero
- Both RPM and SoC are written to output registers readable by the HPS

### HPS — ECU Control Software

The ARM processor runs a C program on Linux that:

- Reads RPM from the FPGA register every 10ms
- Detects push button presses and switches drive mode accordingly
- Runs a PI control loop: `throttle = Kp * error + Ki * integral` with anti-windup clamping
- Writes throttle command back to the FPGA register
- Logs all data to a CSV file on the SD card

### Drive Modes (Push Button Control)

| Button | Mode | Target RPM | Description |
|--------|------|------------|-------------|
| KEY0 | ECO | 2000 RPM | Low speed, energy saving |
| KEY1 | NORMAL | 5000 RPM | Standard cruise |
| KEY2 | SPORT | 7000 RPM | High performance |
| KEY3 | BRAKE | 0 RPM | Emergency stop |

---

## Demo Video

https://github.com/user-attachments/assets/18ba38cc-8177-4ecf-bac0-b77421fd8d5d

The demo shows:
- Live RPM displayed on HEX0-HEX5 seven-segment displays
- Battery SoC bar graph on LEDR[1:9] — LEDs turn off as battery drains
- Physical button presses switching between ECO, NORMAL, SPORT and BRAKE modes
- PI controller smoothly tracking each new RPM target

---

## File Structure

```
HIL-EV-Powertrain-Simulator/
│
├── ghrd_top.v              # Top-level Verilog — HPS, FPGA, peripherals
├── powertrain_sim.v        # Virtual powertrain physics engine (Verilog)
├── hex_driver.v            # 7-segment RPM display driver (Verilog)
├── lt24_dashboard.v        # LT24 LCD colour dashboard (Verilog)
├── soc_system.qsys         # Platform Designer system — AXI bridge and PIO registers
│
├── cruise_control.c        # PI cruise control + button mode selection (C, ARM Linux)
├── drive_cycle.c           # Drive cycle test — accelerate, cruise, brake (C, ARM Linux)
│
├── log.csv                 # Sample data log from cruise control run
└── README.md
```

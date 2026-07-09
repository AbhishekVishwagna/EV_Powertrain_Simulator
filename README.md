# HIL EV Powertrain Simulator

A Hardware-in-the-Loop (HIL) Electric Vehicle Powertrain Simulator built on the Terasic DE1-SoC board, where the FPGA fabric acts as a real-time virtual powertrain and the ARM HPS running Linux acts as the Electronic Control Unit (ECU).

---

## 1. Objectives

- Design and implement a real-time virtual EV powertrain physics engine on FPGA fabric using Verilog HDL
- Establish reliable bidirectional communication between the FPGA and ARM HPS over the Lightweight AXI bridge
- Implement a PI cruise control algorithm on embedded Linux that reads motor RPM and adjusts throttle in real time
- Validate the system through a multi-phase drive cycle test covering acceleration, cruising, and braking
- Log all system data to CSV and visualise control system response using Python matplotlib

---

## 2. Hardware / Software Requirements

### Hardware
| Component | Details |
|-----------|---------|
| Development Board | Terasic DE1-SoC |
| FPGA | Intel Cyclone V (5CSEMA5F31C6) |
| Processor | Dual-core ARM Cortex-A9 HPS @ 925MHz |
| Memory | 1GB DDR3 SDRAM |
| Storage | 64GB microSD card |
| OS | Linux (Terasic Console Image) |
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

## 3. System Design / Methodology

### System Architecture
```text
┌─────────────────────────┐       Lightweight AXI Bridge      ┌──────────────────────────┐ 
│      ARM HPS (Linux)    │  ◄──────────────────────────────► │      FPGA Fabric         │
│                         │                                   │                          │
│  • PI Cruise Controller │   Throttle Command (0xFF200010)   │  • Motor Physics Engine  │
│  • Drive Cycle Test     │ ──────────────────────────────►   │  • RPM Accumulator       │
│  • CSV Data Logger      │                                   │  • Battery SoC Model     │
│                         │ ◄──────────────────────────────   │  • 50MHz Clock Divider   │
└─────────────────────────┘      RPM Feedback (0xFF200000)    └──────────────────────────┘
```

### FPGA — Virtual Powertrain Physics Engine
The FPGA implements a simplified DC motor model in Verilog. A 50MHz clock divider creates a ~10ms physics timestep. On each timestep:
- **Motor model:** RPM increments proportionally to throttle input, capped at 8000 RPM. RPM decrements naturally when throttle is zero
- **Battery model:** State of Charge depletes proportionally to throttle demand. A fault flag is raised when SoC reaches zero
- Both RPM and SoC are written to output registers readable by the HPS

### HPS — ECU Control Software
The ARM processor runs a C program on Linux that:
- Opens `/dev/mem` and uses `mmap` to access the Lightweight AXI bridge at `0xFF200000`
- Reads RPM from the FPGA register every 10ms
- Runs a PI control loop: `throttle = Kp * error + Ki * integral` with anti-windup clamping
- Writes throttle command back to the FPGA register
- Logs all data to a CSV file on the SD card

### Register Map
| Register | Address | Direction | Description |
|----------|---------|-----------|-------------|
| hps_to_fpga | 0xFF200010 | HPS → FPGA | Throttle command (0-100) |
| fpga_to_hps | 0xFF200000 | FPGA → HPS | Motor RPM output |

### PI Controller Parameters
| Parameter | Value |
|-----------|-------|
| Kp | 0.05 |
| Ki | 0.0001 |
| Anti-windup limit | ±500 |
| Loop period | 10ms |

### Results

#### Cruise Control — Convergence to 5000 RPM
The PI controller converges to the 5000 RPM target with no overshoot, reducing throttle smoothly as RPM approaches the setpoint.

![Cruise Control Plot](cruise_control_plot.png)

#### Drive Cycle Test
Multi-phase driving profile: accelerate to 3000 RPM → cruise → accelerate to 6000 RPM → brake to rest.

![Drive Cycle Plot](drive_cycle_plot.png)

---

## 4. File Structure
```text
HIL-EV-Powertrain-Simulator/
│
├── ghrd_top.v                  # Top-level Verilog — integrates HPS and FPGA modules
├── powertrain_sim.v            # Virtual powertrain physics engine (Verilog HDL)
├── soc_system.qsys             # Platform Designer system — AXI bridge and PIO registers
│
├── cruise_control.c            # PI cruise control ECU software (C, ARM Linux)
├── drive_cycle.c               # Drive cycle test — acceleration, cruise, brake (C, ARM Linux)
│
├── cruise_control_plot.png     # PI controller RPM convergence plot
├── drive_cycle_plot.png        # Drive cycle response plot
│
└── README.md
```
---

## How to Run

### 1. Generate HDL (run from WSL — do not use Platform Designer GUI)
```bash
cd /path/to/project
/mnt/c/intelfpga_lite/22.1std/quartus/sopc_builder/bin/qsys-generate.exe soc_system.qsys --synthesis=VERILOG
```

### 2. Compile in Quartus Prime
Open `soc_system.qpf` and run Full Compilation. Ensure `soc_system/synthesis/soc_system.qip` is in the project file list.

### 3. Program the FPGA
Open Quartus Programmer → Auto Detect → select `5CSEMA5` → load `output_files/soc_system.sof` → Start.

### 4. Cross-compile ECU software
```bash
arm-linux-gnueabihf-gcc -o cruise_control cruise_control.c -static -lm
arm-linux-gnueabihf-gcc -o drive_cycle drive_cycle.c -static -lm
```

### 5. Deploy and run on board
```bash
mount /dev/mmcblk0p1 /media/card
chmod +x /media/card/cruise_control
/media/card/cruise_control
```

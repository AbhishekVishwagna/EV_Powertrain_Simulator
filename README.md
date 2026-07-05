# HIL-EV-Powertrain-Simulator
Hardware-in-the-Loop EV Powertrain Simulator on DE1-SoC board.

## Architecture
- FPGA (Cyclone V): Virtual powertrain physics engine
- ARM HPS (Linux): ECU control algorithms

## Week 1: HPS-FPGA Communication
Loopback test proving register communication over Lightweight AXI bridge.

## Week 2: Motor Physics Engine
Virtual DC motor simulation - RPM ramps 0→8000 with throttle input, smooth deceleration on release.

## Hardware
- Terasic DE1-SoC board
- Quartus Prime Lite 22.1

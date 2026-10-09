#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>

#define LW_BRIDGE_BASE     0xFF200000
#define HPS_TO_FPGA_OFFSET 0x0010
#define FPGA_TO_HPS_OFFSET 0x0000
#define BUTTON_PIO_OFFSET  0x100C0
#define MAP_SIZE           0x200000

// Drive modes
#define MODE_ECO     0  // KEY0 - 2000 RPM
#define MODE_NORMAL  1  // KEY1 - 5000 RPM
#define MODE_SPORT   2  // KEY2 - 7000 RPM
#define MODE_BRAKE   3  // KEY3 - Emergency brake

#define KP 0.05
#define KI 0.0001

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE,
                      MAP_SHARED, fd, LW_BRIDGE_BASE);

    volatile uint32_t *throttle   = (uint32_t*)(base + HPS_TO_FPGA_OFFSET);
    volatile uint32_t *rpm        = (uint32_t*)(base + FPGA_TO_HPS_OFFSET);
    volatile uint32_t *button_pio = (uint32_t*)(base + BUTTON_PIO_OFFSET);

    FILE *csv = fopen("/media/card/log.csv", "w");
    fprintf(csv, "timestamp_ms,mode,target_rpm,actual_rpm,throttle\n");

    double integral = 0.0;
    int throttle_val = 0;
    long timestamp = 0;
    int current_mode = MODE_NORMAL;
    int target_rpm = 5000;

    const char* mode_names[] = {"ECO", "NORMAL", "SPORT", "BRAKE"};
    const int   mode_rpms[]  = {2000, 5000, 7000, 0};

    printf("HIL EV Cruise Control — Button Speed Profiles\n");
    printf("KEY0=Eco(2000) KEY1=Normal(5000) KEY2=Sport(7000) KEY3=Brake\n");
    printf("Starting in NORMAL mode...\n\n");

    int i;
    for (i = 0; i < 3000; i++) {
        // Read buttons (active LOW, bits 3:0)
        uint32_t buttons = ~(*button_pio) & 0xF;

        // Check button presses and switch mode
        if (buttons & 0x1) {
            current_mode = MODE_ECO;
            integral = 0;
            printf(">> Switched to ECO mode (2000 RPM)\n");
        } else if (buttons & 0x2) {
            current_mode = MODE_NORMAL;
            integral = 0;
            printf(">> Switched to NORMAL mode (5000 RPM)\n");
        } else if (buttons & 0x4) {
            current_mode = MODE_SPORT;
            integral = 0;
            printf(">> Switched to SPORT mode (7000 RPM)\n");
        } else if (buttons & 0x8) {
            current_mode = MODE_BRAKE;
            integral = 0;
            printf(">> EMERGENCY BRAKE!\n");
        }

        target_rpm = mode_rpms[current_mode];

        // Emergency brake — cut throttle immediately
        if (current_mode == MODE_BRAKE) {
            *throttle = 0;
            throttle_val = 0;
        } else {
            // PI controller
            uint32_t current_rpm = *rpm;
            double error = target_rpm - (double)current_rpm;
            integral += error * 0.01;
            if (integral > 500)  integral = 500;
            if (integral < -500) integral = -500;

            double output = (KP * error) + (KI * integral);
            throttle_val = (int)output;
            if (throttle_val > 100) throttle_val = 100;
            if (throttle_val < 0)   throttle_val = 0;
            *throttle = (uint32_t)throttle_val;
        }

        uint32_t current_rpm = *rpm;
        fprintf(csv, "%ld,%s,%d,%u,%d\n",
                timestamp, mode_names[current_mode],
                target_rpm, current_rpm, throttle_val);

        printf("t=%ldms | Mode=%-6s | Target=%d | RPM=%u | Throttle=%d\n",
               timestamp, mode_names[current_mode],
               target_rpm, current_rpm, throttle_val);

        timestamp += 10;
        usleep(10000);
    }

    *throttle = 0;
    fclose(csv);
    printf("\nDone. Log saved.\n");
    munmap(base, MAP_SIZE);
    return 0;
}

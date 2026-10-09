#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>

#define LW_BRIDGE_BASE     0xFF200000
#define HPS_TO_FPGA_OFFSET 0x0010
#define FPGA_TO_HPS_OFFSET 0x0000
#define MAP_SIZE           0x1000

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE,
                      MAP_SHARED, fd, LW_BRIDGE_BASE);

    volatile uint32_t *throttle = (uint32_t*)(base + HPS_TO_FPGA_OFFSET);
    volatile uint32_t *rpm      = (uint32_t*)(base + FPGA_TO_HPS_OFFSET);

    FILE *csv = fopen("/media/card/drive_cycle.csv", "w");
    fprintf(csv, "timestamp_ms,phase,target_rpm,actual_rpm,throttle\n");

    long timestamp = 0;
    int i;

    printf("Drive cycle test starting...\n");

    // Phase 1: Accelerate to 3000 RPM
    printf("\n--- Phase 1: Accelerating to 3000 RPM ---\n");
    double integral = 0;
    for (i = 0; i < 100; i++) {
        uint32_t current_rpm = *rpm;
        double error = 3000 - (double)current_rpm;
        integral += error * 0.01;
        if (integral > 500) integral = 500;
        int t = (int)(0.05 * error + 0.0001 * integral);
        if (t > 100) t = 100;
        if (t < 0) t = 0;
        *throttle = t;
        fprintf(csv, "%ld,accelerate,3000,%u,%d\n", timestamp, current_rpm, t);
        printf("t=%ldms RPM=%u Throttle=%d\n", timestamp, current_rpm, t);
        timestamp += 10;
        usleep(10000);
    }

    // Phase 2: Cruise at 3000 RPM
    printf("\n--- Phase 2: Cruising at 3000 RPM ---\n");
    for (i = 0; i < 100; i++) {
        uint32_t current_rpm = *rpm;
        double error = 3000 - (double)current_rpm;
        integral += error * 0.01;
        if (integral > 500) integral = 500;
        int t = (int)(0.05 * error + 0.0001 * integral);
        if (t > 100) t = 100;
        if (t < 0) t = 0;
        *throttle = t;
        fprintf(csv, "%ld,cruise,3000,%u,%d\n", timestamp, current_rpm, t);
        printf("t=%ldms RPM=%u Throttle=%d\n", timestamp, current_rpm, t);
        timestamp += 10;
        usleep(10000);
    }

    // Phase 3: Accelerate to 6000 RPM
    printf("\n--- Phase 3: Accelerating to 6000 RPM ---\n");
    integral = 0;
    for (i = 0; i < 100; i++) {
        uint32_t current_rpm = *rpm;
        double error = 6000 - (double)current_rpm;
        integral += error * 0.01;
        if (integral > 500) integral = 500;
        int t = (int)(0.05 * error + 0.0001 * integral);
        if (t > 100) t = 100;
        if (t < 0) t = 0;
        *throttle = t;
        fprintf(csv, "%ld,accelerate,6000,%u,%d\n", timestamp, current_rpm, t);
        printf("t=%ldms RPM=%u Throttle=%d\n", timestamp, current_rpm, t);
        timestamp += 10;
        usleep(10000);
    }

    // Phase 4: Brake to 0
    printf("\n--- Phase 4: Braking to 0 RPM ---\n");
    for (i = 0; i < 100; i++) {
        uint32_t current_rpm = *rpm;
        *throttle = 0;
        fprintf(csv, "%ld,brake,0,%u,0\n", timestamp, current_rpm);
        printf("t=%ldms RPM=%u Throttle=0\n", timestamp, current_rpm);
        timestamp += 10;
        usleep(10000);
    }

    *throttle = 0;
    fclose(csv);
    printf("\nDone! Log saved to /media/card/drive_cycle.csv\n");
    munmap(base, MAP_SIZE);
    return 0;
}

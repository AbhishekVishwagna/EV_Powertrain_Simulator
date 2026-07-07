#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>

#define LW_BRIDGE_BASE     0xFF200000
#define HPS_TO_FPGA_OFFSET 0x0010
#define FPGA_TO_HPS_OFFSET 0x0000
#define MAP_SIZE           0x1000

#define TARGET_RPM   5000
#define KP           0.05
#define KI           0.0001
#define MAX_THROTTLE 100
#define MIN_THROTTLE 0

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE,
                      MAP_SHARED, fd, LW_BRIDGE_BASE);

    volatile uint32_t *throttle = (uint32_t*)(base + HPS_TO_FPGA_OFFSET);
    volatile uint32_t *rpm      = (uint32_t*)(base + FPGA_TO_HPS_OFFSET);

    FILE *csv = fopen("/media/card/log.csv", "w");
    fprintf(csv, "timestamp_ms,target_rpm,actual_rpm,throttle\n");

    double integral = 0.0;
    int throttle_val = 0;
    long timestamp = 0;

    printf("Cruise control started. Target RPM: %d\n", TARGET_RPM);

    int i;
    for (i = 0; i < 300; i++) {
        uint32_t current_rpm = *rpm;
        double error = TARGET_RPM - (double)current_rpm;
        integral += error * 0.01;

        // Anti-windup
        if (integral > 500) integral = 500;
        if (integral < -500) integral = -500;

        double output = (KP * error) + (KI * integral);
        throttle_val = (int)output;
        if (throttle_val > MAX_THROTTLE) throttle_val = MAX_THROTTLE;
        if (throttle_val < MIN_THROTTLE) throttle_val = MIN_THROTTLE;

        *throttle = (uint32_t)throttle_val;

        fprintf(csv, "%ld,%d,%u,%d\n", timestamp, TARGET_RPM, current_rpm, throttle_val);
        printf("t=%ldms | Target=%d | RPM=%u | Throttle=%d | Error=%.0f\n",
               timestamp, TARGET_RPM, current_rpm, throttle_val, error);

        timestamp += 10;
        usleep(10000);
    }

    *throttle = 0;
    fclose(csv);
    printf("Done. Log saved to /media/card/log.csv\n");
    munmap(base, MAP_SIZE);
    return 0;
}

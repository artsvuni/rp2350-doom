#ifndef DOOM_QMI8658_H
#define DOOM_QMI8658_H

#include <stdbool.h>
#include <stdint.h>

// Minimal fixed-memory QMI8658C interface for Doom's first motion-control
// experiment.  The initial model deliberately enables only the accelerometer;
// gyro assistance is a separate UX experiment, not an up-front requirement.
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} qmi8658_accel_raw_t;

bool qmi8658_init_accelerometer(void);
bool qmi8658_read_accel(qmi8658_accel_raw_t *raw);

#endif

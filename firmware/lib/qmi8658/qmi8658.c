#include "qmi8658.h"

#include "DEV_Config.h"
#include "hardware/i2c.h"
#include "pico/time.h"
#include <stddef.h>

enum {
    QMI8658_ADDR_LOW = 0x6b,
    QMI8658_ADDR_HIGH = 0x6a,
    QMI8658_WHO_AM_I = 0x00,
    QMI8658_CTRL1 = 0x02,
    QMI8658_CTRL2 = 0x03,
    QMI8658_CTRL5 = 0x06,
    QMI8658_CTRL7 = 0x08,
    QMI8658_AX_L = 0x35,
    QMI8658_DEVICE_ID = 0x05,
};

static uint8_t device_address;

static bool read_regs(uint8_t address, uint8_t reg, uint8_t *data, size_t len)
{
    if (i2c_write_blocking(I2C_PORT, address, &reg, 1, true) != 1) {
        return false;
    }
    return i2c_read_blocking(I2C_PORT, address, data, len, false) == (int)len;
}

static bool write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = { reg, value };
    return i2c_write_blocking(I2C_PORT, device_address, data, sizeof(data), false)
        == (int)sizeof(data);
}

bool qmi8658_init_accelerometer(void)
{
    static const uint8_t addresses[] = { QMI8658_ADDR_LOW, QMI8658_ADDR_HIGH };
    uint8_t id = 0;

    device_address = 0;
    for (size_t i = 0; i < sizeof(addresses); ++i) {
        if (read_regs(addresses[i], QMI8658_WHO_AM_I, &id, 1)
            && id == QMI8658_DEVICE_ID) {
            device_address = addresses[i];
            break;
        }
    }
    if (device_address == 0) {
        return false;
    }

    // Address auto-increment, little-endian output.  Configure +/-2g at
    // 62.5Hz and the widest built-in LPF (8.4Hz), then enable accel only.
    // This is close to Doom's 35Hz tic rate and avoids the gyro's extra power
    // and drift until a gyro-assisted model is deliberately evaluated.
    if (!write_reg(QMI8658_CTRL1, 0x40)
        || !write_reg(QMI8658_CTRL2, 0x07)
        || !write_reg(QMI8658_CTRL5, 0x07)
        || !write_reg(QMI8658_CTRL7, 0x01)) {
        device_address = 0;
        return false;
    }

    sleep_ms(20);
    return true;
}

bool qmi8658_read_accel(qmi8658_accel_raw_t *raw)
{
    uint8_t data[6];
    if (device_address == 0 || raw == NULL
        || !read_regs(device_address, QMI8658_AX_L, data, sizeof(data))) {
        return false;
    }

    raw->x = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
    raw->y = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8));
    raw->z = (int16_t)((uint16_t)data[4] | ((uint16_t)data[5] << 8));
    return true;
}

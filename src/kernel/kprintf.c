#include <stdint.h>
#include <stddef.h>

extern void serial_putc(char c);
extern void serial_write(const char *s);

// Helper: Convert unsigned int to string
static void uitoa(uint64_t value, char *buf, int base) {
    static const char digits[] = "0123456789ABCDEF";
    char tmp[32];
    int i = 0;

    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (value > 0) {
        tmp[i++] = digits[value % base];
        value /= base;
    }

    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

void serial_write_dec(uint64_t value) {
    char buf[32];
    uitoa(value, buf, 10);
    serial_write(buf);
}

void serial_write_hex(uint64_t value) {
    char buf[32];
    serial_write("0x");
    uitoa(value, buf, 16);
    serial_write(buf);
}

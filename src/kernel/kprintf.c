#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

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

// Basic kprintf implementation with format string support
void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p != '\0'; p++) {
        if (*p == '%' && *(p + 1) != '\0') {
            p++;
            switch (*p) {
                case 'd':  // Decimal integer
                case 'u': {  // Unsigned integer
                    uint64_t val = va_arg(args, uint64_t);
                    serial_write_dec(val);
                    break;
                }
                case 'x':  // Hexadecimal (lowercase)
                case 'X': {  // Hexadecimal (uppercase)
                    uint64_t val = va_arg(args, uint64_t);
                    serial_write_hex(val);
                    break;
                }
                case 'p': {  // Pointer
                    void *ptr = va_arg(args, void *);
                    serial_write_hex((uint64_t)ptr);
                    break;
                }
                case 's': {  // String
                    const char *str = va_arg(args, const char *);
                    if (str) {
                        serial_write(str);
                    } else {
                        serial_write("(null)");
                    }
                    break;
                }
                case 'c': {  // Character
                    char ch = (char)va_arg(args, int);
                    serial_putc(ch);
                    break;
                }
                case '%':  // Literal %
                    serial_putc('%');
                    break;
                default:  // Unknown format specifier
                    serial_putc('%');
                    serial_putc(*p);
                    break;
            }
        } else {
            serial_putc(*p);
        }
    }

    va_end(args);
}

#include <stdio.h>
#include <stdarg.h>
#include <lk/sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))

void _panic(void *caller, const char *fmt, ...)
{
    printf("panic (caller %p): ", caller);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    abort();
}

#if !DISABLE_DEBUG_OUTPUT

void hexdump(const void *ptr, size_t len)
{
    addr_t address = (addr_t)ptr;
    size_t count;

    for (count = 0 ; count < len; count += 16) {
        union {
            uint32_t buf[4];
            uint8_t  cbuf[16];
        } u;
        size_t s = ROUNDUP(MIN(len - count, 16), 4);
        size_t i;

        printf("0x%08"PRIxPTR": ", address);
        for (i = 0; i < s / 4; i++) {
            u.buf[i] = ((const uint32_t *)address)[i];
            printf("%08x ", u.buf[i]);
        }
        for (; i < 4; i++) {
            printf("         ");
        }
        printf("|");

        for (i=0; i < 16; i++) {
            char c = u.cbuf[i];
            if (i < s && isprint(c)) {
                printf("%c", c);
            } else {
                printf(".");
            }
        }
        printf("|\n");
        address += 16;
    }
}

void hexdump8_ex(const void *ptr, size_t len, uint64_t disp_addr)
{
    addr_t address = (addr_t)ptr;
    size_t count;
    size_t i;
    const char *addr_fmt = ((disp_addr + len) > 0xFFFFFFFF)
                           ? "0x%016llx: "
                           : "0x%08llx: ";

    for (count = 0 ; count < len; count += 16) {
        printf(addr_fmt, disp_addr + count);

        for (i=0; i < MIN(len - count, 16); i++) {
            printf("%02hhx ", *(const uint8_t *)(address + i));
        }

        for (; i < 16; i++) {
            printf("   ");
        }

        printf("|");

        for (i=0; i < MIN(len - count, 16); i++) {
            char c = ((const char *)address)[i];
            printf("%c", isprint(c) ? c : '.');
        }

        printf("\n");
        address += 16;
    }
}

#endif // !DISABLE_DEBUG_OUTPUT

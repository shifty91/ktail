#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utils.h"

void *kmalloc(size_t size)
{
    void *mem = malloc(size);
    if (!mem)
        skerr("malloc() failed");
    return mem;
}

void *kmalloc_array(size_t nb, size_t size)
{
    static const size_t MUL_NO_OVERFLOW = (size_t)1 << (sizeof(size_t) * 4);
    void *mem;

    /*
     * Overflow check.
     * Taken from OpenBSD's reallocarray function:
     * http://cvsweb.openbsd.org/cgi-bin/cvsweb/src/lib/libc/stdlib/reallocarray.c?rev=1.3&content-type=text/x-cvsweb-markup
     */
    if ((nb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
        nb > 0 && SIZE_MAX / nb < size)
        serr("kmalloc_array() failed: overflow detected.");

    mem = malloc(nb * size);
    if (!mem)
        skerr("malloc() failed");
    return mem;
}

void *kzmalloc(size_t size)
{
    void *mem = kmalloc(size);
    memset(mem, '\0', size);
    return mem;
}

void *kzmalloc_array(size_t nb, size_t size)
{
    void *mem = kmalloc_array(nb, size);
    memset(mem, '\0', nb * size);
    return mem;
}

int kstrtol(const char * restrict str, int base, long * restrict res)
{
    char *end;
    long value;

    if (!str || !res) {
        perr("NULL pointer(s) passed to '%s'", __func__);
        return -EINVAL;
    }

    value = strtol(str, &end, base);
    if (end == str || *end != '\0' ||
        ((value == LONG_MAX || value == LONG_MIN) && errno == ERANGE))
        return -EINVAL;

    *res = value;
    return 0;
}

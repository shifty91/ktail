#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
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

void *kzmalloc(size_t size)
{
    void *mem = kmalloc(size);
    memset(mem, '\0', size);
    return mem;
}

int kstrtol(const char * restrict str, unsigned int base, long * restrict res)
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

size_t file_size(const FILE * const f)
{
    struct stat info;

    if (!f) {
        perr("NULL pointer passed to '%s'", __func__);
        return 0;
    }

    if (fstat(fileno((FILE *)f), &info)) {
        skperr("fstat() failed");
        return 0;
    }

    return info.st_size;
}

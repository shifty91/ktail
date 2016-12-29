/*
 * Copyright (C) 2016 Kurt Kanzenbach <kurt@kmk-computers.de>
 *
 * This file is part of Ktail.
 *
 * Ktail is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ktail is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ktail.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utils.h"

#include "ktail_config.h"

void *kmalloc(size_t size)
{
    void *mem = malloc(size);
    if (!mem)
        err_errno("malloc() failed");
    return mem;
}

void *kmalloc_array(size_t nb, size_t size)
{
    size_t alloc_size = 0;
    void *mem;

#ifdef HAVE_BUILTIN_UMULL_OVERFLOW
    if (__builtin_umull_overflow(nb, size, &alloc_size))
        err("kmalloc_array() failed: overflow detected.");
#else
    if (size && nb > SIZE_MAX / size)
        err("kmalloc_array() failed: overflow detected.");
    else
        alloc_size = nb * size;
#endif

    mem = malloc(alloc_size);
    if (!mem)
        err_errno("malloc() failed");
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

void _free(void **ptr)
{
    free(*ptr);
    *ptr = NULL;
}

int kstrtol(const char *str, int base, long *res)
{
    char *end;
    long value;

    if (!str || !res) {
        print_err("NULL pointer(s) passed to '%s'", __func__);
        return -EINVAL;
    }

    value = strtol(str, &end, base);
    if (end == str || *end != '\0' ||
        ((value == LONG_MAX || value == LONG_MIN) && errno == ERANGE))
        return -EINVAL;

    *res = value;
    return 0;
}

void _log(const char *restrict level, int die, int with_errno,
          const char *restrict file, int line, const char *restrict fmt, ...)
{
    FILE *out;
    va_list args;

    va_start(args, fmt);

    if (!strcmp(level, "ERROR") || !strcmp(level, "WARN"))
        out = stderr;
    else
        out = stdout;

    fprintf(out, "[%s %s:%d]: ", level, file, line);
    vfprintf(out, fmt, args);
    if (with_errno)
        fprintf(out, ": %s", strerror(errno));
    fprintf(out, "\n");
    fflush(out);

    va_end(args);

    if (die)
        exit(EXIT_FAILURE);
}

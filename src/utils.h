#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

/* asserts */
#define ASSERT_PARAM_NOT_NULL(param)                                \
    do {                                                            \
        if (!(param)) {                                             \
            print_err("'%s': NULL pointer passed to '%s'", #param,  \
                      __func__);                                    \
            return -EINVAL;                                         \
        }                                                           \
    } while (0)

#define ASSERT_PARAM_NOT_NULL_VOID(param)                           \
    do {                                                            \
        if (!(param)) {                                             \
            print_err("'%s': NULL pointer passed to '%s'", #param,  \
                      __func__);                                    \
            return;                                                 \
        }                                                           \
    } while (0)

/* memory allocations */
void *kmalloc(size_t size);
void *kzmalloc(size_t size);
void *kmalloc_array(size_t nb, size_t size);
void *kzmalloc_array(size_t nb, size_t size);

#define kfree(ptr)                                                      \
    do {                                                                \
        free(ptr);                                                      \
        ptr = NULL;                                                     \
    } while (0)

/* conversion */
int kstrtol(const char * restrict str, int base, long * restrict res);

/* logging */
#define err(...)                                                        \
    do {                                                                \
        _log("ERROR", 1, 0, basename(__FILE__), __LINE__, __VA_ARGS__); \
    } while (0)

#define print_err(...)                                                  \
    do {                                                                \
        _log("ERROR", 0, 0, basename(__FILE__), __LINE__, __VA_ARGS__); \
    } while (0)

#define err_errno(...)                                                  \
    do {                                                                \
        _log("ERROR", 1, 1, basename(__FILE__), __LINE__, __VA_ARGS__); \
    } while (0)

#define print_err_errno(...)                                            \
    do {                                                                \
        _log("ERROR", 0, 1, basename(__FILE__), __LINE__, __VA_ARGS__); \
    } while (0)

#define warn(...)                                                       \
    do {                                                                \
        _log("WARN", 0, 0, basename(__FILE__), __LINE__, __VA_RGS__);   \
    } while (0)

#define warn_errno(...)                                                 \
    do {                                                                \
        _log("WARN", 0, 1, basename(__FILE__), __LINE__, __VA_ARGS__);  \
    } while (0)

#define info(...)                                                       \
    do {                                                                \
        _log("INFO", 0, 0, basename(__FILE__), __LINE__, __VA_ARGS__);  \
    } while (0)

#ifndef NDEBUG
#define dbg(...)                                                        \
    do {                                                                \
        _log("DEBUG", 0, 0, basename(__FILE__), __LINE__, __VA_ARGS__); \
    } while (0)
#else
#define dbg(...)
#endif

void _log(const char * restrict level, int die, int with_errno,
          const char * restrict file, int line, const char * restrict fmt, ...);

#endif /* _UTILS_H_ */

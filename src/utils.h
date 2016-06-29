#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

/* printing */
#ifndef NDEBUG
#define dbg(fmt, ...)                                                   \
    do {                                                                \
        printf("[DEBUG %s:%d]: " fmt "\n", basename(__FILE__),          \
               __LINE__, ##__VA_ARGS__);                                \
    } while (0)
#define sdbg(str) dbg("%s", str)
#else
#define dbg(fmt, ...)
#define sdbg(str)
#endif

#define info(fmt, ...)                                                  \
    do {                                                                \
        printf("[INFO %s:%d]: " fmt "\n", basename(__FILE__), __LINE__, \
               ##__VA_ARGS__);                                          \
    } while (0)
#define sinfo(str) info("%s", str)

#define warn(fmt, ...)                                                  \
    do {                                                                \
        fprintf(stderr, "[WARN %s:%d]: " fmt "\n", basename(__FILE__),  \
                __LINE__, ##__VA_ARGS__);                               \
    } while (0)
#define swarn(str) warn("%s", str)

#define kerr(fmt, ...)                                                  \
    do {                                                                \
        fprintf(stderr, "[ERROR %s:%d]: " fmt ": %s\n", basename(__FILE__), \
                __LINE__, ##__VA_ARGS__, strerror(errno));              \
        exit(EXIT_FAILURE);                                             \
    } while (0)
#define skerr(str) kerr("%s", str)

#define err(fmt, ...)                                                   \
    do {                                                                \
        fprintf(stderr, "[ERROR %s:%d]: " fmt "\n", basename(__FILE__), \
                __LINE__, ##__VA_ARGS__);                               \
        exit(EXIT_FAILURE);                                             \
    } while (0)
#define serr(str) err("%s", str)

#define kperr(fmt, ...)                                                 \
    do {                                                                \
        fprintf(stderr, "[ERROR %s:%d]: " fmt ": %s\n", basename(__FILE__), \
                __LINE__, ##__VA_ARGS__, strerror(errno));              \
    } while (0)
#define skperr(str) kperr("%s", str)

#define perr(fmt, ...)                                                  \
    do {                                                                \
        fprintf(stderr, "[ERROR %s:%d]: " fmt "\n", basename(__FILE__), \
                __LINE__, ##__VA_ARGS__);                               \
    } while (0)
#define sperr(str) perr("%s", str)

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
int kstrtol(const char * restrict str, unsigned int base, long * restrict res);

/* utils */
size_t file_size(const FILE * const f);

#endif /* _UTILS_H_ */

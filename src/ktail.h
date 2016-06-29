#ifndef _KTAIL_H_
#define _KTAIL_H_

#include "ktail_config.h"

#include <stddef.h>
#ifdef HAVE_KQUEUE
#include <sys/types.h>
#include <sys/event.h>
#elif HAVE_INOTIFY
#include <sys/inotify.h>
#endif

struct ktail_context {
    FILE *f;
    char *data;
    size_t line_counter;
    size_t bytes;
#ifdef HAVE_KQUEUE
    int fd;
    int kq;
    struct kevent change;
#elif HAVE_INOTIFY
    int fd;
    int wd;
#endif
};

/* memory handling */
struct ktail_context *ktail_init(void);
void ktail_free(struct ktail_context *ctx);

/* functions */
int ktail_open(struct ktail_context *ctx);
int ktail_reopen(struct ktail_context *ctx);
void ktail_close(struct ktail_context *ctx);
int ktail_read(struct ktail_context *ctx);
int ktail_read_and_print(struct ktail_context *ctx);
int ktail_wait(struct ktail_context *ctx);
void ktail_print(const struct ktail_context *ctx);
int ktail_wait_init(struct ktail_context *ctx);
void ktail_wait_close(struct ktail_context *ctx);

#endif /* _KTAIL_H_ */

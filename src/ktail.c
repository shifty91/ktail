#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

#include "ktail.h"

#include "utils.h"
#include "config.h"
#include "ktail_config.h"

#define SLEEP_INTERVALL 500
#define MAX_LINE (sysconf(_SC_LINE_MAX) + 1)
#ifdef HAVE_INOTIFY
#define BUF_SIZE (1 * sizeof(struct inotify_event))
#endif

struct ktail_context *ktail_init(void)
{
    /* allocate memory only once! */
    struct ktail_context *ctx = (struct ktail_context *)kzmalloc(sizeof(*ctx));

    ctx->data = (char **)kmalloc_array(sizeof(char *), config.n);

    for (size_t i = 0; i < config.n; ++i)
        ctx->data[i] = (char *)kmalloc(MAX_LINE);

    return ctx;
}

void ktail_free(struct ktail_context *ctx)
{
    if (!ctx) {
        perr("NULL pointer passed to '%s'", __func__);
        return;
    }

    for (size_t i = 0; i < config.n; ++i)
        kfree(ctx->data[i]);
    kfree(ctx->data);
    kfree(ctx);
}

int ktail_wait_init(struct ktail_context *ctx)
{
    if (!ctx) {
        perr("NULL pointer passed to '%s'", __func__);
        return -EINVAL;
    }

#ifdef HAVE_KQUEUE
    ctx->fd = open(config.file, O_RDONLY);
    if (ctx->fd < 0) {
        skperr("open() failed");
        return -EIO;
    }

    ctx->kq = kqueue();
    if (ctx->kq < 0) {
        skperr("kqueue() failed");
        return -ENOMEM;
    }

    EV_SET(&ctx->change, ctx->fd, EVFILT_VNODE,
           EV_ADD | EV_ENABLE | EV_ONESHOT,
           NOTE_EXTEND | NOTE_WRITE,
           0, 0);
#elif HAVE_INOTIFY
    ctx->fd = inotify_init();
    if (ctx->fd < 0) {
        skperr("inotify_init() failed");
        return -ENOMEM;
    }
    ctx->wd = inotify_add_watch(ctx->fd, config.file, IN_MODIFY);
    if (ctx->wd < 0) {
        skperr("inotify_add_watch() failed");
        return -ENOMEM;
    }
#endif

    return 0;
}

int ktail_wait(struct ktail_context *ctx)
{
    if (!ctx) {
        perr("NULL pointer passed to '%s'", __func__);
        return -EINVAL;
    }

#ifdef HAVE_KQUEUE
    struct kevent event;
    int nev;

    /* zZz */
    while (42) {
        nev = kevent(ctx->kq, &ctx->change, 1, &event, 1, NULL);
        if (nev < 0) {
            if (errno == EINTR)
                return 0;
            skperr("kevent() failed");
            return -ENOMEM;
        }

        if (event.fflags & NOTE_EXTEND || event.fflags & NOTE_WRITE)
            return 0;
    }
#elif HAVE_INOTIFY
    char buf[BUF_SIZE];

    while (42) {
        struct inotify_event *event;
        ssize_t rc;

        rc = read(ctx->fd, buf, BUF_SIZE);
        if (rc == -1 || rc == 0) {
            if (errno == EINTR)
                return 0;
            skperr("Failed to read from inotify fd");
            return -EIO;
        }

        event = (struct inotify_event *)buf;
        if (event->mask & IN_MODIFY)
            return 0;
    }
#else
    while (42) {
        struct stat buf;

        if (stat(config.file, &buf)) {
            skperr("lstat() failed");
            return -EIO;
        }

        if (buf.st_size > ctx->bytes)
            break;

        /* zZz */
        if (usleep(SLEEP_INTERVALL)) {
            if (errno == EINTR)
                return 0;
            skperr("usleep() failed");
            return -EINTR;
        }
    }
#endif

    return 0;
}

void ktail_wait_close(struct ktail_context *ctx)
{
    if (!ctx) {
        perr("NULL pointer passed to '%s'", __func__);
        return;
    }

#ifdef HAVE_KQUEUE
    close(ctx->kq);
    close(ctx->fd);
#elif HAVE_INOTIFY
    inotify_rm_watch(ctx->fd, ctx->wd);
    close(ctx->fd);
#endif
}

int ktail_open(struct ktail_context *ctx)
{
    if (!ctx) {
        perr("NULL pointer passed to '%s'", __func__);
        return -EINVAL;
    }

    if (!(ctx->f = fopen(config.file, "r"))) {
        skperr("fopen() failed");
        return -EIO;
    }

    return 0;
}

int ktail_reopen(struct ktail_context *ctx)
{
    if (!ctx) {
        perr("NULL pointer passed to '%s'", __func__);
        return -EINVAL;
    }

    fclose(ctx->f);

    if (!(ctx->f = fopen(config.file, "r"))) {
        skperr("fopen() failed");
        return -EIO;
    }

    /* seek! */
    if (fseek(ctx->f, ctx->bytes, SEEK_SET)) {
        skperr("fseek() failed");
        return -EINVAL;
    }

    return 0;
}

void ktail_close(struct ktail_context *ctx)
{
    if (!ctx) {
        perr("NULL pointer passed to '%s'", __func__);
        return;
    }
    fclose(ctx->f);
}

int ktail_read(struct ktail_context *ctx)
{
    size_t off = 0;
    int c;

    if (!ctx) {
        perr("NULL pointer passed to '%s'", __func__);
        return -EINVAL;
    }

    while ((c = fgetc(ctx->f)) != EOF) {
        ctx->bytes++;

        /* length check */
        if (off == (MAX_LINE - 1)) {
            perr("Length of line '%lu' is too long. Cutting it.",
                 ctx->line_counter);

            /* read until eol */
            while ((c = fgetc(ctx->f)) != '\n')
                ;
        }

        if (c == '\n') {
            ctx->data[ctx->line_counter % config.n][off] = '\0';
            off = 0;
            ctx->line_counter++;
            continue;
        }
        ctx->data[ctx->line_counter % config.n][off++] = c;
    }
    if (ferror(ctx->f)) {
        skperr("fgetc() failed");
        return -EIO;
    }

    /* deal with last line */
    if (off != 0)
        ctx->data[ctx->line_counter % config.n][off] = '\0';

    return 0;
}

int ktail_read_and_print(struct ktail_context *ctx)
{
    int c;

    if (!ctx) {
        perr("NULL pointer passed to '%s'", __func__);
        return -EINVAL;
    }

    while ((c = fgetc(ctx->f)) != EOF) {
        ctx->bytes++;
        printf("%c", c);
    }
    fflush(stdout);
    if (ferror(ctx->f)) {
        skperr("fgetc() failed");
        return -EIO;
    }

    return 0;
}

void ktail_print(struct ktail_context *ctx)
{
    if (!ctx) {
        perr("NULL pointer passed to '%s'", __func__);
        return;
    }

    size_t lines = ctx->line_counter >= config.n ? config.n : ctx->line_counter;
    size_t start = ctx->line_counter >= config.n ? ctx->line_counter : 0;

    for (size_t i = start; i < start + lines; ++i)
        printf("%s\n", ctx->data[i % config.n]);
    fflush(stdout);
}

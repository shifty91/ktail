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

#if !defined(HAVE_INOTIFY) && !defined(HAVE_KQUEUE)
#define SLEEP_INTERVALL 500
#endif
#ifdef HAVE_INOTIFY
#define BUF_SIZE (1 * sizeof(struct inotify_event))
#endif

static size_t MAX_LINE;

__attribute__((constructor)) static void init(void)
{
    long res = sysconf(_SC_LINE_MAX);
    if (res < 0)
        err_errno("sysconf() failed");
    if (res == 0)
        err("sysconf() returned 0 for _SC_LINE_MAX");
    MAX_LINE = (size_t)res + 1;
}

static inline size_t idx(const size_t line, const size_t off)
{
    return line * MAX_LINE + off;
}

struct ktail_context *ktail_init(void)
{
    /* allocate memory only once! */
    struct ktail_context *ctx = (struct ktail_context *)kzmalloc(sizeof(*ctx));
    ctx->data = (char *)kmalloc_array(MAX_LINE, config.n);

    return ctx;
}

void ktail_free(struct ktail_context *ctx)
{
    ASSERT_PARAM_NOT_NULL_VOID(ctx);

    kfree(ctx->data);
    kfree(ctx);
}

int ktail_wait_init(struct ktail_context *ctx)
{
    ASSERT_PARAM_NOT_NULL(ctx);

#ifdef HAVE_KQUEUE
    ctx->fd = open(config.file, O_RDONLY);
    if (ctx->fd < 0) {
        print_err_errno("open() failed");
        return -EIO;
    }

    ctx->kq = kqueue();
    if (ctx->kq < 0) {
        print_err_errno("kqueue() failed");
        return -ENOMEM;
    }

    EV_SET(&ctx->change, ctx->fd, EVFILT_VNODE,
           EV_ADD | EV_ENABLE | EV_ONESHOT,
           NOTE_EXTEND | NOTE_WRITE,
           0, 0);
#elif HAVE_INOTIFY
    ctx->fd = inotify_init();
    if (ctx->fd < 0) {
        print_err_errno("inotify_init() failed");
        return -ENOMEM;
    }
    ctx->wd = inotify_add_watch(ctx->fd, config.file, IN_MODIFY);
    if (ctx->wd < 0) {
        print_err_errno("inotify_add_watch() failed");
        return -ENOMEM;
    }
#endif

    return 0;
}

int ktail_wait(struct ktail_context *ctx)
{
    ASSERT_PARAM_NOT_NULL(ctx);

#ifdef HAVE_KQUEUE
    struct kevent event;
    int nev;

    /* zZz */
    while (42) {
        nev = kevent(ctx->kq, &ctx->change, 1, &event, 1, NULL);
        if (nev < 0) {
            if (errno == EINTR)
                break;
            print_err_errno("kevent() failed");
            return -ENOMEM;
        }

        if (event.fflags & NOTE_EXTEND || event.fflags & NOTE_WRITE)
            break;
    }
#elif HAVE_INOTIFY
    char buf[BUF_SIZE];

    while (42) {
        struct inotify_event *event;
        ssize_t rc;

        rc = read(ctx->fd, buf, BUF_SIZE);
        if (rc == -1 || rc == 0) {
            if (errno == EINTR)
                break;
            print_err_errno("Failed to read from inotify fd");
            return -EIO;
        }

        event = (struct inotify_event *)buf;
        if (event->mask & IN_MODIFY)
            break;
    }
#else
    while (42) {
        struct stat buf;

        if (stat(config.file, &buf)) {
            print_err_errno("lstat() failed");
            return -EIO;
        }

        if (buf.st_size > ctx->bytes)
            break;

        /* zZz */
        if (usleep(SLEEP_INTERVALL)) {
            if (errno == EINTR)
                break;
            print_err_errno("usleep() failed");
            return -EINTR;
        }
    }
#endif

    return 0;
}

void ktail_wait_close(struct ktail_context *ctx)
{
    ASSERT_PARAM_NOT_NULL_VOID(ctx);

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
    ASSERT_PARAM_NOT_NULL(ctx);

    if (!(ctx->f = fopen(config.file, "r"))) {
        print_err_errno("fopen() failed");
        return -EIO;
    }

    return 0;
}

int ktail_reopen(struct ktail_context *ctx)
{
    ASSERT_PARAM_NOT_NULL(ctx);

    fclose(ctx->f);

    if (!(ctx->f = fopen(config.file, "r"))) {
        print_err_errno("fopen() failed");
        return -EIO;
    }

    /* seek! */
    if (fseek(ctx->f, ctx->bytes, SEEK_SET)) {
        print_err_errno("fseek() failed");
        return -EINVAL;
    }

    return 0;
}

void ktail_close(struct ktail_context *ctx)
{
    ASSERT_PARAM_NOT_NULL_VOID(ctx);

    fclose(ctx->f);
}

int ktail_read(struct ktail_context *ctx)
{
    size_t off = 0;
    int c;

    ASSERT_PARAM_NOT_NULL(ctx);

    while ((c = fgetc(ctx->f)) != EOF) {
        ctx->bytes++;

        /* length check */
        if (off == (MAX_LINE - 1)) {
            print_err("Length of line '%lu' is too long. Cutting it.",
                      ctx->line_counter);

            /* read until eol */
            while ((c = fgetc(ctx->f)) != EOF) {
                ctx->bytes++;
                if (c == '\n')
                    break;
            }
            if (ferror(ctx->f)) {
                print_err_errno("fgetc() failed");
                return -EIO;
            }
        }

        if (c == '\n') {
            ctx->data[idx(ctx->line_counter % config.n, off)] = '\0';
            off = 0;
            ctx->line_counter++;
            continue;
        }
        ctx->data[idx(ctx->line_counter % config.n, off++)] = c;
    }
    if (ferror(ctx->f)) {
        print_err_errno("fgetc() failed");
        return -EIO;
    }

    /* deal with last line */
    if (off != 0)
        ctx->data[idx(ctx->line_counter % config.n, off)] = '\0';

    return 0;
}

int ktail_read_and_print(struct ktail_context *ctx)
{
    int c;

    ASSERT_PARAM_NOT_NULL(ctx);

    while ((c = fgetc(ctx->f)) != EOF) {
        ctx->bytes++;
        printf("%c", c);
    }
    fflush(stdout);
    if (ferror(ctx->f)) {
        print_err_errno("fgetc() failed");
        return -EIO;
    }

    return 0;
}

void ktail_print(const struct ktail_context *ctx)
{
    ASSERT_PARAM_NOT_NULL_VOID(ctx);

    const size_t lines = ctx->line_counter >= config.n ? config.n : ctx->line_counter;
    const size_t start = ctx->line_counter >= config.n ? ctx->line_counter : 0;

    for (size_t i = start; i < start + lines; ++i)
        printf("%s\n", ctx->data + idx(i % config.n, 0));
    fflush(stdout);
}

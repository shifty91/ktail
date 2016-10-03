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

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
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>

#include "config.h"
#include "utils.h"
#include "ktail.h"

static volatile int stop;

static struct option long_options[] = {
    { "number" , required_argument, NULL, 'n' },
    { "follow" , no_argument      , NULL, 'f' },
    { "version", no_argument      , NULL, 'v' },
    { "help"   , no_argument      , NULL, 'h' },
    { NULL     , 0                , NULL,  0  }
};

__attribute__((noreturn)) static void print_usage_and_die(int ret)
{
    fprintf(stderr, "ktail [options] <file>\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  --number, -n <lines>: last <lines> lines\n");
    fprintf(stderr, "  --follow, -f: follow output\n");
    fprintf(stderr, "Ktail version 1.3, Copyright (C) 2016 Kurt Kanzenbach <kurt@kmk-computers.de>\n");

    ret ? exit(EXIT_FAILURE) : exit(EXIT_SUCCESS);
}

static void term_handler(int sig)
{
    (void)sig;
    stop = 1;
}

static int is_tailable(const char *file)
{
    struct stat sb;

    /* follow symlinks */
    if (stat(file, &sb))
        err_errno("stat() failed");

    return S_ISREG(sb.st_mode);
}

static void setup_signals(void)
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = term_handler;
    sa.sa_flags = 0;

    if (sigaction(SIGTERM, &sa, NULL))
        err_errno("sigaction() failed");
    if (sigaction(SIGINT, &sa, NULL))
        err_errno("sigaction() failed");
}

static int ktail(void)
{
    struct ktail_context *ctx;
    int ret = -EIO;

    ctx = ktail_init();

    if (ktail_open(ctx))
        goto out0;

    if (ktail_read(ctx))
        goto out1;

    ktail_print(ctx);

    ret = 0;

out1:
    ktail_close(ctx);
out0:
    ktail_free(ctx);

    return ret;
}

static int ktail_with_follow(void)
{
    struct ktail_context *ctx;
    int ret = -EIO;

    setup_signals();

    /* tail */
    ctx = ktail_init();

    if (ktail_open(ctx))
        goto out0;

    if (ktail_read(ctx))
        goto out1;

    ktail_print(ctx);

    /* wait */
    ktail_wait_init(ctx);
    while (!stop) {
        if (ktail_wait(ctx))
            goto out2;
        if (ktail_reopen(ctx))
            goto out2;
        if (ktail_read_and_print(ctx))
            goto out2;
    }

    ret = 0;

out2:
    ktail_wait_close(ctx);
out1:
    ktail_close(ctx);
out0:
    ktail_free(ctx);

    return ret;
}

int main(int argc, char *argv[])
{
    char *number_str = NULL;
    long n = 1000;
    int c, res;

    /* get args */
    while ((c = getopt_long(argc, argv, "n:fvh", long_options, NULL)) != -1) {
        switch (c) {
        case 'n':
            number_str = optarg;
            break;
        case 'f':
            config.f_flag = 1;
            break;
        case 'v':
        case 'h':
            print_usage_and_die(0);
        default:
            print_usage_and_die(1);
        }
    }
    if (argc - optind != 1)
        print_usage_and_die(1);

    /* set args */
    config.file = argv[optind];
    if (number_str && (kstrtol(number_str, 10, &n) || n <= 0))
        err("Invalid argument for --number");
    config.n = n;

    /* sanity checks */
    if (!is_tailable(config.file))
        err("The file '%s' cannot be tailed.", config.file);

    /* print tail */
    res = config.f_flag ? ktail_with_follow() : ktail();

    return res ? EXIT_FAILURE : EXIT_SUCCESS;
}

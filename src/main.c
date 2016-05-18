#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>

#include "config.h"
#include "utils.h"
#include "ktail.h"

static volatile int stop;

static struct option long_options[] = {
    { "number", required_argument, NULL, 'n' },
    { "follow", required_argument, NULL, 'f' },
    { NULL    , 0                , NULL, 0   }
};

static void print_usage_and_die(void)
{
    fprintf(stderr, "ktail [options] <file>\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  --number, -n <lines>: last <lines> lines\n");
    fprintf(stderr, "  --follow, -f: follow output\n");
    fprintf(stderr, "Ktail version 1.1, Copyright (C) 2016 Kurt Kanzenbach <kurt@kmk-computers.de>\n");

    exit(EXIT_FAILURE);
}

static void term_handler(int sig)
{
    (void)sig;
    stop = 1;
}

static void setup_signals(void)
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = term_handler;
    sa.sa_flags = 0;

    if (sigaction(SIGTERM, &sa, NULL))
        skerr("sigaction() failed");
    if (sigaction(SIGINT, &sa, NULL))
        skerr("sigaction() failed");
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
    while ((c = getopt_long(argc, argv, "n:f", long_options, NULL)) != -1) {
        switch (c) {
        case 'n':
            number_str = optarg;
            break;
        case 'f':
            config.f_flag = 1;
            break;
        default:
            print_usage_and_die();
        }
    }
    if (argc - optind != 1)
        print_usage_and_die();

    /* set args */
    config.file = argv[optind];
    if (number_str && (kstrtol(number_str, 10, &n) || n <= 0))
        serr("Invalid argument for --number");
    config.n = n;

    /* print tail */
    if (config.f_flag) {
        res = ktail_with_follow();
    } else {
        res = ktail();
    }

    return res ? EXIT_FAILURE : EXIT_SUCCESS;
}

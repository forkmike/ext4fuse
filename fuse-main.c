/*
 * Copyright (c) 2010, Gerard Lledó Vives, gerard.lledo@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See README and COPYING for
 * more details.
 *
 * Obviously inspired on the great FUSE hello world example:
 *      - http://fuse.sourceforge.net/helloworld.html
 */

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>

#include "common.h"
#include "disk.h"
#include "inode.h"
#include "logging.h"
#include "ops.h"
#include "super.h"

static struct fuse_operations e4f_ops = {
    .getattr    = op_getattr,
    .readdir    = op_readdir,
    .open       = op_open,
    .read       = op_read,
    .readlink   = op_readlink,
    .init       = op_init,
};

void signal_handle_sigsegv(int signal)
{
    UNUSED(signal);
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    DEBUG("========================================");
    DEBUG("Segmentation Fault.  Starting backtrace:");
    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);

    for (i = 0; i < size; i++)
        DEBUG("%s", strings[i]);
    DEBUG("========================================");

    abort();
}

void print_usage(char* self)
{
    fprintf(stderr, "Usage: %s fs mountpoint [-l logfile] [-o offset/bytes]\n", self);
}

int main(int argc, char *argv[])
{
    char* logfilep = DEFAULT_LOG_FILE;
    long offset = 0;

    if (argc < 3 || argc > 7) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (signal(SIGSEGV, signal_handle_sigsegv) == SIG_ERR) {
        fprintf(stderr, "Failed to initialize signals\n");
        return EXIT_FAILURE;
    }

    if (argc > 3 && (argc == 5 || argc == 7)) {
        if (argv[3][1] == 'l') {
            logfilep = argv[4];
        } else if (argc == 7 && argv[5][1] == 'l') {
            logfilep = argv[6];
        }
        if (argv[3][1] == 'o') {
            offset = atol(argv[4]);
        } else if (argc == 7 && argv[5][1] == 'o') {
            offset = atol(argv[6]);
        }
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (logging_open(logfilep) < 0) {
        fprintf(stderr, "Failed to initialize logging\n");
        return EXIT_FAILURE;
    }

    if (disk_open(argv[1], offset) < 0) {
        perror("disk_open");
        return EXIT_FAILURE;
    }

    argc = 2;
    argv[1] = argv[2];
    argv[2] = NULL;

    return fuse_main(argc, argv, &e4f_ops, NULL);
}   

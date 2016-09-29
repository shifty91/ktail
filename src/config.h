#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stddef.h>

struct config {
    const char *file;
    size_t n;
    int f_flag;
};

extern struct config config;

#endif /* _CONFIG_H_ */

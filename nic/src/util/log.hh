#pragma once
#include <stdio.h>
#include <assert.h>

#define debug(fmt, ...) do {                                \
    fprintf(stderr, "[DEBUG] %s:%d %s(): " fmt "\n",        \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__);   \
} while(0)

#define fatal(fmt, ...) do {                                \
    fprintf(stderr, "[FATAL] %s:%d %s(): " fmt "\n",        \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__);   \
    abort();                                                \
} while(0)

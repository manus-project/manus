/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#include <time.h>
#include <string.h>
#include <stdio.h>
#include "debug.h"

using namespace std;

long __start_time = clock();

#ifdef SOURCE_COMPILE_ROOT
const char* _source_root = SOURCE_COMPILE_ROOT;
#else
const char* _source_root = "";
#endif
const int _source_root_len = strlen(_source_root);

#ifdef ENABLE_DEBUG
int ___debug = 1;
#else
int ___debug = 0;
#endif

FILE* ___stream = stdout;

void __debug_enable() {
    ___debug = 1;
}

void __debug_disable() {
    ___debug = 0;
}

int __is_debug_enabled() {
    return ___debug;
}

void __debug_set_target(FILE* stream) {
    ___stream = stream;
}

FILE* __debug_get_target() {
    return ___stream;
}

void __debug_flush() {
    if (___stream)
        fflush(___stream);
}

void tic() {
    __start_time = clock();
}

void toc() {
    long diff = clock() - __start_time;
    DEBUGMSG(" *** Elapsed time %ldms\n", (diff) / (CLOCKS_PER_SEC / 1000));
}

const char* __short_file_name(const char* filename) {

    int position = 0;

    while (position < _source_root_len) {
    
        if (!filename[position]) 
            return filename;

        if (_source_root[position] != filename[position])
            return filename;

        position++;
    }

    return &(filename[position]);
}


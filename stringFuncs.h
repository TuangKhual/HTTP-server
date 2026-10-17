#pragma once

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    const char* data;
    size_t len;
} stringView;

typedef struct {
    stringView* splits; // array of stringViews
    size_t count;
    size_t capacity;
} stringSplit;

bool stringsEqual(const stringView* l, const stringView* r) {
    return l->len == r->len && memcmp(l->data, r->data, l->len) == 0;
}

stringView stringFromCstr(const char* str) {
    stringView s;
    s.len = strlen(str);
    s.data = str;
    return s;
}

void stringTrimSpaces(stringView* s){
    // left trim;
    while (*s->data == ' '){
        s->data += 1;
    }

    // right trim
    while (s->len > 0 && s->data[s->len - 1] == ' '){
        s->len -= 1;
    }
}

#define STRING_VIEW_FROM_LITERAL(str) \
    (stringView) { .data = str, .len = sizeof(str) }

static stringSplit splitString(const char* string, size_t len, const char* delim){
    stringSplit res;
    const char* start = string;
    size_t res_i = 0;
    size_t delimLen = strlen(delim);

    res.capacity = 8;
    res.splits = calloc(res.capacity, sizeof(stringView));
    res.count = 0;

    for (size_t i = 0; i < len; ++i){
        if (i + delimLen < len && memcmp(&string[i], delim, delimLen) == 0){
            res.splits[res_i].data = start;
            res.splits[res_i].len = &string[i] - start;
            res.count += 1;
            res_i += 1;
            start = &string[i + delimLen];
            i += delimLen;

            if (res.count == res.capacity){
                res.capacity += 2;
                stringView* temp = realloc(res.splits, sizeof(stringView) * res.capacity);
                if (temp) {
                    res.splits = temp;
                } else {
                    perror("realloc()");
                    abort();
                }
            }
        }
    }
    size_t lastLen = &string[len] - start;
    if (lastLen > 0){
        res.splits[res_i].data = start;
        res.splits[res_i].len = lastLen;
        res.count += 1;
    }
    return res;
}

static void freeSplits(stringSplit* splits){
    if (splits){
        free(splits->splits);
        splits->splits = NULL;
    }
}
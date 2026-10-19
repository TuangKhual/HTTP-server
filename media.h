#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

#include "stringFuncs.h"
#include "server.c"

#ifndef CRLF
#define CRLF
#endif

typedef struct {
    bool exists;
    bool correct;
    size_t start;
    size_t end;
} byteRange;

byteRange byteRangeInIt(){
    byteRange range;
    range.exists = false;
    range.correct = false;
    range.start = 0;
    range.end = 0;
    return range;
}

static bool stringToNum(const char* str, size_t len, size_t* out){
    size_t val = 0;
    if (len == 0){
        return false;
    }
    for (size_t i = 0; i < len; i++){
        if (str[i] > '9' || str[i] < '0'){
            return false;
        }
        val = val * 10 + (str[i] - '0');
    }
    *out = val;
    return true;
}

byteRange parseRangeVal(stringView val, size_t size){
    byteRange range = byteRangeInIt();
    range.exists = true;

    const char* wantedRange;
    size_t  wantedRangeLen = 0;
    const char* middleDash;
    const char* secondRangeStart;
    size_t firstRangeLen = 0;
    size_t secondRangeLen = 0;
    size_t start = 0;
    size_t end = 0;

    if (size == 0) {
        return range;
    }

    stringTrimSpaces(&val);

    const char* type = "bytes=";
    size_t typeLen = 6;
    if (val.len <= typeLen || memcmp(val.data, type, typeLen) != 0) {
        return range;
    }
    
    wantedRange = val.data + typeLen;
    wantedRangeLen = val.len - typeLen;

    if (memchr(wantedRange, ',', wantedRangeLen) != NULL){
        return range;
    }
    middleDash = (const char*)memchr(wantedRange, '-', wantedRangeLen);
    if (middleDash == NULL){
        return range;
    }
    firstRangeLen = middleDash - wantedRange;
    secondRangeStart = middleDash + 1;
    secondRangeLen = wantedRangeLen - firstRangeLen - 1;
    /* 3 cases: 0-500
    1:    -100       The last 100 bytes = 400-499
    2:    100-       Start at 100 until the end = 100-499
    3:    100-500    Certain Range
     */
    if (firstRangeLen == 0){
        if (!stringToNum(secondRangeStart, secondRangeLen, &start)){
            return range;
        }
        if (start == 0){
            return range;
        }
        start = (start >= size) ? 0 : size - start;
        end = size - 1;
    } else if (secondRangeLen == 0) {
        if (!stringToNum(wantedRange, firstRangeLen, &start)){
            return range;
        }
        if (secondRangeLen == 0){
            if (start >= size){
                return range;
            }
            end = size - 1;
        }
    } else {
        if (!stringToNum(wantedRange, firstRangeLen, &start)){
            return range;
        }
        if (!stringToNum(secondRangeStart, secondRangeLen, &end)){
            return range;
        }
        if (end >= size){
            end = size - 1;
        }
        if (start > end || start >= size){
            return range;
        }
    }
    range.start = start;
    range.end = end;
    range.correct = true;
    return range;
}

bool findHeaderLine(stringView line, const char* name, size_t nameLen, stringView* output){
    if (line.len < nameLen + 1){
        return false;
    }
    for (size_t i = 0; i <  nameLen; ++i){
        char c = line.data[i];
        if (c >= 'A' && c <= 'Z'){
            c = c - 'A' + 'a';
        }
        if (c != name[i]){
            return false;
        }
    }
    if (line.data[nameLen] != ':'){
        return false;
    }
    output->len = line.len - nameLen - 1;
    output->data = line.data + nameLen + 1;
    return true;
}

byteRange parseRange(const stringView* lines, size_t totalLines, size_t size){
    byteRange range = byteRangeInIt();

    if (lines == NULL){
        return range;
    }

    for (size_t i = 1; i < totalLines; ++i){
        stringView val;
        if (findHeaderLine(lines[i], "range", 5, &val)){
            return parseRangeVal(val, size);
        }
    }
    return range;
}

stringView httpResponseGenMedia(char* buffer, size_t bufferLen, httpResponse resp, size_t bodyLen, long start, long end, size_t fileSize){
    stringView response;
    response.len = 0;
    memset(buffer, 0, bufferLen);

    response.len += sprintf(buffer, "%s %d %s" CRLF, "HTTP/1.1", resp, httpResponseToString(resp));

    if (resp == HTTP_RES_PARTIAL_CONTENT){
        response.len += sprintf(buffer + response.len, "Content-Range: bytes %ld-%ld/%zu" CRLF, start, end, fileSize);
    } else if (resp == HTTP_RES_RANGE_ERR){
        response.len += sprintf(buffer + response.len, "Content-Range: bytes */%zu" CRLF, fileSize);
    }
    response.len += sprintf(buffer + response.len, "Content-Length: %zu" CRLF, bodyLen);
    response.len += sprintf(buffer + response.len, "Accept-Ranges: bytes" CRLF);
    response.len += sprintf(buffer + response.len, CRLF);
    response.data = buffer;
    return response;
}


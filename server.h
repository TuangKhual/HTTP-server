#pragma once

#include "stringFuncs.h"

#define CRLF "\r\n"

typedef enum httpResponse {
    HTTP_RES_OK = 200,
    HTTP_RES_PARTIAL_CONTENT = 206,
    HTTP_RES_BAD_REQUEST = 400,
    HTTP_RES_NOT_FOUND = 404,
    HTTP_RES_RANGE_ERR = 416,
    HTTP_RES_INTERNAL_SERVER_ERR = 500,
} httpResponse;

const char* httpResponseToString(httpResponse res);
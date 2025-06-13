#include <stdio.h>
#include "http.h"
int http_get(const char *url) {
    printf("[HTTP] GET request to: %s (placeholder)\n", url);
    return 200;
}

#include <stdio.h>
#include <stdlib.h>

void project_fatal(const char *msg)
{
    fprintf(stderr, "[unit] project_fatal: %s\n", msg ? msg : "(null)");
    abort();
}


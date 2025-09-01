#include <stdio.h>
#include <stdlib.h>

void grlc_assert_fatal(const char *msg)
{
    fprintf(stderr, "[unit] project_fatal: %s\n", msg ? msg : "(null)");
    abort();
}

void project_fatal(const char *msg)
{
    grlc_assert_fatal(msg);
}

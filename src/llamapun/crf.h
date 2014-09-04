#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <crfsuite.h>

#define    SAFE_RELEASE(obj)    if ((obj) != NULL) { (obj)->release(obj); (obj) = NULL; }
#define    MAX(a, b)    ((a) < (b) ? (b) : (a))


typedef struct {
    char *type;
    char *algorithm;
    char *model;
    char *logbase;

    int split;
    int cross_validation;
    int holdout;
    int logfile;

    int help;
    int help_params;

    int num_params;
    char **params;
} learn_option_t;

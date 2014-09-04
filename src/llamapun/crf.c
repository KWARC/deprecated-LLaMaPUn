/*
 *        Learn command for CRFsuite frontend.
 *
 * Copyright (c) 2007-2010, Naoaki Okazaki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the names of the authors nor the names of its contributors
 *       may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* API adaptation for LLaMaPUn (from crfsuite's learn.c) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <crfsuite.h>
#include "crf.h"

static char* mystrdup(const char *src)
{
    char *dst = (char*)malloc(strlen(src)+1);
    if (dst != NULL) {
        strcpy(dst, src);
    }
    return dst;
}

static char* mystrcat(char *dst, const char *src)
{
    int n = (dst != 0 ? strlen(dst) : 0);
    dst = (char*)realloc(dst, n + strlen(src) + 1);
    strcat(dst, src);
    return dst;
}

static void learn_option_init(learn_option_t* opt)
{
    memset(opt, 0, sizeof(*opt));
    opt->num_params = 0;
    opt->holdout = -1;
    opt->type =   ("crf1d");
    opt->algorithm = mystrdup("lbfgs");
    opt->model = mystrdup("");
    opt->logbase = mystrdup("log.crfsuite");
}

static void learn_option_finish(learn_option_t* opt)
{
    int i;

    free(opt->model);

    for (i = 0;i < opt->num_params;++i) {
        free(opt->params[i]);
    }
    free(opt->params);
}
static int message_callback(void *instance, const char *format, va_list args)
{
    vfprintf(stdout, format, args);
    fflush(stdout);
    return 0;
}

int learn(learn_option_t opt_choice)
{
    int i, groups = 1, ret = 0;
    time_t ts;
    char timestamp[80];
    char trainer_id[128];
    //clock_t clk_begin, clk_current;
    learn_option_t opt;
    FILE *fpo = stdout, *fpe = stderr;
    crfsuite_data_t data;
    crfsuite_trainer_t *trainer = NULL;
    //crfsuite_dictionary_t *attrs = NULL, *labels = NULL;

    /* Initializations. */
    learn_option_init(&opt);
    crfsuite_data_init(&data);

    /* Parse the command-line option. */

    /* Open a log file if necessary. */
    if (opt.logfile) {
        /* Generate a filename for the log file. */
        char *fname = NULL;
        fname = mystrcat(fname, opt.logbase);
        fname = mystrcat(fname, "_");
        fname = mystrcat(fname, opt.algorithm);
        for (i = 0;i < opt.num_params;++i) {
            fname = mystrcat(fname, "_");
            fname = mystrcat(fname, opt.params[i]);
        }

        fpo = fopen(fname, "w");
        if (fpo == NULL) {
            fprintf(fpe, "ERROR: Failed to open the log file.\n");
            ret = 1;
            goto force_exit;
        }
    }

    /* Create dictionaries for attributes and labels. */
    ret = crfsuite_create_instance("dictionary", (void**)&data.attrs);
    if (!ret) {
        fprintf(fpe, "ERROR: Failed to create a dictionary instance.\n");
        ret = 1;
        goto force_exit;
    }
    ret = crfsuite_create_instance("dictionary", (void**)&data.labels);
    if (!ret) {
        fprintf(fpe, "ERROR: Failed to create a dictionary instance.\n");
        ret = 1;
        goto force_exit;
    }

    /* Create a trainer instance. */
    sprintf(trainer_id, "train/%s/%s", opt.type, opt.algorithm);
    ret = crfsuite_create_instance(trainer_id, (void**)&trainer);
    if (!ret) {
        fprintf(fpe, "ERROR: Failed to create a trainer instance.\n");
        ret = 1;
        goto force_exit;
    }

    /* Show the help message for the training algorithm if specified. */
    if (opt.help_params) {
        crfsuite_params_t* params = trainer->params(trainer);

        fprintf(fpo, "PARAMETERS for %s (%s):\n", opt.algorithm, opt.type);
        fprintf(fpo, "\n");

        for (i = 0;i < params->num(params);++i) {
            char *name = NULL;
            char *type = NULL;
            char *value = NULL;
            char *help = NULL;

            params->name(params, i, &name);
            params->get(params, name, &value);
            params->help(params, name, &type, &help);

            fprintf(fpo, "%s %s = %s;\n", type, name, value);
            fprintf(fpo, "%s\n", help);
            fprintf(fpo, "\n");

            params->free(params, help);
            params->free(params, type);
            params->free(params, value);
            params->free(params, name);
        }

        params->release(params);
        goto force_exit;
    }

    /* Set parameters. */
    for (i = 0;i < opt.num_params;++i) {
        char *value = NULL;
        char *name = opt.params[i];
        crfsuite_params_t* params = trainer->params(trainer);

        /* Split the parameter argument by the first '=' character. */
        value = strchr(name, '=');
        if (value != NULL) {
            *value++ = 0;
        }

        if (params->set(params, name, value) != 0) {
            fprintf(fpe, "ERROR: paraneter not found: %s\n", name);
            goto force_exit;
        }
        params->release(params);
    }

    /* Log the start time. */
    time(&ts);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&ts));
    fprintf(fpo, "Start time of the training: %s\n", timestamp);
    fprintf(fpo, "\n");

    /* Split into data sets if necessary. */
    if (0 < opt.split) {
        /* Shuffle the instances. */
        for (i = 0;i < data.num_instances;++i) {
            int j = rand() % data.num_instances;
            crfsuite_instance_swap(&data.instances[i], &data.instances[j]);
        }

        /* Assign group numbers. */
        for (i = 0;i < data.num_instances;++i) {
            data.instances[i].group = i % opt.split;
        }
        groups = opt.split;
    }

    /* Report the statistics of the training data. */
    fprintf(fpo, "Statistics the data set(s)\n");
    fprintf(fpo, "Number of data sets (groups): %d\n", groups);
    fprintf(fpo, "Number of instances: %d\n", data.num_instances);
    fprintf(fpo, "Number of items: %d\n", crfsuite_data_totalitems(&data));
    fprintf(fpo, "Number of attributes: %d\n", data.attrs->num(data.attrs));
    fprintf(fpo, "Number of labels: %d\n", data.labels->num(data.labels));
    fprintf(fpo, "\n");
    fflush(fpo);

    /* Set callback procedures that receive messages and taggers. */
    trainer->set_message_callback(trainer, NULL, message_callback);

    /* Start training. */
    if (opt.cross_validation) {
        for (i = 0;i < groups;++i) {
            fprintf(fpo, "===== Cross validation (%d/%d) =====\n", i+1, groups);
            if ((ret = trainer->train(trainer, &data, "", i))) {
                goto force_exit;
            }
            fprintf(fpo, "\n");
        }

    } else {
        if ((ret = trainer->train(trainer, &data, opt.model, opt.holdout))) {
            goto force_exit;
        }

    }

    /* Log the end time. */
    time(&ts);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&ts));
    fprintf(fpo, "End time of the training: %s\n", timestamp);
    fprintf(fpo, "\n");

force_exit:
    SAFE_RELEASE(trainer);
    SAFE_RELEASE(data.labels);
    SAFE_RELEASE(data.attrs);

    crfsuite_data_finish(&data);
    learn_option_finish(&opt);
    if (fpo != NULL) {
        fclose(fpo);
    }

    return ret;
}

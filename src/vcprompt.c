/*
 * Copyright (C) 2009-2013, Gregory P. Ward and contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "../config.h"

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "cvs.h"
#include "fossil.h"
#include "git.h"
#include "hg.h"
#include "svn.h"
/*
#include "bzr.h"
*/

static char* features[] = {
    /* Some version control systems don't change their working copy
       format every couple of versions (or they are just
       unmaintained), so we don't need versioned feature strings. */
    "cvs",
    "hg",
    "git",
    "fossil",

    /* Support for Subversion up to 1.6 is unconditional, because those
       versions don't require any additional libraries. Subversion >= 1.7
       requires SQLite, so it's conditional. */
    "svn-1.3",
    "svn-1.4",
    "svn-1.5",
    "svn-1.6",
#if HAVE_SQLITE3
    "svn-1.7",
    "svn-1.8",
#endif
    0,
};

#define DEFAULT_FORMAT "[%n:%b] "

void
parse_args(int argc, char** argv, options_t* options)
{
    int opt;
    while ((opt = getopt(argc, argv, "hvf:dt:F")) != -1) {
        switch (opt) {
        case 'f':
            options->format = strdup(optarg);
            break;
        case 'd':
            options->debug = 1;
            break;
        case 't':
            options->timeout = strtol(optarg, NULL, 10);
            break;
        case 'F':
            options->show_features = 1;
            break;
        case 'v':
            puts(PACKAGE_STRING);
            exit(0);
            break;
        case 'h':
        default:
            fprintf(stderr, "usage: %s [-h] [-d] [-t timeout_ms] [-f FORMAT] <DIRECTORY>\n%s",
                    argv[0],
                    "FLAGS:\n"
                    "  -h  show this help message and exit\n"
                    "  -v  show program version\n"
                    "  -d  output debug messages to console\n"
                    "  -F  show vcs features installed on this system\n"
                    "ARGUMENTS:\n"
                    "  -t  timeout threshold, in milliseconds\n"
                    "  -f  tokenized string that determines output\n"
                    "      %n  show VC name\n"
                    "      %b  show branch\n"
                    "      %r  show revision\n"
                    "      %p  show patch name (MQ, guilt, ...)\n"
                    "      %u  indicate unknown (untracked) files\n"
                    "      %m  indicate uncommitted changes (modified/added/removed)\n"
                    "      %%  show '%'"
                    "\nENVIRONMENT:\n"
                    "  $VCPROMPT_FORMAT  variable containing FORMAT string");
            fprintf(stderr, " (default=\"%s\")\n", DEFAULT_FORMAT);
            exit(1);
        }
    }
    if (argv[optind] != NULL) options->directory = argv[optind];
}

void
show_features(void)
{
    for (char** f = features; *f != NULL; ++f) {
        puts(*f);
    }
}

void
parse_format(options_t* options)
{
    options->show_branch = 0;
    options->show_revision = 0;
    options->show_patch = 0;
    options->show_unknown = 0;
    options->show_modified = 0;

    char* format = options->format;
    for (size_t i = 0; format[i] != '\0'; ++i) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
            case 'n': /* name of VC system */
                break;
            case 'b':
                options->show_branch = 1;
                break;
            case 'r':
                options->show_revision = 1;
                break;
            case 'p':
                options->show_patch = 1;
                break;
            case 'u':
                options->show_unknown = 1;
                break;
            case 'm':
                options->show_modified = 1;
                break;
            case '%':
                break;
            default:
                fprintf(stderr, "error: invalid format string: %%%c\n", format[i]);
                exit(1);
            }
        }
    }
}

void
print_result(vccontext_t* context, options_t* options, result_t* result)
{
    char* format = options->format;

    for (size_t i = 0; format[i] != '\0'; ++i) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
            case 'n':
                fputs(context->name, stdout);
                break;
            case 'b':
                if (result->branch) fputs(result->branch, stdout);
                break;
            case 'r':
                if (result->revision) fputs(result->revision, stdout);
                break;
            case 'p':
                if (result->patch) fputs(result->patch, stdout);
            case 'u':
                if (result->unknown) putc('?', stdout);
                break;
            case 'm':
                if (result->modified) putc('*', stdout);
                break;
            case '%': /* escaped % */
                putc('%', stdout);
                break;
            default:
                putc(format[i], stdout);
            }
        } else {
            putc(format[i], stdout);
        }
    }
}

vccontext_t*
probe_all(vccontext_t** contexts, int num_contexts)
{
    for (int i = 0; i < num_contexts; ++i) {
        vccontext_t* ctx = contexts[i];
        if (ctx->probe(ctx)) {
            return ctx;
        }
    }
    return NULL;
}

/* walk up the directory tree until the probes work or we hit / */
vccontext_t*
probe_dirs(vccontext_t** contexts, int num_contexts)
{
    char* start_dir = malloc(PATH_MAX);
    if (getcwd(start_dir, PATH_MAX) == NULL) {
        debug("getcwd() failed: %s", strerror(errno));
        free(start_dir);
        return NULL;
    }
    char* rel_path = start_dir + strlen(start_dir);

    vccontext_t* context = NULL;
    while (1) {
        context = probe_all(contexts, num_contexts);
        if (context) {
            break;
        }
        if (rel_path == start_dir + 1) {
            debug("reached the root: %s not under version control", start_dir);
            break;
        }

        debug("no context claimed current dir: walking up the tree");
        if (-1 == chdir("..")) {
            debug("chdir(\"..\") failed: %s", strerror(errno));
            break;
        }
        do {
            rel_path--;
        } while (rel_path > start_dir && rel_path[-1] != '/');
    }
    if (context) {
        debug("found a context: %s (rel_path=%s)", context->name, rel_path);
        context->rel_path = strdup(rel_path);
    }
    free(start_dir);
    return context;
}

/* The signal handler just clears the flag and re-enables itself.  */
void
exit_on_alarm(int sig)
{
    debug("exit signal received: %d", sig);
    printf("[timeout]");
    exit(1);
}

unsigned int
set_alarm(unsigned int milliseconds)
{
    struct itimerval old, new;
    new.it_interval.tv_usec = 0;
    new.it_interval.tv_sec = 0;
    new.it_value.tv_usec = 1000 * (long int)milliseconds;
    new.it_value.tv_sec = 0;
    if (setitimer(ITIMER_REAL, &new, &old) < 0)
        return 0;
    else
        return old.it_value.tv_sec;
}

int
main(int argc, char** argv)
{
    /* Establish a handler for SIGALRM signals.  */
    signal(SIGALRM, exit_on_alarm);

    options_t options = {
        .debug = 0,
        .format = NULL,
        .directory = NULL,
        .show_branch = 0,
        .show_revision = 0,
        .show_unknown = 0,
        .show_modified = 0,
        .show_features = 0,
    };

    parse_args(argc, argv, &options);
    if (options.show_features) {
        show_features();
        if (options.format) {
            free(options.format);
        }
        return 0;
    }
    if (!options.format) {
        char* format = getenv("VCPROMPT_FORMAT");
        if (!format) {
            format = DEFAULT_FORMAT;
        }
        options.format = strdup(format);
    }
    if (options.directory) {
        if (-1 == chdir(options.directory)) {
            debug("chdir to \"%s\" failed: %s", options.directory, strerror(errno));
        }
    }

    parse_format(&options);
    set_options(&options);

    if (options.timeout) {
        debug("will timeout after %d ms", options.timeout);
        set_alarm(options.timeout);
    } else {
        debug("will never timeout");
    }

    vccontext_t* contexts[] = {
        /* ordered by popularity, so the common case is fast */
        get_git_context(&options), get_hg_context(&options),     get_svn_context(&options),
        get_cvs_context(&options), get_fossil_context(&options),
    };
    int num_contexts = sizeof(contexts) / sizeof(vccontext_t*);

    result_t* result = NULL;
    vccontext_t* context = NULL;

    /* Starting in the current dir, walk up the directory tree until
       someone claims that this is a working copy. */
    context = probe_dirs(contexts, num_contexts);

    /* Nobody claimed it: bail now without printing anything. */
    if (context == NULL) {
        goto done;
    }

    /* Analyze the working copy metadata and print the result. */
    result = context->get_info(context);
    if (result != NULL) {
        print_result(context, &options, result);
        free_result(result);
        if (options.debug) putc('\n', stdout);
    }

done:
    for (int i = 0; i < num_contexts; ++i) {
        free_context(contexts[i]);
    }
    if (options.format != NULL) {
        free(options.format);
    }
}

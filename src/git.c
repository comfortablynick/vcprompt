/*
 * Copyright (C) 2009-2013, Gregory P. Ward and contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>

#include "capture.h"
#include "common.h"
#include "git.h"

static int git_probe() { return isdir(".git"); }

static result_t *git_get_info(vccontext_t *context)
{
    result_t *result = init_result();
    char buf[1024];

    if (!read_first_line(".git/HEAD", buf, 1024)) {
        debug("unable to read .git/HEAD: assuming not a git repo");
        goto err;
    }

    char *prefix = "ref: refs/heads/";
    int prefixlen = strlen(prefix);

    if (context->options->show_branch || context->options->show_revision) {
        int found_branch = 0;
        if (strncmp(prefix, buf, prefixlen) == 0) {
            /* yep, we're on a known branch */
            debug("read a head ref from .git/HEAD: '%s'", buf);
            if (result_set_branch(result, buf + prefixlen)) found_branch = 1;
        } else {
            /* if it's not a branch name, assume it is a commit ID */
            debug(".git/HEAD doesn't look like a head ref: unknown branch");
            result_set_branch(result, "(unknown)");
            result_set_revision(result, buf, 12);
        }
        if (context->options->show_revision && found_branch) {
            char buf[1024];
            char filename[1024] = ".git/refs/heads/";
            int nchars = sizeof(filename) - strlen(filename) - 1;
            strncat(filename, result->branch, nchars);
            if (read_first_line(filename, buf, 1024)) {
                result_set_revision(result, buf, 12);
            }
        }
    }

    if (!context->options->show_modified && !context->options->show_unknown) return result;

    char *argv[] = {"git", "status", "--porcelain", "--untracked-files=normal", NULL};
    if (!context->options->show_unknown) {
        // asking git to search for unknown files can be expensive, so
        // skip it unless the user wants it
        argv[3] = "--untracked-files=no";
    }
    capture_t *capture = capture_child("git", argv);
    if (!capture) {
        debug("unable to execute 'git status'");
        goto err;
    }
    char *cstdout = capture->childout.buf;
    for (char *ch = cstdout; *ch; ++ch) {
        if (ch == cstdout || *(ch - 1) == '\n') {
            // at start of output or start of line: look for ?, M, etc.
            if (context->options->show_unknown && *ch == '?') {
                result->unknown = 1;
            } else if (context->options->show_modified && *(ch + 1) != ' ') {
                result->modified = 1;
            }
        }
    }

    cstdout = NULL;
    free_capture(capture);
    return result;

err:
    free_result(result);
    return NULL;
}

vccontext_t *get_git_context(options_t *options)
{
    return init_context("git", options, git_probe, git_get_info);
}

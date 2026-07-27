/* casegen — case converter, and eventually a template renderer.

   One translation unit. The sections below live in separate files purely for
   navigation; the preprocessor pastes them in here, in dependency order, so
   the compiler still sees a single file. That means no headers to maintain and
   every function stays `static`. Split into real modules with headers only if
   something ever needs to link against one of them on its own. */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stringlist.c"
#include "ascii.c"
#include "split.c"
#include "cases.c"
#include "input.c"
#include "args.c"


int main(const int argc, char *argv[])
{
    Options opt = {0};
    parseArgs(argc, argv, &opt);
    validateArgs(&opt);

    if (!opt.spec)
    {
        fprintf(stderr, "casegen: template mode is not implemented yet\n");
        fprintf(stderr, "casegen: use -c CASE to render input lines (see --help)\n");
        exit(1);
    }

    StringList lines = {0};
    for (size_t i = 0; i < opt.sources.count; i++)
        loadSource(opt.sources.item[i], &lines);

    /* One input line = one output line, so a line with no words prints blank. */
    for (size_t i = 0; i < lines.count; i++)
    {
        StringList words = {0};
        splitWords(lines.item[i], &words);
        renderCase(&words, opt.spec, stdout);
        fputc('\n', stdout);
        stringsFree(&words);
    }

    stringsFree(&lines);
    stringsFree(&opt.sources);
    return 0;
}

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
#include "template.c"
#include "args.c"


int main(const int argc, char *argv[])
{
    Options opt = {0};
    parseArgs(argc, argv, &opt);
    validateArgs(&opt);

    /* Both modes read the same assembled input; only what happens to it differs. */
    StringList lines = {0};
    SourceMap map = {0};
    for (size_t i = 0; i < opt.sources.count; i++)
        loadSource(opt.sources.item[i], &lines, &map);

    if (opt.spec)
    {
        /* One input line = one output line, so a line with no words prints blank. */
        for (size_t i = 0; i < lines.count; i++)
        {
            StringList words = {0};
            splitWords(lines.item[i], &words);
            renderCase(&words, opt.spec, stdout);
            fputc('\n', stdout);
            stringsFree(&words);
        }
    }
    else
        renderTemplate(&lines, &map, opt.quiet);

    spansFree(&map);
    stringsFree(&lines);
    stringsFree(&opt.sources);
    return 0;
}

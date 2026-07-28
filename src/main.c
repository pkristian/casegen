/* casegen — case converter and template renderer.

   One module per concern, each with a header declaring only what other modules may
   call; everything else in a .c is static. The dependencies run one way:

       main    -> args, input, split, cases, template
       args    -> cases, stringlist
       template-> input, split, cases, stringlist
       input   -> stringlist
       split   -> ascii, stringlist
       cases   -> ascii, stringlist
       *       -> mem

   mem is at the bottom because allocation failure is the one error every module can
   hit and none can handle. Nothing includes main. */

#include <stdio.h>

#include "args.h"
#include "cases.h"
#include "input.h"
#include "split.h"
#include "stringlist.h"
#include "template.h"


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

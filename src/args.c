/* Command line: flags, operands, and the usage text. */

#include "args.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cases.h"
#include "stringlist.h"


static void usage(FILE *out)
{
    fputs("Usage: casegen [-i] [-q] [-c CASE] [FILE|-]...\n"
          "\n"
          "Input:\n"
          "  FILE...          read in order; \"-\" is stdin at that position\n"
          "  -i               stdin only; error if any FILE is given\n"
          "  (none)           this message, exit 2 — stdin is never read implicitly\n"
          "\n"
          "Mode:\n"
          "  -c, --case=CASE  render every input line in CASE\n"
          "  (no -c)          template mode: any line carrying \"casegen:\" is a\n"
          "                   directive, and a placeholder written in some case\n"
          "                   comes back in that same case\n"
          "\n"
          "Other:\n"
          "  -q, --quiet      suppress warnings\n"
          "  -h, --help       this message on stdout, exit 0\n"
          "  --               end of options\n"
          "\n"
          "Cases:\n", out);

    StringList sample = {0};
    stringsPush(&sample, "casegen");
    stringsPush(&sample, "case");

    for (size_t i = 0; i < CASE_COUNT; i++)
    {
        if (CASES[i].isAlias)
            continue;
        if (CASES[i].code)
            fprintf(out, "  -%c  %-18s", CASES[i].code, CASES[i].name);
        else
            fprintf(out, "      %-18s", CASES[i].name);
        renderCase(&sample, &CASES[i], out);
        fputc('\n', out);
    }

    stringsFree(&sample);
}


static _Noreturn void usageError(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fputs("casegen: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);

    usage(stderr);
    exit(2);
}


void parseArgs(const int argc, char *argv[], Options *opt)
{
    int endOfFlags = 0;

    for (int i = 1; i < argc; i++)
    {
        char *a = argv[i];

        if (!endOfFlags && strcmp(a, "--") == 0)
        {
            endOfFlags = 1;
            continue;
        }

        /* "-" on its own is an operand, not a flag */
        if (endOfFlags || a[0] != '-' || a[1] == '\0')
        {
            stringsPush(&opt->sources, a);
            continue;
        }

        if (a[1] == '-') /* long option */
        {
            char *name = a + 2;
            char *eq = strchr(name, '=');
            char *value = NULL;

            if (eq)
            {
                *eq = '\0';
                value = eq + 1;
            }

            if (strcmp(name, "case") == 0)
            {
                if (!value)
                {
                    if (i + 1 >= argc)
                        usageError("--case requires a case name");
                    value = argv[++i];
                }
                opt->spec = findCase(value);
                if (!opt->spec)
                    usageError("unknown case \"%s\"", value);
            }
            else if (strcmp(name, "quiet") == 0)
                opt->quiet = 1;
            else if (strcmp(name, "help") == 0)
            {
                usage(stdout);
                exit(0);
            }
            else
                usageError("unknown option \"--%s\"", name);

            continue;
        }

        /* short bundle: -i, -q, -iq, -cs, -ic snake, -ics */
        int valueTaken = 0;
        for (const char *p = a + 1; *p != '\0' && !valueTaken; p++)
        {
            switch (*p)
            {
                case 'i':
                    opt->stdinOnly = 1;
                    break;
                case 'q':
                    opt->quiet = 1;
                    break;
                case 'h':
                    usage(stdout);
                    exit(0);
                case 'c':
                {
                    const char *value = p + 1; /* rest of the token, if any */
                    if (*value == '\0')
                    {
                        if (i + 1 >= argc)
                            usageError("-c requires a case");
                        value = argv[++i];
                    }
                    opt->spec = findCase(value);
                    if (!opt->spec)
                        usageError("unknown case \"%s\"", value);
                    valueTaken = 1;
                    break;
                }
                default:
                    usageError("unknown option \"-%c\"", *p);
            }
        }
    }
}


/* -i and file operands are mutually exclusive on purpose: the point of -i is that
   the whole input is the pipe. Once files are in play the user has to say where
   the stream belongs, and "-" is how they say it. */
void validateArgs(Options *opt)
{
    if (opt->stdinOnly && opt->sources.count > 0)
    {
        fprintf(stderr, "casegen: -i means stdin only, but %zu file(s) were given\n",
                opt->sources.count);
        fprintf(stderr, "casegen: use \"-\" to say where the stream belongs, e.g.\n");
        fprintf(stderr, "casegen:     casegen -c snake %s -\n", opt->sources.item[0]);
        exit(2);
    }

    if (opt->stdinOnly)
        stringsPush(&opt->sources, "-");

    if (opt->sources.count == 0)
        usageError("no input given");

    size_t stdinCount = 0;
    for (size_t i = 0; i < opt->sources.count; i++)
        if (strcmp(opt->sources.item[i], "-") == 0)
            stdinCount++;

    if (stdinCount > 1)
        usageError("\"-\" given %zu times; stdin can only be read once", stdinCount);
}

#include "parse-util.h"
#include "wm-parse.tab.h"

#include <stdio.h>
#include <errno.h>

extern int yydebug;
extern FILE *yyin;

int
main (int argc, char *argv[])
{
  if (argc == 2) {
     /*
       printf ("Reading from file `%s'.\n", argv[1]);
      */
    errno = 0;
    yyin = fopen (argv[1], "r");
    if (yyin == NULL) {
      perror ("opening input file");
      exit (1);
    }
  } else {
     printf ("Let's start!\n");
  }
  /*  yydebug = 0;*/
  yyparse ();
  
  print_symbols();
}

/* -*-Mode: text;-*-
   lexer fuer Waldmeister
   Geschrieben von Andreas Jaeger
*/
%{
#include "parse-util.h"
#include "wm-parse.tab.h"
#include <stdio.h>
#include <string.h>
int yywrap (void)
{
  return 1;
}
int lineno = 1;
int column = 0;
void yyerror (char *s);
%}

DIGIT	[0-9]
ALPHA   [A-Za-z0-9+|*/\-~\\_^$]
%%

NAME		{ column += yyleng; return NAME; }
SORTS?          { column += yyleng; return SORT; }
MODE		{ column += yyleng; return MODE; }
SIGNATURE       { column += yyleng; return SIGNATURE; }
ORDERING        { column += yyleng; return ORDERING; }
VARIABLES?      { column += yyleng; return VARIABLE; }
EQUATIONS?	{ column += yyleng; return EQUATION; }
CONCLUSIONS?	{ column += yyleng; return CONCLUSION; }
ADD             { column += yyleng; return ADD; }
DELETE          { column += yyleng; return DELETE; }
TEST            { column += yyleng; return TEST; }

COMPLETION      { column += yyleng; return COMPLETION; }
CONFLUENCE      { column += yyleng; return CONFLUENCE; }
CONVERGENCE     { column += yyleng; return CONVERGENCE; }
PROOF           { column += yyleng; return PROOF; }
REDUCTION       { column += yyleng; return REDUCTION; }
TERMINATION     { column += yyleng; return TERMINATION; }
CCE             { column += yyleng; return CCE; }
ACG             { column += yyleng; return ACG; }

LPO             { column += yyleng; return LPO; }
KBO             { column += yyleng; return KBO; }

"("|")"|","|">"|":"	{ column += yyleng; return *yytext; }
"="		{ column += yyleng; return EQ; }
"->"		{ column += yyleng; return ARROW; }

{DIGIT}+        { column += yyleng;
                  if (noExpected()){
                    sscanf(yytext, "%d", &(yylval.value));
                    return NUMBER;
                  }
                  else {
                    yylval.ident = strdup (yytext);
		    return IDENT;
                  }
                }
{ALPHA}+        {
                  yylval.ident = strdup (yytext);
		  column += yyleng;
                  return IDENT;
                }

[%#]([^\n])*	{ /* ignore Comment until end of line */
		  column += yyleng;
		}
[ \t]+		{ /* ignore space/tabs */
		  column += yyleng;
		}
\n		{ lineno++;
		  column = 0;
                }
.		{ fprintf (stderr, "Error in lexer: %c ignored\n", *yytext);}


%%
void
yyerror (char *s)
{
  fprintf (stderr, "\aError: %s in line %d, column %d: at `%s'\n", 
	   s ? s : "", lineno, column, yytext);
}

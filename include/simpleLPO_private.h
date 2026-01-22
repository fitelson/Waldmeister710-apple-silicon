#ifndef SIMPLE_LPO_PRIVATE
#define SIMPLE_LPO_PRIVATE

/* Alles, was die verschiedenen Versionen gemeinsam brauchen. */
#include "simpleLPO.h"
#include "Praezedenz.h"
#include "SymbolOperationen.h"
#include "Ausgaben.h"

extern CounterT S_counter;

/* lokale Fassungen der Funktionen aus TermOperationen: 
 * a) evtl. Inlining, da static
 * b) eigentlicher Grund: Zaehlen ermoeglichen 
 */

#if 1
static BOOLEAN sTO_TermeGleich(UTermT Term1, UTermT Term2)
/* Prueft, ob UTerme Term1, Term2 denselben Praefix haben. Der Vergleich erfolgt von links nach rechts Symbol fuer
   Symbol. Abbruch, falls unterschiedliche Symbole oder Term1 abgearbeitet. Dass Term2 abgearbeitet ist und die-
   selben Symbole hat wie Term1 und Term1 noch nicht abgearbeitet ist, kommt wegen der Termstruktur nicht vor.     */
{
#if S_doCounting
  unsigned counter = 0;
#endif  
  TermzellenZeigerT Ende1;
  Ende1 = TO_NULLTerm(Term1);
  while ((SO_SymbGleich(TO_TopSymbol(Term1),TO_TopSymbol(Term2)))) {
#if S_doCounting
    counter++;
#endif  
    if (TO_PhysischGleich(Term1 = TO_Schwanz(Term1),Ende1)){
#if S_doCounting
      S_counter += counter - 1;
#endif  
      return TRUE;
    }
    Term2 = TO_Schwanz(Term2);
  }
  return FALSE;
}

static BOOLEAN sTO_SymbolEnthalten(SymbolT f, UTermT t)
{ /* Ist die f-Laenge von t positiv? */
  SymbolT g;
  TO_doSymbole(t, g, {
    if (SO_SymbGleich(f,g))
      return TRUE;
    if (S_doCounting) {S_counter++;}
  });
  return FALSE;
}

#else

static BOOLEAN sTO_TermeGleich(UTermT Term1, UTermT Term2)
{
  return TO_TermeGleich(Term1, Term2);
}

static BOOLEAN sTO_SymbolEnthalten(SymbolT f, UTermT t)
{ 
  return TO_SymbolEnthalten(f, t);
}


#endif

/* Das TRACE schaltet sich ein, wenn sich die Ergebnisse von LP_LPO und S_LPO in Ordnungen.c unterscheiden. */
/* wenn welcheLPO == 2 */
/* Dies wird ueber die externe Variable ordtracing erreicht. */

extern BOOLEAN ordtracing;
#define INDENT_TRACE      1 /* 0, 1 */

/* -------------------------------------------------------------------------- */
/* - Tracing Infrastructure ------ BEGINN ----------------------------------- */
/* -------------------------------------------------------------------------- */

#if S_doTRACE
static unsigned int tracelevel = 0;

static void traceindent (int level)
{
  if (INDENT_TRACE){
    int i;
    for (i=0; i < level; i++){
      fputs("| ",stdout);
    }
  }
  else {
    printf("%3d:", level);
  }
}
#endif
#if S_doTRACE == 1
#define OPENTRACE(x)  {if(ordtracing){traceindent(tracelevel++);IO_DruckeFlex x;}}
#define CLOSETRACE(x) {if(ordtracing){traceindent(--tracelevel);IO_DruckeFlex x;}}
#define RESTRACE(x)   {if(ordtracing){traceindent(tracelevel);IO_DruckeFlex x;}}
#else
#define OPENTRACE(x)  /* nix */
#define CLOSETRACE(x) /* nix */
#define RESTRACE(x)   /* nix */
#endif

/* -------------------------------------------------------------------------- */
/* - Tracing Infrastructure ----- ENDE -------------------------------------- */
/* -------------------------------------------------------------------------- */


#endif


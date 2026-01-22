/*
Dieses Modul enthaelt die Ausgabe-Funktionen sowie alle 'Bezeichner' der eingelesenen Spezifikation.

28.02.94 Angelegt.
02.03.94 DruckeKopf und DruckeStatistik eingebaut.

*/
/*
  ToDo:
  Anpassung an englische Gepflogenheiten: Decimalpunkt, Tausenderkomma?
*/
/* -------------------------------------------------------*/
/* ------------- Includes --------------------------------*/
/* -------------------------------------------------------*/
#include "antiwarnings.h"
#include <signal.h> 
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "Ausgaben.h"
#include "compiler-extra.h"
#include "Constraints.h"
#include "FehlerBehandlung.h"
#include "general.h"
#include "Grundzusammenfuehrung.h"
#include "Id.h"
#include "Position.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "TermIteration.h"

#ifdef CCE_Source
#include "parse-util.h"
#else
#include "version.h"
#include "WaldmeisterII.h"
#include "Zielverwaltung.h"
#include "Parameter.h"
#include "parser.h"
#include "PhilMarlow.h"
#include "SpezNormierung.h"
#endif

TI_IterationDeklarieren(IO)

unsigned long int IO_AusdehnungAusgabestapel(void)
{
   return IO_AusdehnungKopierstapel();
}

/* ------------------------------------------------------------------------*/
/* -------------------- globale Variablen (siehe Ausgaben.h) --------------*/
/* ------------------------------------------------------------------------*/

unsigned int IO_level;
BOOLEAN IO_GUeberschrift;

/* -------------------------------------------------------*/
/* ------------- Globals ---------------------------------*/
/* -------------------------------------------------------*/

static char *Spezifikationsname = NULL;
static char **IO_SymbolNamen;
static char IO_SpezName[MaximaleBezeichnerLaenge];
static char IO_Modus[MaximaleBezeichnerLaenge];
static int Ausgabelevel;

#define IO_KOMBINATORAUSGABE 0


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* +++++++++++++ Initialisierung +++++++++++++++++++++++++*/
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

void IO_InitApriori(void)
/* Vor der ersten Ausgabe auf aufzurufen!!!! */
{
#ifndef CCE_Source
  if (WM_ErsterAnlauf())
  /* Line buffering benutzen */
#if !COMP_WM
  if (setvbuf (stdout, NULL, _IOLBF, 0) != 0)
    perror ("setvbuf");
#endif
#else
  if (setvbuf (stdout, NULL, _IONBF, 0) != 0)
    perror ("setvbuf");
#endif
}

#ifndef CCE_Source
static void ParamodSymbolnamenEtablieren(SymbolT ParamodSymbol, const char *Name)
{
  unsigned int n = 0;
  IO_SymbolNamen[ParamodSymbol] = our_alloc(MaximaleBezeichnerLaenge*sizeof(char *));
  while (TRUE) {
    SymbolT i;
    BOOLEAN ok = TRUE;
    if (n)
      sprintf(IO_SymbolNamen[ParamodSymbol],"%s%d",Name,n);
    else
      sprintf(IO_SymbolNamen[ParamodSymbol],"%s",Name);
    SO_forSpezSymbole(i)
      if (strcmp(IO_SymbolNamen[i],IO_SymbolNamen[ParamodSymbol])==0) 
        ok = FALSE;
    if (ok)
      return;
    n++;
  }
}
#endif
void IO_CleanUp(void)
{
  SymbolT i;

  SO_forFktSymbole(i)
    our_dealloc(IO_SymbolNamen[i]);
  our_dealloc(IO_SymbolNamen);

}

void IO_InitAposteriori(int level)
/* Daten aus der Spezifikation holen. */
{
   Ausgabelevel = level;
   IO_level     = level;
   IO_GUeberschrift = FALSE;
#if IO_KOMBINATORAUSGABE
   TO_KombinatorapparatAktivieren();
#endif
}  


void IO_InitAposterioriEarly (void)
/* Daten aus der Spezifikation holen. */
{
   func_symb_list p;
   
   IO_SymbolNamen = array_alloc(SO_AnzahlFunktionen,char*);
   for (p=get_sig(); p; p = sig_rest(p)) {
     func_symb f = sig_first(p);
     IO_SymbolNamen[SN_WMvonDISCOUNT(f)] = our_strdup(map_func(f));
   }
   if (TRUE) {
     IO_SymbolNamen[SO_const1] = our_strdup("const1");
     IO_SymbolNamen[SO_const2] = our_strdup("const2");
   }
   if (PM_Existenzziele()) {
     ParamodSymbolnamenEtablieren(SO_EQ   ,"eq");
     ParamodSymbolnamenEtablieren(SO_TRUE ,"true");
     ParamodSymbolnamenEtablieren(SO_FALSE,"false");
   }
   IO_SymbolNamen[SO_EQN] = our_strdup("===");
#ifndef CCE_Source
   strcpy(IO_SpezName,get_spec_name());
   strcpy(IO_Modus,mode_names[PA_ExtModus()]);
#endif
   /* default-Werte setzen */
   Ausgabelevel = 2;
   IO_level     = 2;
   IO_GUeberschrift = FALSE;
}  


int IO_AusgabeLevel(void)
{
   return Ausgabelevel;
}

void IO_AusgabeLevelBesetzen(int level)
{
  IO_level     = level;
  Ausgabelevel = level;
}

void IO_AusgabenAus(void)
/* Schaltet unabhaengig vom aktuellen Ausgabelevel die Ausgaben aus.*/
{
  IO_level = 0;
}

void IO_AusgabenEin(void)
/* Schaltet die mit IO_AusgabenAus zuvor ausgeschalteten Ausgaben gemaess Einstellung wieder ein.*/
{
  IO_level = Ausgabelevel;
}

void IO_SpezifikationsnamenSetzen(char *Filename)
/* Setzt den Namen der bearbeiteten Spezifikation (incl. Pfad) ein.*/
{
   Spezifikationsname = our_strdup(Filename);
}

char *IO_Spezifikationsname(void)
/* Gibt den Namen (und ggf. Pfad) der zu verwendeten Spez.datei zurueck.*/
{
   return Spezifikationsname;
}

char *IO_SpezifikationsBasisname(void)
/* Gibt den Basisnamen (ohne Pfad) der zu verwendeten Spez.datei zurueck.*/
{
  if (Spezifikationsname != NULL){
    char *Dateiname = strrchr(Spezifikationsname,'/');
    if (Dateiname != NULL){
      return Dateiname + 1;
    }
  }
  return Spezifikationsname;
}

/* -------------------------------------------------------*/
/* -------------- Kopf und Spezifikation -----------------*/
/* -------------------------------------------------------*/

void IO_DruckeKopf(void)
{
  if (COMP_DIST && PA_Slave()){
    return;
  }
#if !COMP_POWER
#endif
  printf("********************************************************************************\n");
  printf("*                             W A L D M E I S T E R           \\|  \\ /      \\|/ *\n");
  printf("*                                                              |/  |    \\/  |  *\n");
  printf("*              (C) 1994-2010  A. Buch and Th. Hillenbrand,      \\ /      \\ /   *\n");
  printf("*                             A. Jaeger and B. Loechner          |        |    *\n");
  printf("*                             <waldmeister@informatik.uni-kl.de>          |    *\n");
  printf("********************************************************************************\n");
#if !COMP_POWER
  printf("\n");
#endif
}

void IO_DruckeBeispiel(void)
{
  if (Spezifikationsname != NULL){
    if (IO_SpezName && IO_SpezName [0] != '\0') {
       printf("   %-40s: %s\n\n", "problem           ", IO_SpezName);
       printf("   %-40s: %s\n\n", "... from file     ", Spezifikationsname);
       printf("   %-40s: %s\n",   "Computation Mode",   IO_Modus);
    }
  }
}  

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* +++++++++++++ Elementare Druck-Funktionen +++++++++++++*/
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

void IO_DruckeGanzzahl(int n)
{
  printf("%d",n);
}

static void DruckeStellenfolgeStrom(FILE *Strom, SeitenT seite, UTermT Term, UTermT Teilterm);
static void DruckeStellenfolgeStromLit(FILE *Strom, UTermT Lit, UTermT Teilterm);
static void io_druckeTermStrom (FILE *Strom, UTermT t, BOOLEAN kurz);
static void DruckePositionStrom(FILE *Strom, Position pos);
static void DruckeIdStrom(FILE *Strom, Id* id);
static void DruckeWIdStrom(FILE *Strom, WId* wid);

static void io_vfdruckeFlex(FILE *Strom, const char *fmt, va_list ap)
/* Analogon zu vfprintf */
{
   int ival;
   double dval;
   unsigned long uval;
   long int lval;
   void *pval;
   EConsT eval;
   DConsT Dval;
   ContextT cval;
   TermT tval,tval2;
   UTermT t_NULL;
   SeitenT seite;
   Position pos;
   Id* id;
   WId* wid;
   char *s;
   char *ArgPosition;
   
   do {
     ArgPosition = strchr(fmt, '%');
     if (ArgPosition != NULL){
       fwrite(fmt, sizeof(char), ArgPosition-fmt, Strom); /* (ArgPosition-fmt) Objekte aus fmt auf Ausgabe-Strom schreiben */
       switch (*++ArgPosition){
       case '*':
         ival = va_arg(ap, int);
         while (ival > 0){
           putc(' ', Strom);
           ival--;
         }
         break;
       case 'b':
         ival = va_arg(ap, int);
         fprintf(Strom, "%s", ival ? "True " : "False");
         break;
       case 'C':
	 cval = va_arg(ap, ContextT);
	 CO_printContext(Strom,cval);
         break;
       case 'd':
         ival = va_arg(ap, int);
         fprintf(Strom, "%d", ival);
         break;
       case 'D':
         Dval = va_arg(ap, DConsT);
         CO_printDCons(Strom, Dval);
         break;
       case 'e':
         eval = va_arg(ap, EConsT);
         CO_printECons(Strom, eval, FALSE);
         break;
       case 'E':
         eval = va_arg(ap, EConsT);
         CO_printECons(Strom, eval, TRUE);
         break;
       case 'f':
         dval = va_arg(ap, double);
         fprintf(Strom, "%f", dval);
         break;
       case 's':
         s = va_arg(ap, char *);
         if (s != NULL){
           fprintf(Strom, "%s", s);
         }
         else {
           fprintf(Strom, "(null)");
         }
         break;
       case 'l':
         lval = va_arg(ap, long);
         fprintf(Strom, "%ld",lval);
         break;
       case 'L':{
         long long llval = va_arg(ap, long long);
         fprintf(Strom, "%lld", llval);
         break;
       }
       case 'u':
         uval = va_arg(ap, unsigned long);
         fprintf(Strom, "%lu", uval);
         break;
       case 'U':{
         unsigned long long llval = va_arg(ap, unsigned long long);
         fprintf(Strom, "%llu",llval);
         break;
       }
       case 'v':
         lval = va_arg(ap, long);
	 switch (lval){
         case Kleiner:        fprintf(Strom, " < "); break;
         case Gleich:         fprintf(Strom, " = "); break;
         case Groesser:       fprintf(Strom, " > "); break;
         case Unvergleichbar: fprintf(Strom, " # "); break;
	 default:             fprintf(Strom, " ? "); break;
	 }
         break;
       case 'V':
         lval = va_arg(ap, long);
	 switch (lval){
	 case Gleich:         fprintf(Strom, "Gleich");         break;
	 case Unvergleichbar: fprintf(Strom, "Unvergleichbar"); break;
	 case Groesser:       fprintf(Strom, "Groesser");       break;
	 case Kleiner:        fprintf(Strom, "Kleiner");        break;
	 default:             fprintf(Strom, "??VGL??");        break;
	 }
         break;
       case 'S':
	 {
	   SymbolT sym = (SymbolT) va_arg(ap,int);
	   IO_DruckeSymbolStrom(Strom,sym);
	 }
	 break;
       case 't':
         tval = va_arg(ap, TermT);
         io_druckeTermStrom(Strom, tval, TRUE);
         break;
       case 'T':
         tval = va_arg(ap, TermT);
         io_druckeTermStrom(Strom, tval, FALSE);
         break;
       case 'A':
         tval = va_arg(ap, TermT);
	 tval2 = tval;
	 t_NULL = va_arg(ap, UTermT);
 	 fprintf(Strom,"[");
	 while (tval != t_NULL){
	   if (tval != tval2){
	     fprintf(Strom,", ");
	   }
	   io_druckeTermStrom(Strom,tval,TRUE);
	   tval = TO_NaechsterTeilterm(tval);
	 }
	 fprintf(Strom,"]");
         break;
       case 'p':
         seite = va_arg(ap, SeitenT);
         tval = va_arg(ap, TermT);
	 tval2 = va_arg(ap, TermT);
         DruckeStellenfolgeStrom(Strom,seite,tval,tval2);
         break;
       case 'P':
         tval = va_arg(ap, TermT);
	 tval2 = va_arg(ap, TermT);
         DruckeStellenfolgeStromLit(Strom,tval,tval2);
         break;
       case 'X':
         pval = va_arg(ap, void *);
         fprintf(Strom,"%p",pval);
         break;
       case 'q':
         seite = va_arg(ap, SeitenT);
	 putc(seite == linkeSeite ? 'L' : 'R', Strom);
         break;
       case 'z':
         pos = va_arg(ap,Position);
         DruckePositionStrom(Strom,pos);
         break;
       case 'i':
         id = va_arg(ap,Id*);
         DruckeIdStrom(Strom,id);
         break;
       case 'w':
         wid = va_arg(ap,WId*);
         DruckeWIdStrom(Strom,wid);
         break;
       default:
         putc(*ArgPosition, Strom);
         break;
       }
       fmt = ArgPosition + 1;
     }
     else
       fprintf(Strom, fmt);
   } while(ArgPosition);
}

void IO_DruckeFlex(const char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   io_vfdruckeFlex(stdout,fmt,ap);
   va_end(ap);
}

void IO_DruckeFlexStrom(FILE *Strom, const char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   io_vfdruckeFlex(Strom,fmt,ap);
   va_end(ap);
}



void IO_DruckeZeile (char zeichen)
/* Druckt eine ganze Zeile mit dem Zeichen zeichen. */
{
  short i;

  for (i=0; i<IO_Ueberschriftenbreite; i++){
    putc (zeichen, stdout);
  }
  IO_LF();
}

void IO_DruckeUeberschrift(const char *text)
/* Ueberschrift drucken */
{
  short i, rahmen;

  IO_LF();
  IO_DruckeZeile('*');
  rahmen = ((IO_Ueberschriftenbreite - strlen(text) - 2) / 2);
  for (i=0; i<rahmen; i++){
    printf("*");
  }
  printf(" ");
  printf(text);
  printf(" ");
  for (i=rahmen+strlen(text)+2; i<IO_Ueberschriftenbreite; i++){
    printf("*");
  }
  IO_LF();
  IO_DruckeZeile('*');
  IO_LF();
}  

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* +++++++++++++ Druck-Funktionen fuer System-Strukturen +*/
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

void IO_DruckeSymbol(SymbolT symbol)
{
  IO_DruckeSymbolStrom(stdout,symbol);
}

char *IO_FktSymbName(SymbolT FktSymb)
/* Zeiger auf Zeichenketten-Darstellung des Druckenamens des Funktionssymbols */
{
  return IO_SymbolNamen[FktSymb];
}


void IO_DruckeSymbolStrom(FILE *Strom, SymbolT symbol)
/* Ausgabe eines Symbols auf dem angegebenen Ausgabestrom.*/
{
#if GZ_GRUNDZUSAMMENFUEHRUNG
  if (SO_IstExtrakonstante(symbol))
    fprintf(Strom, "c%d",symbol-SO_Extrakonstante(1)+1); 
  else
#endif
  if (SO_SymbIstFkt(symbol))
#ifndef CCE_Source
    fprintf(Strom,IO_SymbolNamen[symbol]);
#else
    fprintf(Strom,SO_Symbolnamen(symbol));
#endif
  else
    fprintf(Strom,"x%d",SO_VarNummer(symbol));
}

#if IO_KOMBINATORAUSGABE

static BOOLEAN IstKTerm(UTermT t)
{
  return SO_SymbGleich(TO_TopSymbol(t),TO_ApplyOperator);
}

static void DruckeKTerm(FILE *Strom, UTermT t, BOOLEAN Klammern)
{
  unsigned int n = 1;
  UTermT ti;
  switch (TO_AnzahlTeilterme(t)) {
  case 0:
    IO_DruckeSymbolStrom(Strom,TO_TopSymbol(t));
    break;
  case 1:
    IO_DruckeSymbolStrom(Strom,TO_TopSymbol(t));
    for (ti=TO_Schwanz(t); TO_TopSymboleGleich(t,ti); n++, ti=TO_Schwanz(ti));
    if (n>=StartzahlVerkettungskuerzung)
      fprintf(Strom, "^%d",n); 
    putc('(', Strom);
    DruckeKTerm(Strom,n>=StartzahlVerkettungskuerzung ? ti : TO_Schwanz(t),TRUE);
    putc(')', Strom);
    break;
  case 2:
    if (IstKTerm(t)) {
      UTermT t1 = TO_ErsterTeilterm(t),
        t2 = TO_NaechsterTeilterm(t1);
      if (Klammern) putc('(', Strom);
      DruckeKTerm(Strom,t1,FALSE);
      putc(' ', Strom);
      DruckeKTerm(Strom,t2,TRUE);
      if (Klammern) putc(')', Strom);
      break;
    }
  default:
    IO_DruckeSymbolStrom(Strom,TO_TopSymbol(t));
    putc('(', Strom);
    ti = TO_ErsterTeilterm(t);
    DruckeKTerm(Strom,ti,TRUE);
    while (TO_PhysischUngleich(ti = TO_NaechsterTeilterm(ti),TO_NULLTerm(t))) {
      putc(',', Strom);
      DruckeKTerm(Strom,ti,TRUE);
    }
    putc(')', Strom);
  }
}

static void io_druckeTermStrom (FILE *Strom, UTermT t, BOOLEAN kurz MAYBE_UNUSEDPARAM)
{
  DruckeKTerm(Strom,t,FALSE);
}

#else

static void io_druckeTermStrom (FILE *Strom, UTermT t, BOOLEAN kurz)
/* Druckt den Term t auf den Ausgabestrom Strom aus. Dabei werden Folgen gleicher, einstelliger
 * Funktionssymbole nicht gekuerzt und Konstanten mit nachfolgendem Klammernpaar ausgegeben.
 * Ist kurz == TRUE enfallen Argumentklammern fuer Konstanten, weiterhin werden
 * aufeinanderfolgende unaere Symbole gekuerzt: f(f(f(f(f(x))))) wird zu f^5(x)
 */
{
  if (t == NULL){
    fputs("<nullterm>", Strom);
  }
  else if (TO_Nullstellig(t)){
    if (TO_TermIstVar(t) && !kurz)    /* XXX TPTP XXX */
      fprintf(Strom,"X%d",SO_VarNummer(TO_TopSymbol(t)));
    else
      IO_DruckeSymbolStrom(Strom, TO_TopSymbol(t));
#if 0 /* XXX TPTP XXX */
    if (!kurz && TO_TermIstNichtVar(t)){ /* bei Konstanten Klammernpaar ausgeben */
      fputs("()", Strom);
    }
#endif
  }
  else {
    for(;;) {
      switch (TO_AnzahlTeilterme(t)) {
      case 0: 
        if (TO_TermIstVar(t) && !kurz)    /* XXX TPTP XXX */
          fprintf(Strom,"X%d",SO_VarNummer(TO_TopSymbol(t)));
        else
          IO_DruckeSymbolStrom(Strom, TO_TopSymbol(t));
#if 0 /* XXX TPTP XXX */
        if (!kurz && TO_TermIstNichtVar(t)){ /* bei Konstanten Klammernpaar ausgeben */
          fputs("()", Strom);
        }
#endif
        while (!(--TI_TopZaehler)) {
          putc(')', Strom);
          if (TI_PopUndLeer())
            return;
        }
        putc(',', Strom);
        t = TO_Schwanz(t);
        break;
      case 1: 
        if (kurz){
          SymbolT Symbol = TO_TopSymbol(t);
          UTermT Teilterm = TO_ErsterTeilterm(t);
          unsigned int n = 1;
          while (SO_SymbGleich(Symbol,TO_TopSymbol(Teilterm))) {
            n++;
            Teilterm = TO_ErsterTeilterm(Teilterm);
          }
          if (n >= StartzahlVerkettungskuerzung) {
            IO_DruckeSymbolStrom(Strom,Symbol);
            fprintf(Strom, "^%d(",n);
            TI_PushOT(t);
          }
          else {
            while (n--) {
              IO_DruckeSymbolStrom(Strom,Symbol);
              putc('(', Strom);
              TI_PushOT(t);
            }
          }
          t = Teilterm;
        }
        else {
          IO_DruckeSymbolStrom(Strom, TO_TopSymbol(t));
          putc('(', Strom);
          TI_PushOT(t);
          t = TO_ErsterTeilterm(t);
        }
        break;
      default: 
        IO_DruckeSymbolStrom(Strom, TO_TopSymbol(t));
        putc('(', Strom);
        TI_PushOT(t);
        t = TO_Schwanz(t);
        break;
      }
    }
  }
}

#endif

void IO_DruckeTerm(UTermT Term)
/* Gibt einen Term auf den eingestellten Ausgabestrom aus. */
{
  io_druckeTermStrom(stdout, Term, TRUE);
}

void IO_DruckeTermPaar(UTermT term1,const char *trenner,UTermT term2)
{
   IO_DruckeFlex("%t %s %t", term1, trenner, term2);
}

void IO_DruckeFaktum(TermT l, BOOLEAN orientierbar, TermT r)
{
  UTermT s,t;
#ifndef CCE_Source
  if (Z_UngleichungEntmantelt(&s,&t, l,r))
    IO_DruckeTermPaar(s,"?",t);
  else
#endif
    IO_DruckeTermPaar(l, orientierbar ? "->" : "=", r);
  IO_DruckeFlex("\n");
}


static void DruckeFolgenelement(FILE *Strom, unsigned int i)
{
  fprintf(Strom, "%d",TO_Stelligkeit(TI_Kopierstapel[i].Teilterm)+1-TI_Kopierstapel[i].Zaehler);
}

static void DruckeStellenfolgeStrom(FILE *Strom, SeitenT seite, UTermT Term, UTermT Teilterm)
{
  putc (seite == linkeSeite ? 'L' : 'R', Strom);
  if (TO_PhysischUngleich(Term,Teilterm)) {
    unsigned int i;
    TI_Push(Term);
    while (TO_PhysischUngleich(Term = TO_Schwanz(Term),Teilterm))
      if (TO_Nullstellig(Term))
        while (!(--TI_TopZaehler))
          TI_Pop();
      else
        TI_Push(Term);
    for (i=1; i<=TI_KSIndex; i++) {
      putc('.',Strom);
      DruckeFolgenelement(Strom,i);
    }
    TI_KSLeeren();
  }
  /* Gilt TO_PhysischGleich(Term,Teilterm), was fuer Term nullstellig stets der Fall
     sein muss, so handelt es sich um die Wurzelstelle, wofuer die leere Stellenfolge 
     ausgegeben wird. */
}

static void DruckeStellenfolgeStromLit(FILE *Strom, UTermT Lit, UTermT Teilterm)
{
  unsigned int i;
  UTermT Term = Lit;
  TI_Push(Term);
  while (TO_PhysischUngleich(Term = TO_Schwanz(Term),Teilterm))
    if (TO_Nullstellig(Term))
      while (!(--TI_TopZaehler))
        TI_Pop();
    else
      TI_Push(Term);
  putc ((TI_Kopierstapel[1].Zaehler == 2) ? 'L' : 'R', Strom);
  for (i=2; i<=TI_KSIndex; i++) {
    putc('.',Strom);
    DruckeFolgenelement(Strom,i);
  }
  TI_KSLeeren();

  /*  fprintf(Strom,"|%ld|", TO_StelleInTerm(Teilterm,Lit));*/
}

void DruckePositionStrom(FILE *Strom, Position pos)
{
    putc('[',Strom);
    putc(POS_into_i(pos)?'1':'0',Strom);    
    putc(',',Strom);
    putc(POS_i_right(pos)?'1':'0',Strom);    
    putc(',',Strom);
    putc(POS_j_right(pos)?'1':'0',Strom);
    putc(',',Strom);
    if (POS_pos(pos) == POS_MASK) {
	putc('*',Strom);
    } else {
	fprintf(Strom,"%lu",POS_pos(pos));    
    };
    putc(']',Strom);
}

void DruckeIdStrom(FILE *Strom, Id* id)
{
     if (ID_i(*id) == ID_WILDCARD) {
	fprintf(Strom,"<*,");
    } else {
	fprintf(Strom,"<%u,",ID_i(*id));
    }
    if (ID_j(*id) == ID_WILDCARD) {
	fprintf(Strom,"*,");
    } else {
	fprintf(Strom,"%u,",ID_j(*id));
    }
    DruckePositionStrom(Strom,ID_xp(*id));
    putc('>',Strom);    
}

void DruckeWIdStrom(FILE *Strom, WId* wid)
{
    if (WID_w(*wid) == maximalWeight()) {
	fprintf(Strom,"<+oo,");
    } else if (WID_w(*wid) == minimalWeight()) {
	fprintf(Strom,"<-oo,");
    } else if (WID_w(*wid) == undefinedWeight()) {
	fprintf(Strom,"<?,");
    } else {
	fprintf(Strom,"<%lu,",WID_w(*wid));
    }
    if (WID_i(*wid) == ID_WILDCARD) {
	fprintf(Strom,"*,");
    } else {
	fprintf(Strom,"%u,",WID_i(*wid));
    }
    if (WID_j(*wid) == ID_WILDCARD) {
	fprintf(Strom,"*,");
    } else {
	fprintf(Strom,"%u,",WID_j(*wid));
    }
    DruckePositionStrom(Strom,WID_xp(*wid));
    putc('>',Strom);    
}

    

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* +++++++++++++++++++++ Sonstiges +++++++++++++++++++++++*/
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

unsigned int IO_SymbAusgabelaenge(SymbolT Symbol)
{
  return SO_SymbIstFkt(Symbol) ? strlen(IO_SymbolNamen[Symbol]) : (1+LaengeNatZahl(SO_VarNummer(Symbol)));
}

#ifdef CCE_Protocol

static CCE_loggingT CCE_State = CCE_Undef;
static FILE *CCEFile;
static BOOLEAN do_CCE = FALSE;
static char *CCEFilename;
static char CCEInitialized = FALSE;

void IO_CCEinit(char *filename)
{
  CCEFilename = filename;
  do_CCE = TRUE;
}
static void CCE_OpenFile()
{
  SymbolT symbol;

  if (!do_CCE) return;
  
  if (!(CCEFile = fopen(CCEFilename, "w"))){
    our_error("Cannot open file for logging CCE!");
  }  
  CCEInitialized = TRUE;

  fprintf (CCEFile, "NAME\t%s\n\n", get_spec_name());
  fprintf (CCEFile, "MODE\tCCE\n\n");
  fprintf (CCEFile, "SORTS\t");
  printSorts (CCEFile);

  fprintf (CCEFile, "\nSIGNATURE\n");
  printSig (CCEFile);

  fprintf (CCEFile, "ORDERING\t");
  switch (PA_Ordering()) {
  case ordering_kbo: fprintf (CCEFile, "KBO\n");break;
  case ordering_lpo: fprintf (CCEFile, "LPO\n");break;
  default:
    our_error("unexpected label in IO_CCEinit");
    break;
  }
  printOrdering (CCEFile);

  fprintf (CCEFile, "VARIABLES\n");
  fprintf (CCEFile, "\tx1");
  for (symbol = 2; symbol <= 10; symbol++) {
    fprintf (CCEFile, ", x%d", symbol);
  }
  fprintf (CCEFile, " : ");
  /* wir geben allen Variablen die gleiche Sorte, nicht schoen,
     aber anscheinend einzige Moeglichkeit, da Variablen keine
     Sorte haben - hier geht Information verloren */
  printSort (CCEFile, 1);
  fprintf (CCEFile, "\n\n");
}

void IO_CCElog(CCE_loggingT tp, TermT lhs, TermT rhs)
{
  if (!do_CCE) return;
  if (!CCEInitialized)
    CCE_OpenFile();
  if (CCE_State == tp){
    IO_DruckeFlexStrom(CCEFile, "\t%T = %T\n", lhs, rhs);
  }
  else {
    CCE_State = tp;
    switch (tp){
    case CCE_Add:    IO_DruckeFlexStrom(CCEFile, "\nADD\t%T = %T\n",    lhs, rhs); break;
    case CCE_Delete: IO_DruckeFlexStrom(CCEFile, "\nDELETE\t%T = %T\n", lhs, rhs); break;
    case CCE_Test:   IO_DruckeFlexStrom(CCEFile, "\nTEST\t%T = %T\n",   lhs, rhs); break;
    default:         our_error("unexpected value for CCE_loggingT in IO_CCElog!");
    }
  }
}
#endif

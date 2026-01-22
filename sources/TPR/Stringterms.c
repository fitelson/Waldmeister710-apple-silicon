#include "Ausgaben.h"
#include "Stringterms.h"
#include "SymbolOperationen.h"
#include "TermIteration.h"
#include "TermOperationen.h"
#include "general.h"

TI_IterationDeklarieren(STT)

/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- Platzbedarf bestimmen -------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

static unsigned long int Platzbedarf (TermT t)
{
  unsigned long int res = 0;
  do {                                 /* Term durchlaufen, dabei Stellen zaehlen */
    res++;
    if (TO_TermIstVar(t) && (TO_TopSymbol(t) < -127)){
      res = res + 2; /* jede Variablen-Nummer < -127 wird mit drei Byte abgelegt! */
    }
    t = TO_Schwanz(t);
  } while (t != NULL);
  return res;
}

unsigned long int STT_TermpaarPlatzbedarf(TermT l, TermT r)
/* Termpaar durchlaufen und dabei tatsaechlichen Platzbedarf ermitteln:
   jede Variablen-Nummer < -127 wird mit drei Byte abgelegt!*/
{
  return Platzbedarf(l) + Platzbedarf(r);
}

/* -------------------------------------------------------------------------------------------------------------*/
/* ------------------- Auslesen von Symbolen in gepackter Darstellung ------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

static SymbolT Topsymbol(PTermpaarT *pp)
/* Liefert das Topsymbol von pp und schaltet den Zeiger pp um eine bzw. drei Stellen weiter.*/
{
   SymbolT top = (signed char)**pp;
   (*pp)++;
   if (top == -128){
      unsigned short int Top;
      Top = (**pp) << 8;         (*pp)++;
      Top = Top | (**pp);        (*pp)++;
      top = -Top;
   }
   return top;
}

/* -------------------------------------------------------------------------------------------*/
/* ------------------- Ein- und Auspacken von Termen -----------------------------------------*/
/* -------------------------------------------------------------------------------------------*/

void STT_TermpaarEinpackenFragil(TermT lhs, TermT rhs, PTermpaarT tp)
/* Die Gleichung lhs=rhs in das uebergebene PTermpaar einschreiben. 
   Kein Test auf ausreichende Groesse von tp! */
{
   PTermpaarT pIndex;
   TermT tIndex;
   SymbolT top;
   /*IO_DruckeFlex("HINEIN: %t = %t\n",lhs,rhs);*/

   pIndex = tp;

   /* hier wird in das erste Byte des gepackte Termpaares die Nummer des */
   /* verwendeten Managers eingetragen */

   TO_NachfBesetzen(TO_TermEnde(lhs), rhs);
   tIndex = lhs;

   do{
      top = TO_TopSymbol(tIndex);
      if (top > -128){
         *pIndex = (byte) top;  pIndex++;
      }
      else{
        unsigned short int Top = -top;
        *pIndex = (byte) (-128);        pIndex++;
        *pIndex = (byte) (Top >> 8);    pIndex++;
        *pIndex = (byte) (Top & 0xff);  pIndex++;
     }
   }while((tIndex = TO_Schwanz(tIndex)));

   TO_NachfBesetzen(TO_TermEnde(lhs), NULL);
}

PTermpaarT STT_TermpaarEinpacken(TermT lhs, TermT rhs, int *len)
/* Die Gleichung lhs=rhs in ein neu allokiertes PTermpaar einschreiben. 
   Rueckgabe: neues PTermpaar und dessen Laenge in len */
{
  int needed = STT_TermpaarPlatzbedarf(lhs,rhs);
  PTermpaarT res = our_alloc(needed);
  STT_TermpaarEinpackenFragil(lhs,rhs,res);
  *len = needed;
  return res;
}

static TermT TermAuspacken(PTermpaarT *pp)
/* Ersten Term aus Termpaar pp auspacken, dabei Rueckgabe-Term erzeugen.*/
{
  SymbolT Symbol = Topsymbol(pp);
  if (SO_Stelligkeit(Symbol)==0) {
    TermT TermNeu;
    TO_TermzelleHolen(TermNeu);
    TO_SymbolBesetzen(TermNeu,Symbol);
    TO_NachfBesetzen(TermNeu,NULL);
    TO_EndeBesetzen(TermNeu,TermNeu);
    return TermNeu;
  }
  else {
    TermT TermNeu = TO_HoleErsteNeueZelle(),
          IndexNeu = TermNeu;
    TO_SymbolBesetzen(TermNeu,Symbol);
    TI_Push(TermNeu);
    do {
      TO_AufNaechsteNeueZelle(IndexNeu);
      Symbol = Topsymbol(pp);
      TO_SymbolBesetzen(IndexNeu,Symbol);
      if (SO_Stelligkeit(Symbol)==0) {
        TO_EndeBesetzen(IndexNeu,IndexNeu);
        while (!(--TI_TopZaehler)) {
          TO_EndeBesetzen(TI_TopTeilterm,IndexNeu);
          if (TI_PopUndLeer()) {
            TO_LetzteNeueZelleMelden(IndexNeu);
            TO_NachfBesetzen(IndexNeu,NULL);
            return TermNeu;
          }
        }
      }
      else
        TI_Push(IndexNeu);
    } while (TRUE);
  }
}


void STT_TermpaarAuspacken(PTermpaarT tp, TermT *lhsPtr, TermT *rhsPtr)
{
  PTermpaarT tp2 = tp;
  *lhsPtr = TermAuspacken(&tp2);
  *rhsPtr = TermAuspacken(&tp2);
  /*IO_DruckeFlex("HERAUS: %t = %t\n",*lhsPtr,*rhsPtr);*/
}

/* -------------------------------------------------------------------------------------------------------------*/
/* ------- Output ----------------------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

static PTermpaarT DruckeTermAusPTermpaar(PTermpaarT tp)
/* Gepacktes Termpaar drucken.*/
{
  SymbolT symb;
  byte *DurchlaufIndex = tp;

  symb = Topsymbol(&DurchlaufIndex); /* schaltet weiter */
  if (SO_Stelligkeit(symb) == 0)
    IO_DruckeSymbol(symb);
  else
    do
      switch (SO_Stelligkeit(symb)) {
      case 0:
        {
          IO_DruckeSymbol(symb);
          while (!(--TI_TopZaehler)) {
            printf(")");
            if (TI_PopUndLeer())
              return DurchlaufIndex;
          }
          printf(",");
          symb = Topsymbol(&DurchlaufIndex); /* schaltet weiter */
          break;
        }
      case 1:
        {
          SymbolT Symbol = Topsymbol(&DurchlaufIndex); /* schaltet weiter */
          unsigned int n = 1;
          while (SO_SymbGleich(symb, Symbol)) {
            n++;
            Symbol = Topsymbol(&DurchlaufIndex);
          }
          if (n>=StartzahlVerkettungskuerzung) {
            IO_DruckeSymbol(symb);
            printf("^%d(",n);
            TI_PushMitAnz(NULL,1); /* Stelligkeit war eins */
          }
          else
            while (n--) {
              IO_DruckeSymbol(symb);
              printf("(");
              TI_PushMitAnz(NULL,1);
            }
          symb = Symbol;
          break;
        }
      default:
        {
          IO_DruckeSymbol(symb);
          printf("(");
          TI_PushMitAnz(NULL,SO_Stelligkeit(symb)); /* mehrstellig -> Fkt-Symb */
          symb = Topsymbol(&DurchlaufIndex);
          break;
        }
      }
    while (TRUE);
  return DurchlaufIndex;
}

void STT_DruckePTermpaar(PTermpaarT tp, const char *trenner)
{
  PTermpaarT tp2;
  tp2 = DruckeTermAusPTermpaar(tp);
  printf(" %s ", trenner);
  DruckeTermAusPTermpaar(tp2);
}


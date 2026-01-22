/*=================================================================================================================
  =                                                                                                               =
  =  VariablenMengen.c                                                                                            =
  =                                                                                                               =
  =  Funktionen bzw. Makros zur Darstellung von Variablenmengen als Bitsets                                       =
  =                                                                                                               =
  =  Zweck: Einsatz bei Vorpruefung der Terme in Ordnungen                                                        =
  =  Zur Verfuegung gestellt werden Einfuegen von Elementen in Mengen sowie VM_MengenVergleich.                   =
  =                                                                                                               =
  =  24.02.94 Thomas. Als separates Modul geschrieben.                                                            =
  =  28.02.94 Thomas. Ausfuehrlich getestet und fuer Include umgearbeitet.                                        =
  =                                                                                                               =
  =================================================================================================================*/

#include <limits.h>

#include "SpeicherVerwaltung.h"
#include "VariablenMengen.h"

#if VAR_SETS_UNLIMITED
/*=================================================================================================================*/
/*= 0. Typdeklarationen                                                                                           =*/
/*=================================================================================================================*/

#define LaengeVMengenteilT ((signed)BITS(VMengenteilT))



/*=================================================================================================================*/
/*= I. Initialisierung                                                                                            =*/
/*=================================================================================================================*/

static SV_SpeicherManagerT VM_FreispeicherManager;

void VM_InitApriori(void)
{
  SV_ManagerInitialisieren(&VM_FreispeicherManager,VMengenT,VM_AnzListenzellenJeBlock,"VariablenMengen");
}



/*=================================================================================================================*/
/*= II. Elementare Operationen auf Mengen und Elementen                                                           =*/
/*=================================================================================================================*/

void VM_MengeLeeren(VMengenT *Menge)                                             /* fuer Mengen beliebiger Groesse */
{
  do
    Menge->Teil = (VMengenteilT) 0L;
  while ((Menge = Menge->Nachf));
}

void VM_MengeLoeschen(VMengenT *Menge)                                                       /* Speicher freigeben */
{
  VMengenT *Alt;
  Menge = Menge->Nachf;
  while ((Alt = Menge)) {
    Menge = Menge->Nachf;
    SV_Dealloc(VM_FreispeicherManager,Alt);
  }
}


void VM_ElementEinfuegen(VMengenT *Menge, SymbolT Symbol)                /* Element in Menge einfuegen, dabei ggf. */  
{                                                                             /* neue, leere Bitfelder hinzufuegen */
  Symbol = -Symbol;
  while (Symbol>=LaengeVMengenteilT) {
    Symbol -= LaengeVMengenteilT;
    if (Menge->Nachf)
      Menge = Menge->Nachf;
    else {
      SV_Alloc(Menge->Nachf,VM_FreispeicherManager,VMengenT *);
      (Menge = Menge->Nachf)->Teil = (VMengenteilT) 0L;
      Menge->Nachf = NULL;
    }
  }
  Menge->Teil |= 1L<<Symbol;
}


BOOLEAN VM_EnthaltenIn(VMengenT *Menge, SymbolT Symbol)                           /* Ist Symbol Element von Menge? */
{
  Symbol = -Symbol;
  while (Symbol>=LaengeVMengenteilT) {
    if (!(Menge = Menge->Nachf))
      return FALSE;
    Symbol -= LaengeVMengenteilT;
  }
  return (BOOLEAN) Menge->Teil>>Symbol & 1L;
}


BOOLEAN VM_NichtEnthaltenIn(VMengenT *Menge, SymbolT Symbol)                /* Ist Symbol nicht Element von Menge? */
{
  Symbol = -Symbol;
  while (Symbol>=LaengeVMengenteilT) {
    if (!(Menge = Menge->Nachf))
      return TRUE;
    Symbol -= LaengeVMengenteilT;
  }
  return (BOOLEAN) (Menge->Teil>>Symbol & 1L) ^ 1L;
}



/*=================================================================================================================*/
/*= III. Funktionen zum Vergleichen von Mengen                                                                    =*/
/*=================================================================================================================*/

#define /*BOOLEAN*/ TeilNichtKleinerGleich(/*VMengenteilT*/ Teil1, /*VMengenteilT*/ Teil2)                          \
  Teil1 & ~Teil2                     /* testet, ob Bitfeld der 1. Menge Teilmenge eines Bitfelds der zweiten Menge */


BOOLEAN VM_Teilmenge(VMengenT *Menge1, VMengenT *Menge2)                   /* Test, ob Menge1 Teilmenge von Menge2 */
{                                                                      /* Bitfelder so lange parallel durchlaufen, */
  do                                                                 /* bis Bitfelder nicht in <=-Beziehung stehen */
    if (TeilNichtKleinerGleich(Menge1->Teil,Menge2->Teil))        /* oder alle Felder einer Menge durchlaufen sind */
      return FALSE;
  while ((Menge2 = Menge2->Nachf) && (Menge1 = Menge1->Nachf));
  return Menge2 || !Menge1->Nachf;
}


#define /*VergleichsT*/ MengenteilVergleich(/*VMengenteilT*/ Teil1, /*VMengenteilT*/ Teil2)                         \
                                                 /* In welchem Vergleichsverhaeltnis stehen die beiden Bitfelder? */\
  TeilNichtKleinerGleich(Teil1,Teil2) ?                                                                             \
    (TeilNichtKleinerGleich(Teil2,Teil1) ? Unvergleichbar : Groesser)                                               \
  :                                                                                                                 \
    Kleiner


VergleichsT VM_MengenVergleich(VMengenT *Menge1, VMengenT *Menge2)
{                              /* Bitfelder parallel durchlaufen, bis Unterschied oder eine Menge ganz durchlaufen */
  while (Menge1->Teil == Menge2->Teil) {
    if (!(Menge1 = Menge1->Nachf))
      return Menge2->Nachf ? Kleiner : Gleich;
    else if (!(Menge2 = Menge2->Nachf))
      return Groesser;
  }                             /* Beide Mengen noch nicht fertig durchlaufen, also Unterschied genauer betrachten */
  switch (MengenteilVergleich(Menge1->Teil,Menge2->Teil)) {
  case Unvergleichbar:
    return Unvergleichbar;
    break;
  case Kleiner:           /* Mengen weiter durchlaufen, bis eine fertig oder aber VM_Teilmengen-Bedingung verletzt */
    while ((Menge2 = Menge2 ->Nachf) && (Menge1 = Menge1->Nachf)) {
      if (TeilNichtKleinerGleich(Menge1->Teil,Menge2->Teil))
        return Unvergleichbar;
    }
    return (!Menge2 && Menge1->Nachf) ? Unvergleichbar : Kleiner;
    break;
  case Groesser:          /* Mengen weiter durchlaufen, bis eine fertig oder aber VM_Teilmengen-Bedingung verletzt */
    while ((Menge1 = Menge1->Nachf) && (Menge2 = Menge2->Nachf)) {
      if (TeilNichtKleinerGleich(Menge2->Teil,Menge1->Teil))
        return Unvergleichbar;
    }
    return (!Menge1 && Menge2->Nachf) ? Unvergleichbar : Groesser;
    break;
  }
  return Gleich;                                  /* Statement wird nie erreicht; verhindert aber Compiler-Warnung */
}

#endif

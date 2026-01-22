#include <ctype.h>
#include <limits.h>

#include "Ausgaben.h"
#include "FehlerBehandlung.h"
#include "SpeicherVerwaltung.h"
#include "SymbolGewichte.h"
#include "SymbolOperationen.h"

#ifndef CCE_Source
#include "Parameter.h"
#include "SpezNormierung.h"
#else
#include "parse-util.h"
#endif

long int Sg_KBO_Mu_Gewicht;

SG_GewichtstafelT Sg_CG_Tafel;
SG_GewichtstafelT Sg_CP_Tafel;
SG_GewichtstafelT Sg_KBO_Tafel;

static void sg_TafelAufEinsSetzen(SG_GewichtstafelT tafel)
{
  SymbolT f;
  tafel[SO_ErstesVarSymb] = 1;
  SO_forFktSymbole(f){
    tafel[f] = 1;
  }
}

static void sg_KBO_Mu_Bestimmen(void)
{
  SymbolT f;
  long int mu = LONG_MAX;
  SO_forFktSymbole(f){
    if (SO_SymbIstKonst(f) && Sg_KBO_Tafel[f] < mu){
      mu = Sg_KBO_Tafel[f];
    }
  }
  if (Sg_KBO_Tafel[SO_ErstesVarSymb] > mu){
    char bla[200];
    snprintf(bla,200,"KBO weight for variables reduced (%d -> %d).", Sg_KBO_Tafel[SO_ErstesVarSymb],mu);
    our_warning(bla);
    Sg_KBO_Tafel[SO_ErstesVarSymb] = mu;
  }
  Sg_KBO_Mu_Gewicht = mu;
}

static void sg_LiesSpecGewichte(SG_GewichtstafelT tafel)
{
  func_symb_list p;
  for (p = get_sig(); p; p = sig_rest(p)) {
    func_symb F = sig_first(p);
    SymbolT f = SN_WMvonDISCOUNT(F);
    tafel[f] = ord_func_weight(F);
  }
}

#ifndef CCE_Source
static void sg_AuswertungZuVeraendert(InfoStringT DEF, InfoStringT USR, SG_GewichtstafelT *ActArray)
/* Eintraege sind durch ':' getrennt. Jeder Eintrag enthaelt den Bezeichner, das Zuweisungssymbol '=' und das 
   Gewicht. Der Bezeichner ist entweder ein Funktionssymbol der Spezifikation oder eines der beiden Schluessel-
   worte VAR fuer Variablengewichte bzw. DEF fuer den Defaultwert nicht-angegebener Funktionssymbole. */
{
  /* 1. def und var raussammeln */
  long int def = IS_ValueOfOption(DEF, USR, "DEF") ? atoi(IS_ValueOfOption(DEF, USR, "DEF")) : 1;
  long int var = IS_ValueOfOption(DEF, USR, "VAR") ? atoi(IS_ValueOfOption(DEF, USR, "VAR")) : 1;
  BOOLEAN geaendert = (def != 1) || (var != 1);
  SymbolT i;
  SG_GewichtstafelT tafel = negarray_alloc(SO_AnzahlFunktionen+1,long int,1);
  sg_TafelAufEinsSetzen(tafel);

  /* 2. Identifizieren der Eintraege */
  tafel[SO_ErstesVarSymb] = var;
  SO_forFktSymbole(i) {
     char *wert = IS_ValueOfOption(DEF, USR, IO_FktSymbName(i));
     if ((wert != NULL) && (isdigit((int)*wert) || *wert=='-')){
       tafel[i] = atoi(wert);
       geaendert = geaendert || (tafel[i] != 1);
     }
     else {
       tafel[i] = def;
     }
  }

  /* 3. Test-Ausgabe */
  if (0 && !COMP_POWER) {
    printf("  Symbol-Weights: ");
    SO_forFktSymbole(i){
      IO_DruckeFlex("%S = %d, ", i, tafel[i]);
    }
    IO_DruckeFlex("Vars = %d\n", tafel[SO_ErstesVarSymb]);
  }

  /* 4. In Variable schreiben */
  if (geaendert){
    if (*ActArray != NULL){
      negarray_dealloc(*ActArray,1);
    }
    *ActArray = tafel;
  }
  else if (*ActArray == NULL){
    *ActArray = tafel;
  }
}

void SG_SymbGewichteEintragen(InfoStringT DEF, InfoStringT USR, int welcheTafel)
/* Aufdroeseln des Aufruf-Parameters nach Default-, Variablen- und den einzelnen Funktionsgewichten.
   welcheTafel = 0: Sg_CG_Tafel
   welcheTafel = 1: Sg_CP_Tafel
   welcheTafel = 2: Sg_KBO_Tafel

*/
{
  sg_AuswertungZuVeraendert(DEF,USR,
                            (welcheTafel == 1) ? &Sg_CP_Tafel : 
                            (welcheTafel == 0) ? &Sg_CG_Tafel : &Sg_KBO_Tafel);
  if (welcheTafel == 2){
    sg_KBO_Mu_Bestimmen();
  }
}
#endif

void SG_KBOGewichteVonSpecLesen(void)
{
  Sg_KBO_Tafel = negarray_alloc(SO_AnzahlFunktionen + 1,long int,1);
  sg_TafelAufEinsSetzen(Sg_KBO_Tafel);
  if (ord_name(get_ordering()) == ordering_kbo){
    sg_LiesSpecGewichte(Sg_KBO_Tafel);
  }
  sg_KBO_Mu_Bestimmen();
}

void SG_CleanUp(void)
{
  if (Sg_CP_Tafel != NULL){
    negarray_dealloc(Sg_CP_Tafel,1);
  }
  if (Sg_CG_Tafel != NULL){
    negarray_dealloc(Sg_CG_Tafel,1);
  }
  if (Sg_KBO_Tafel != NULL){
    negarray_dealloc(Sg_KBO_Tafel,1);
  }
}


/*=================================================================================================================
  =                                                                                                               =
  =  YFiles.c                                                                                                     =
  =                                                                                                               =
  =  Kapselung der Zeichenketten fuer kommandozeilenartige Parametereinstellungen                                 =
  =                                                                                                               =
  =  18.08.98 Aus PhilMarlow.c extrahiert. Thomas.                                                                =
  =                                                                                                               =
  =================================================================================================================*/

#include "Ausgaben.h"
#include "compiler-extra.h"
#include "PhilMarlow.h"
#include "SpeicherVerwaltung.h"
#include "XFiles.h"
#include "YFiles.h"


void empty(void) {;}


void lpo_(PraezedenzartenT Art, Signatur_SymbolT sig, Signatur_ZeichenT sig_c)
{
  char *Praezedenz = PG_Praezedenz(Art,sig,sig_c,ordering_lpo);
  oprintf("-ord lpo:prec=%s",Praezedenz);
  our_dealloc(Praezedenz);
}


void kbo_(PraezedenzartenT Art, Signatur_SymbolT sig, Signatur_ZeichenT sig_c)
{
  char *Praezedenz = PG_Praezedenz(Art,sig,sig_c,ordering_kbo);
  oprintf("-ord kbo:DEF=1:VAR=1:prec=%s",Praezedenz);
  our_dealloc(Praezedenz);
}


void nao(PraezedenzartenT Art MAYBE_UNUSEDPARAM)
{
  oprintf("-ord col4");
}


void zb(ZielbehandlungsartenT Art)
{
  const char *Parametrisierungen[] = 
  { "rnf",
    "mnf=re.0:s:mnfanalysis=100.200.4:mnfantiWOVar:mnfantir=5:mnfup=5",
    "mnf=re.0:s:mnfanalysis=1000.2000.10:mnfantiWOVar:mnfantir=5:mnfup=5"
  };
  oprintf("-zb %s",Parametrisierungen[Art]);
}

static SymbolT Char2Symbol(char c, Signatur_SymbolT sig, Signatur_ZeichenT sig_c)
{
  for (; *sig_c!=c; sig_c++, sig++);
  return *sig;
}


static void KlassifikationDrucken(HeuristikartenT Art, char *ClasPraefix, Signatur_SymbolT sig, Signatur_ZeichenT sig_c)
{
  if (Art==ngt) {
    char *Negation = IO_FktSymbName(Char2Symbol('N',sig,sig_c));
    oprintf("-%sclas heuristic=gtweight:DEF=5:VAR=5:%s=1",ClasPraefix,Negation);
  }
  else {
    const char *Parametrisierungen[] = { "addweight", "Addweight", "mixweight", "gtweight", "Mixweight", "polyweight", ""};
    oprintf("-%sclas heuristic=%s",ClasPraefix,Parametrisierungen[Art]);
  }
}

void cph_(HeuristikartenT Art, Signatur_SymbolT sig, Signatur_ZeichenT sig_c)
{
  KlassifikationDrucken(Art,"",sig,sig_c);
}

void cgh_(HeuristikartenT Art, Signatur_SymbolT sig, Signatur_ZeichenT sig_c)
{
  KlassifikationDrucken(Art,"cg",sig,sig_c);
}


void itl(VerschnittT Verschnitt)
{
  const struct {
    unsigned int Fifo,
                 Heuristik;
  } Schrittweiten[] = {{1,200},{1,100},{1,50},{1,10}},
  Schrittweite = Schrittweiten[Verschnitt];
  oprintf("-pq interleave=%d.%d",Schrittweite.Fifo,Schrittweite.Heuristik);
}


void gj(int level)
{
  if (level == 2){
    oprintf("-gj");
  }
  else {
    oprintf("-gj -acg -sk");
  }
}

void einsstern(void)
{
  oprintf("-einsstern");
}

void golem(void)
{
  oprintf("-golem");
}

void cgcp(void)
{
  oprintf("-sue cgcp=1.20");
}


void back(void)
{
  oprintf("-back");
}


void rose(void)
{
  if (!PM_Existenzziele) {
    oprintf("-zb mnf=re.0:s:mnfanalysis=1000.2000.10:mnfantiWOVar:mnfantir=5:mnfup=5");
    oprintf("-ord col3 -kg -: -ks -:@");
  }
}


void kern(void)
{
  oprintf("-kg -:s -ks -:s -kern");
}


void bis(SchrittzahlT Selektor)
{
  const unsigned int Schrittzahlen[] = {400,6000,14000},
                     Schrittzahl = Schrittzahlen[Selektor];
  test_error(!Schrittzahl, "Strategie fuer 0 Schritte angegeben in Tafel2");
  NeueStrategie(Schrittzahl);
}

void cc_(char welche[], Signatur_SymbolT sig, Signatur_ZeichenT sig_c)
{
  char *opt = string_alloc((MaximaleBezeichnerLaenge+1)*SO_AnzahlFunktionen+4);
  sprintf(opt,"-cc %s",IO_FktSymbName(Char2Symbol(*welche,sig,sig_c)));
  while (*++welche) {
    strcat(opt,":");
    strcat(opt,IO_FktSymbName(Char2Symbol(*welche,sig,sig_c)));
  }
  oprintf("%s",opt);
  our_dealloc(opt);
}

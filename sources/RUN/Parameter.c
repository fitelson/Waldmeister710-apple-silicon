
#include "antiwarnings.h"
#include <string.h>
#include <stdlib.h>

#include "ACGrundreduzibilitaet.h"
#include "Ausgaben.h"
#include "BeweisObjekte.h"
#include "Constraints.h"
#include "Dispatcher.h"
#include "FehlerBehandlung.h"
#include "FileMenue.h"
#include "Grundzusammenfuehrung.h"
#include "Hauptkomponenten.h"
#include "Interreduktion.h"
#include "KBO.h"
#include "Konsulat.h"
#include "KPVerwaltung.h"
#include "LPOVortests.h"
#include "NewClassification.h"
#include "NFBildung.h"
#include "Ordnungen.h"
#include "Parameter.h"
#include "PhilMarlow.h"
#include "Praezedenz.h"
#include "SpezNormierung.h"
#include "Statistiken.h"
#include "Unifikation1.h"
#include "Zielverwaltung.h"
#include "WaldmeisterII.h"

#if IF_TERMZELLEN_JAGD
int PA_HACK;
#endif

/* ------------------------------------------------*/
/* -------- Aufruf-Optionen -----------------------*/
/* ------------------------------------------------*/


/* Jede zu ermoeglichende Aufruf-Option wird in einer solchen Struktur eingerichtet.
   Dabei sind die einzelnen Teile weitgehend selbsterklaerend, bis auf 'ParameterArt':
   Ist ParameterArt == 0, so wird das folgende Argument der Aufrufzeile als Wert dieser
                          Option erwartet und als String nach eingestellterWert geschrieben ('frei');
   ist ParameterArt == 1, so wird lediglich das Vorkommen des entsprechenden Parameters festgestellt
                          (der als default eingetragene Wert wird invertiert) ('einstellig');
                          Moeglich ist hier, ueber AN/AUS bzw. an/aus unabhaengig vom Default-Wert den Schalter zu setzen.
   ist ParameterArt == 2, so wird das folgende Argument in der Werte-Liste gesucht und sein Index nach
                          eingestellterWert geschrieben ('zweistellig').
   Ein gewuenschter Default-Wert wird einfach als eingestellter Wert in der Initialisierung des Arrays (s.u.)
   angegeben, dies ist dann der Wert, der verwendet wird, wenn diese Option beim Programm-Aufruf nicht
   explizit angegeben wurde.
*/
typedef struct AufrufOptionTS{
   const char *Kuerzel;                           /* z.B. '-m' */
   const char *Name;                              /* z.B. 'Modus' */
   const char *Bedeutung;                         /* z.B. 'Einstellen des internen Modus' */
   int ParameterArt;                        /* z.B. 1 :es wird kein Wert erwartet bzw. nur AN/AUS */
   union{
      struct {                         /* Parameter-Art == 3 */
         char *DefaultInfoString;
         char *UserInfoString;              /* Ist NULL, wenn die Option nicht angegeben wurde */
      } InfoString;
      /* Bei InfoStrings ist das System-Verhalten wie folgt:
         der UserInfoString wird ausgewertet, fehlende Informationen 
         werden aus dem DefaultInfoString gezogen, wobei der UserInfoString natuerlich
         Prioritaet hat.
       */
      struct {			       /* Parameter-Art == 2 */
         int eingestellterWert;             /* z.B. 0 fuer Normal */
         int defaultWert;             /* z.B. 0 fuer Normal */
         int AnzahlVerschiedenerWerte;      /* z.B. 3 */
         char *WerteImKlartext[10];         /* z.B. 'Normal, KPSharing, MultipleNF'*/
      } zweistellig;
      struct {			       /* Parameter-Art == 1 */
         BOOLEAN eingestellterWert;         /* z.B. FALSE  */
         BOOLEAN defaultWert;         /* z.B. FALSE  */
         char *BedeutungF, *BedeutungT;     /* Bedeutung von TRUE == Bedeutung  */
      } einstellig;
      struct {			       /* Parameter-Art == 0 */
         BOOLEAN wurdeAngegeben;
         char *eingestellterWert;           /* z.B. 'braid4.prot' */
         char *defaultWert;                     /* z.B. 'braid4.prot' */
      } frei;
   } ParameterTyp;
} AufrufOptionT;

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* +++++++++++++ Beim Programm-Aufruf eingestellbare Optionen ++++*/
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

typedef enum {
  OptNrFailing, OptNrSUE, OptNrClassification,
  OptNrReClassification, OptNrCGClassification, OptNrOrdnungen,
  OptNrPriorityQueue, OptNrTermpairRepresentation, OptNrKapur,
  OptNrEinsStern, OptNrNusfu, OptNrKern, OptNrGolem, OptNrBack, 
  OptNrCancellation,
  OptNrGZFB, OptNrACG, OptNrSK, OptNrACGSKCheck, 
  OptNrACGRechts, OptNrSKRechts, OptNrStatIntervall,
  OptNrFork, OptNrWaisenmord,
  OptNrKPgen, OptNrKPsel, OptNrKPInterreduktion,
  OptNrCGInterreduktion,
  OptNrNFBildung, OptNrZielbehandlung,
  OptNrAusgabeLevel, OptNrPCLProt, OptNrPCLVerbose, OptNrPCLFile, 
  OptNrZeitlimit, OptNrRealZeitlimit,
  OptNrMemlimit, OptNrAutomodus, OptNrDis,
  OptNrSymbnorm, 
  OptNrPermsubAC, OptNrPermsubACI, 
  OptNrCTVersion, OptNrMaxNodeCount, OptNrCTS, OptNrCTP, 
  OptNrSortDCons, OptNrOptDCons, OptNrMergeDCons, OptNrPreferD1,
  OptNrFullPermLemmata, OptNrPermLemmata, OptNrIdempotenzLemmata,  OptNrC_Lemma, 
  OptNrSaveTerms, OptNrFrischzellen,
  OptNrWeightAxiom, OptNrAllocLog,
  OptNrDummy,
  OptNrLAST /* Diese Option muss immer letzte sein */
} AufrufOptionenTyp;

/* ------------------------------------------------------------------------*/
/* -------------------- globale Variablen (siehe Parameter.h) -------------*/
/* ------------------------------------------------------------------------*/

PA_ParasT PA_Paras;

static  AufrufOptionT AufrufOptionen[OptNrLAST];

static BOOLEAN pa_verbose = 0;

/* ------------------------------------------------------------------------*/
/* --- Schrottplatz -------------------------------------------------------*/
/* ------------------------------------------------------------------------*/

/*##########################################################################################*/
/*##########################################################################################*/
/*##########################################################################################*/
/*##########################################################################################*/
/*##########################################################################################*/
/*##########################################################################################*/

#if 0

static void z_ParseInfo (InfoStringT DEF, InfoStringT USR,
                         BOOLEAN *cont, Z_ZielbehandlungsModulT *ZielModul)
{
   /* Wenn Paramodulation benutzt wird, dann wird NormaleZiele genommen,
      ansonsten wird - wenn nichts anderes eingegeben - NormaleZiele
      genommen */
   switch (IS_FindOption (DEF, USR, 3, "snf", "rnf", "mnf")) {
   default:
   case 0:
      *ZielModul = Z_NormaleZiele;
      break;
   case 1:
      *ZielModul = Z_Repeat;
      break;
   case 2:
      *ZielModul = Z_MNF_Ziele;
      break;
   }
   
   switch (IS_FindOption(DEF, USR, 2, "stop", "continue")){
   case 0: *cont = FALSE;break;
   case 1: default: *cont = TRUE; break;
   }
   switch (IS_FindOption(DEF, USR, 1, "xstat")){
   case 0:
     statistikAfterProved = TRUE;
     break;
   default:
     statistikAfterProved = FALSE;
   }
   
#if 0 && !COMP_POWER
   printf("\n* GOALS:   %s after success. Module: ", 
           *cont?"continue":"stop");
   switch (*ZielModul) {
   case Z_NormaleZiele:
      printf("Normal");
      break;
   case Z_MNF_Ziele:
      printf("MNF");
      break;
   case Z_NichtDefiniert:
      printf("Not specified ");
      break;
   case Z_Paramodulation:
      break;
   case Z_Repeat:
      printf("RNF");
      break;
   }
   if (PM_Existenzziele())
      printf(" with Paramodulation");
   printf(".\n");
#endif
}

void Z_ParamInfo(ParamInfoArtT what, InfoStringT DEF, InfoStringT USR)
{
   BOOLEAN cont; 
   Z_ZielbehandlungsModulT ZielModul;
   

   switch (what) {
   case ParamEvaluate:
      z_ParseInfo (DEF, USR, &cont, &ZielModul);
      switch (ZielModul) {
      case Z_NormaleZiele:
      case Z_Repeat:
         break;
      case Z_MNF_Ziele:
         MNFZ_ParamInfo (what, DEF, USR);
         break;
      case Z_NichtDefiniert:
      case Z_Paramodulation:
         our_error ("ZielModul is not defined. :-(");
         break;
      }
      printf("\n");
      break;
   case ParamExplain:
      printf("Treatment of Goals\n");
      printf("******************\n");
      printf("\n");
      printf("1. [continue|stop]\n");
      printf("2. [xstat]\n");
      printf("\n");
      printf("1. After the first goal has been proved, Waldmeister will\n");
      printf("continue resp. stop.\n");
      printf("2. Output statistics after every proved goal.\n\n");
      printf("Classical Goal Treatment (without MNF)\n");
      printf("**************************************\n");
      printf("To select: snf (keeps goals always reduced)\n");
      printf("To select: rnf (tries to reduce initial goal)\n");
      MNFZ_ParamInfo (what, DEF, USR);   
      break;
   default:
      our_error ("Zielverwaltung: Z_ParamInfo: case Label fehlt");
      break;
   }
}

BOOLEAN SG_ParamInfo(ParamInfoArtT what, InfoStringT DEF, InfoStringT USR, BOOLEAN fuerCPs)
{
    printf("Symbol-Weights\n**************\n\n"
            "Assign each symbol a different individual weight (default is 1). This works for\n"
            "both critical pairs and critical goals. Variables cannot be assigned\n"
            "individual weights, but can be assigned a value different from 1 by VAR=<value>.\n"
            "The default value for non-variable symbols can be changed by DEF=<value>.\n"
            "Other symbols can be assigned values by <symbol>=<value>.\n\n"
            "Example: DEF=2:VAR=5:f=5:g=0\n\n");
    return TRUE;
}


/* Dieses Makro soll die Auswertung der Parameter uebernehmen. Die auszufuehrenden
   Operationen werden als Bloecke uebergeben.*/

#define KPS_Auswertung(PERMSUB, NFR, NFE, SUB)                                        \
{                                                                                               \
  PERMSUB=FALSE;NFR=NFE=SUB=FALSE;                                                            \
                                                                                                \
  if (!COMP_POWER)                                                                              \
    printf("\n* TREATMENT OF CPS AFTER SELECTION:  ");                               \
                                                                                                \
  if (IS_FindOption(DEF, USR, 1, "p") == 0) PERMSUB  = TRUE;                                    \
  if (IS_FindOption(DEF, USR, 1, "s") == 0) SUB = TRUE;                                        \
  if (IS_FindOption(DEF, USR, 1, "r") == 0) NFR = TRUE;                                        \
  if (IS_FindOption(DEF, USR, 1, "e") == 0) NFE = TRUE;                                        \
  if (!COMP_POWER) {                                                                            \
    printf(" %s%s", NFR?"R":"-", NFE?"E":"-");                                     \
    printf("%s", SUB?"S":"-");                                                       \
    printf("%s", PERMSUB?"P":"-");                                                   \
    printf("\n");                                                                     \
   }                                                                                            \
}

void KPS_ParamInfo (ParamInfoArtT what, InfoStringT DEF, InfoStringT USR)
{
  if (what == ParamEvaluate){
    BOOLEAN PSUBInfo;
    BOOLEAN Rinfo, Einfo, Sinfo;
    KPS_Auswertung(PSUBInfo, Rinfo, Einfo, Sinfo);
  }
  else{
    IO_DruckeKurzUeberschrift("Treatment of CPs after Selection", '-');
    printf("1. [r]\n");
    printf("2. [e]\n");
    printf("3. [s]\n");
    printf("4. [p]\n");
    printf("\n");
    printf("Explanation:\n");
    printf("\n");
    printf("1-3.  These letters cause Waldmeister to normalize newly generated\n");
    printf("      critical pairs with _r_ules and/or _e_quations, with or without\n");
    printf("      additional _s_ubsumption\n");
    printf("4.    Suppress redundant AC provable rules and equations.\n\n");
  }
}

void KPB_ParamInfo(ParamInfoArtT what, InfoStringT DEF, InfoStringT USR)
{
   if (what == ParamEvaluate){
      BOOLEAN dokgR, dokgE, dokgS, dokgP;
      
      dokgR = (IS_FindOption(DEF, USR, 1, "r") == 0);
      dokgE = (IS_FindOption(DEF, USR, 1, "e") == 0);
      dokgS = (IS_FindOption(DEF, USR, 1, "s") == 0);
      dokgP = (IS_FindOption(DEF, USR, 1, "p") == 0);

      printf("\n* TREATMENT OF CPS ON GENERATION:    ");

      if (dokgR || dokgE || dokgS || dokgP){
	if (dokgR) printf("R");
	if (dokgE) printf("E");
	if (dokgS) printf("S");
	if (dokgP) printf("P");
	printf("\n");
      }
      else 
	printf("-\n");
   }
   else{
      printf("Treatment of CPs after Generation\n");
      printf("*********************************\n");
      printf("\n");
      printf("1. [r]\n");
      printf("2. [e]\n");
      printf("3. [s]\n");
      printf("4. [p]\n");
      printf("\n");
      printf("Explanation:\n");
      printf("\n");
      printf("1-3. These letters cause Waldmeister to normalize newly generated\n");
      printf("     critical pairs with _r_ules and/or _e_quations, with or without\n");
      printf("     additional _s_ubsumption\n");
      printf("4.   This option tells Waldmeister to discard AC-redundant equations.\n");
   }
}


/* -------------------------------------------------------------------------------*/
/* --------------- Initialisieren ------------------------------------------------*/
/* -------------------------------------------------------------------------------*/

 






void SUE_ParamInfo(ParamInfoArtT what, int whichModule,
                   InfoStringT DEFsue,    InfoStringT USRsue,
                   InfoStringT DEFclas,   InfoStringT USRclas,
                   InfoStringT DEFreclas, InfoStringT USRreclas,
                   InfoStringT DEFcgclas, InfoStringT USRcgclas,
                   InfoStringT DEFpq MAYBE_UNUSEDPARAM,     InfoStringT USRpq MAYBE_UNUSEDPARAM,
                   InfoStringT DEFtpr,    InfoStringT USRtpr,
		   InfoStringT DEFcg,    InfoStringT USRcg)
{
  /*to avoid compiler Warnings*/if ((DEFpq) || (USRpq)){}
   if ((whichModule==0) || (whichModule==99)){                /* SUE */
     if (what == ParamEvaluate){
       unsigned long int infoMod, infoTh;
       Auswertung(&infoMod, &infoTh, DEFsue, USRsue);
     }
     else{
       IO_DruckeKurzUeberschrift("Set of unselected equations", '-');
       printf("[cgcp=<n1>.<n2>]\n");
       printf("Intervall for paramodulation with existentially quantified goals.\n");
     }
   }

   if ((whichModule==1) || (whichModule==2) ||(whichModule==3))
     C_ParamInfo(what, whichModule, DEFclas, USRclas, DEFreclas, USRreclas, DEFcgclas, USRcgclas);
   if (whichModule == 99) {   
      C_ParamInfo(what, 1, DEFclas, USRclas, DEFreclas, USRreclas, DEFcgclas, USRcgclas);
      C_ParamInfo(what, 2, DEFclas, USRclas, DEFreclas, USRreclas, DEFcgclas, USRcgclas);
      C_ParamInfo(what, 3, DEFclas, USRclas, DEFreclas, USRreclas, DEFcgclas, USRcgclas);
   }
   
}
      
#endif

static void pa_IO_ParamInfo(ParamInfoArtT what, int level)
{
  if (what == ParamEvaluate){
    printf("\n* OUTPUT:  Level %d\n", level);
  }
  else{
    printf("-a (0..5)\n");
  }
}


static void pa_NF_ParamInfo(ParamInfoArtT what, InfoStringT DEF, InfoStringT USR)
/* what == ParamEvaluate: Auswerten und Ausgeben, aber nix einstellen.
   what == ParamExplain: Syntax ausgeben.
*/
{
   if (what == ParamEvaluate){
     char *name = NULL;

     switch (IS_FindOption(DEF, USR, 3, "outermost", "innermost", "mixmost")){
     case 0:
       name = "Leftmost-Outermost";
       break;
     case 1:
       name = "Leftmost-Innermost";
       break;
     case 2:
       name = "Mix-Most";
       break;
     default:
       our_error("Nicht implementierte NF-Variante!\n");
       break;
     }
     printf("\n* NORMALIZATION:    %s\n", name);
   }
   else{
      IO_DruckeKurzUeberschrift("Normalization", '-');
      printf("[outermost|innermost|mixmost]\n");
      printf("\n");
      printf("\n");
      printf("Strategy\n");
      printf("\n");
      printf("outermost   = leftmost-outermost\n");
      printf("innermost   = leftmost-innermost\n");
      printf("mixmost     = strategy with good behavior in practice\n");
      printf("\n");
   }      
}   

static void pa_PCL_ParamInfo(ParamInfoArtT what, InfoStringT DEF, InfoStringT USR)
{
  char *arg = NULL;

  if (1||COMP_POWER) return;

  if (what == ParamEvaluate){
    if ((arg = IS_ValueOfOption(DEF, USR, "iPCL"))){
      printf("\n* PCL: write integrated PCL to %s", arg);
    }
    else if (IS_FindOption(DEF, USR, 1, "iPCL") == 0){
      printf("\n* PCL: write integrated PCL to <stdout>");
    }
    else if ((arg = IS_ValueOfOption(DEF, USR, "sPCL"))){
      printf("\n* PCL: write standard PCL (with chaining) to %s", arg);
    }
    else if ((arg = IS_ValueOfOption(DEF, USR, "fPCL"))){
      printf("\n* PCL: write full PCL (without chaining) to %s", arg);
    }
    else {
      printf("\n* PCL: off\n"); 
      
      return;
    }
  }
  else 
    printf ("possible settings:\n"
	    " iPCL[=<file>]   write integrated PCL (i.e. extracted) to <file> or <stdout>\n"
	    " sPCL=<file>     write standard PCL (i.e. with chaining) to <file>\n"
            " fPCL=<file>     write full PCL (i.e. every step!) to <file> \n"
	    "\n");
}

static void pa_U1_ParamInfo(ParamInfoArtT what, char* KapurString)
{
   if (what == ParamEvaluate){
      int KapurStaerke = 0;
      printf("\n* SUBCONNECTEDNESS CRITERION:   ");
      if (strchr(KapurString, 'r'))
         KapurStaerke += 1;
      if (strchr(KapurString, 'e'))
         KapurStaerke += 2;
      switch (KapurStaerke){
      case 1:
         printf("R\n");
         break;
      case 2:
         printf("E\n");
         break;
      case 3:
         printf("RE\n");
         break;
      case 0:
      default:
         printf("-\n");
         break;
      }
   }
   else{
      printf("Subconnectedness Criterion\n");
      printf("**************************\n");
      printf("\n");
      printf("[r][e]\n");
      printf("\n");
      printf("Strength of the subconnectedness-criterion:\n");
      printf("  'r':  application of rules,\n");
      printf("  'e':  application of equations,\n");
      printf("  're': application of rules and equations.  \n");
   }
}

/* ------------------------------------------------------------------------*/
/* -------------------- Initialisierung -----------------------------------*/
/* ------------------------------------------------------------------------*/

#if 0
void MNF_ParseInfoStrings (InfoStringT DEF, InfoStringT USR,
                           BOOLEAN *useE, BOOLEAN *useR, MNFPQ_ArtT *MNFPQ_Art,
                           unsigned int *maxSize, unsigned int *maxGlSchritte,
                           BOOLEAN *TryAllElements, 
                           unsigned int *maxAntiRSchritte,
                           MNF_AntiT *antiKind,
                           BOOLEAN *useAnalyse, unsigned int *SwitchIrred,
                           unsigned int *SwitchNF, unsigned int *CountNF,
                           HashSizeT *HashSize)
{
   char          *valueStr, *token;
   unsigned long tokenValue;
   
   *useE=TRUE;
   *useR=TRUE;
   *MNFPQ_Art = MNFPQ_stack;
   *maxSize = 0;
   *maxGlSchritte = 0;
   *TryAllElements=TRUE;
   *maxAntiRSchritte = 0;
   *antiKind = noAnti;
   *useAnalyse = FALSE;
   *SwitchIrred = 0;
   *SwitchNF=0;
   *CountNF=0;
   *HashSize=1023;

   valueStr = IS_ValueOfOption (DEF, USR, "mnf");
   /* valueStr ist leer, defaults uebernehmen */
   if (!valueStr)
     return;

   token = valueStr;
   while (token != NULL) {
      if ((strncmp (token, "re", 2) == 0) || (strncmp(token, "er", 2) == 0)) {
         *useR=TRUE;
         *useE=TRUE;
      }
      else if (strncmp(token, "e", 1) == 0) {
         *useR=FALSE;
         *useE=TRUE;
      }
      else if (strncmp(token, "r", 1) == 0) {
         *useR=TRUE;
         *useE=FALSE;
      }
      else if (strncmp(token, "stack", 5) == 0) {
         *MNFPQ_Art = MNFPQ_stack;
      }
      else if (strncmp(token, "deque", 5) == 0) {
         *MNFPQ_Art = MNFPQ_deque;
      }
      else if (strncmp(token, "fifo", 4) == 0) {
         *MNFPQ_Art = MNFPQ_fifo;
      }
      else if (sscanf(token, "%ld", &tokenValue) == 1) {
         *maxSize = tokenValue;
      }
      else {
        fprintf(stderr, "cannot parse mnf option: >>%s<<\n", token);
        our_error("wrong arguments for mnf option");
      }
      token = strchr(token,'.');
      if (token != NULL) token++;
   }

   if (IS_FindValuedOption (DEF, USR, 1, "mnfup") == 0) {
      valueStr = IS_ValueOfOption (DEF, USR, "mnfup");
      if (valueStr) {
         tokenValue = strtoul (valueStr, NULL, 10);
         *maxGlSchritte = tokenValue;
      }
   }
   
   if (IS_FindValuedOption (DEF, USR, 1, "mnfantir") == 0) {
      valueStr = IS_ValueOfOption (DEF, USR, "mnfantir");
      if (valueStr) {
         tokenValue = strtoul (valueStr, NULL, 10);
         *maxAntiRSchritte = tokenValue;
         *antiKind = antiWOVar;
      }
   }
   if (IS_FindValuedOption (DEF, USR, 1, "mnfanalysis") == 0) {
      valueStr = IS_ValueOfOption (DEF, USR, "mnfanalysis");
      *useAnalyse = TRUE;

      token = valueStr;
      if (token != NULL) {
         *SwitchIrred = strtoul (token, NULL, 10);
         token = strchr(token,'.');
         if (token != NULL) token++;
      }
      if (token != NULL) {
         *SwitchNF = strtoul (token, NULL, 10);
         token = strchr(token,'.');
         if (token != NULL) token++;
      }
      if (token != NULL) {
         *CountNF = strtoul (token, NULL, 10);
      }
   }
   if (IS_FindOption (DEF,USR, 1, "mnftrynotall") == 0) {
      *TryAllElements = FALSE;
   }

   if (IS_FindOption (DEF,USR, 1, "mnfantiWOVar") == 0) {
      *antiKind = antiWOVar;
   }
   if (IS_FindOption (DEF,USR, 1, "mnfantiBoundVar") == 0) {
      *antiKind = antiBoundVar;
   }
   if (IS_FindOption (DEF,USR, 1, "mnfantiFreeVar") == 0) {
      *antiKind = antiFreeVar;
   }
   if (IS_FindOption (DEF, USR, 1, "stack") == 0) {
      *MNFPQ_Art = MNFPQ_stack;
   } 
   if (IS_FindOption (DEF, USR, 1, "deque") == 0) {
      *MNFPQ_Art = MNFPQ_deque;
   }
   if (IS_FindOption (DEF, USR, 1, "fifo") == 0) {
      *MNFPQ_Art = MNFPQ_fifo;
   }

   if (IS_FindValuedOption (DEF, USR, 1, "hash") == 0) {
      valueStr = IS_ValueOfOption (DEF, USR, "hash");
      if (valueStr) {
         tokenValue = strtoul (valueStr, NULL, 10);
         *HashSize = tokenValue;
      }
   }
}

void MNF_PrintOptions (BOOLEAN useE, BOOLEAN useR, MNFPQ_ArtT MNFPQ_Art,
                       unsigned int maxSize, unsigned int maxGlSchritte,
                       BOOLEAN AddTryAllElements, 
                       unsigned int maxAntiRSchritte,
                       MNF_AntiT antiKind,
                       BOOLEAN useAnalyse, unsigned int SwitchIrred,
                       unsigned int SwitchNF, unsigned int CountNF,
                       HashSizeT initialHashSize)
{
   printf("    MNF: ");
   if (useE)
      printf("Equations, ");
   if (useR)
      printf("Rules, ");
   if (maxSize != 0)
      printf("MaxSize=%u, ", maxSize);
   if (maxGlSchritte != 0)
      printf("Max Up=%u, ", maxGlSchritte);
   if (maxAntiRSchritte != 0) {
      printf("Try Rules Up=%u ", maxAntiRSchritte);
      switch (antiKind) {
      case antiWOVar:
         printf(" (rules without vars)");
         break;
      case antiBoundVar:
         printf(" (rules without new vars on lhs)");
         break;
      case antiFreeVar:
         printf(" (all rules)");
         break;
      default:
         break;
      }
      printf(", ");
   }
   if (useAnalyse)
      printf("Analysis: %u:%u:%u, ",
               SwitchIrred, SwitchNF, CountNF);
   if (!AddTryAllElements)
      printf("Expand only irreduc. Elements");
   switch (MNFPQ_Art) {
   case MNFPQ_stack:
      printf("PQ: Stack");
      break;
   case MNFPQ_deque:
      printf("PQ: Deque");
      break;
   case MNFPQ_fifo:
      printf("PQ: Fifo");
      break;
   }
   printf(", initial hash size: %d", initialHashSize);
   printf("\n");
}


void MNF_PrintOptionsMenge (MNF_MengeT *menge)
{
   MNF_PrintOptions (menge->useE, menge->useR, menge->MNFPQ_Art, 
                     menge->maxSize,
                     menge->maxGlSchritte, menge->MNF_AddTryAllElements,
                     menge->maxAntiRSchritte,
                     menge->antiKind,
                     menge->useAnalyse,
                     menge->SwitchIrred, menge->SwitchNF,
                     menge->CountNF, menge->initialHashSize);
}


void MNF_ParamInfo (ParamInfoArtT what, InfoStringT DEF, InfoStringT USR) 
{
   BOOLEAN             useE;
   BOOLEAN             useR;
   BOOLEAN             tryAllElements;
   BOOLEAN             useAnalyse;
   unsigned int        SwitchIrred;
   unsigned int        SwitchNF;
   unsigned int        CountNF;
   MNFPQ_ArtT          MNFPQ_Art;
   unsigned int        maxSize;
   unsigned int        maxGlSchritte;
   unsigned int        maxAntiRSchritte;
   MNF_AntiT           antiKind;
   HashSizeT           initialHashSize;
   
   switch (what) {
   case ParamEvaluate:
      MNF_ParseInfoStrings (DEF, USR, &useE, &useR, &MNFPQ_Art,
                            &maxSize, &maxGlSchritte, &tryAllElements,
                            &maxAntiRSchritte,
                            &antiKind,
                            &useAnalyse,
                            &SwitchIrred, &SwitchNF, &CountNF,
                            &initialHashSize);
      MNF_PrintOptions (useE, useR, MNFPQ_Art, maxSize, maxGlSchritte,
                        tryAllElements, maxAntiRSchritte,
                        antiKind,
                        useAnalyse,
                        SwitchIrred, SwitchNF, CountNF,
                        initialHashSize);
      break;
   case ParamExplain:
      printf("Multiple Normalforms\n");
      printf("********************\n");
      printf("To select: mnf=[r|e|re].<n>[:additional paramameter]*\n");
      printf("  <n>: Maximal size of MNF set\n");
      printf("  e  : use only equations\n");
      printf("  r  : use only rules\n");
      printf("  re : use rules and equations (default)\n");
      printf(" additional parameter:\n");
      printf("  [stack|deque|fifo] : priority queue (stack is default)\n");
      printf("  [hash=<n>]     : initial hash size (default 1023)\n");
      printf("  [mnfup=<n>]    : max. number of steps up with equations "
             "(default 0)\n");
      printf("  [mnfantir=<n>] : max. number of steps up with rules "
             "(default 0)\n");
      printf("  [mnfantiWOVar|mnfantiBoundVar|mnfantiFreeVar] :\n");
      printf("   class of rules to try steps up with\n");
      printf("  [mnftrynotall] : expand only new and irreducible terms\n");
      printf("  [mnfanalysis=<n1>.<n2>.<n3>] : parameter for analysis to "
             "limit size of set,\n");
      printf("   n1: no. of elements for switching to expand only new and "
             "irreducible terms\n");
      printf("   n2: no. of elements for switching to reduce with normal "
             "strategy\n");
      printf("   n3: no. of elements to reduce with normal strategy for "
             "each side\n");
      break;
   }
}

#endif


static BOOLEAN Vervollstaendigung = TRUE;

#define HoechsterSichtbarerIndex(option) (AufrufOptionen[option].ParameterTyp.zweistellig.AnzahlVerschiedenerWerte)


void PA_InitApriori(void)
/* Aufruf-Optionen besetzen.*/
{
  int i;

  if (WM_ErsterAnlauf()) {
#if COMP_COMP
    PA_Paras.Automodus = TRUE;
#else
    PA_Paras.Automodus = FALSE;
#endif
  }

/*------ Arbeitsmodi-------------------------------------*/

   AufrufOptionen[OptNrFailing].Kuerzel           = "-f";
   AufrufOptionen[OptNrFailing].Name              = "'f'ailing completion";
   AufrufOptionen[OptNrFailing].Bedeutung         = "Abbruch beim Auftreten unrichtbarer kritischer Paare";
   AufrufOptionen[OptNrFailing].ParameterTyp.einstellig.BedeutungT        = "Failing Completion";
   AufrufOptionen[OptNrFailing].ParameterTyp.einstellig.BedeutungF        = "Unfailing Completion";
   AufrufOptionen[OptNrFailing].ParameterArt      = 1;
   AufrufOptionen[OptNrFailing].ParameterTyp.einstellig.defaultWert = FALSE;

/*------ SUE-Statistics ---------------------------------*/

   AufrufOptionen[OptNrSUE].Kuerzel           = "-sue";
   AufrufOptionen[OptNrSUE].Name              = "'SUE'";
   AufrufOptionen[OptNrSUE].Bedeutung         = "SUEM.";
   AufrufOptionen[OptNrSUE].ParameterArt      = 3;
   AufrufOptionen[OptNrSUE].ParameterTyp.InfoString.DefaultInfoString = "";
      
/*------ Classification ---------------------------------*/

   AufrufOptionen[OptNrClassification].Kuerzel           = "-clas";
   AufrufOptionen[OptNrClassification].Name              = "'Clas'sification";
   AufrufOptionen[OptNrClassification].Bedeutung         = "Classification of unselected equations. Syntax:\n"
      "heuristic=[addweight|cp-in-goal|glam|goal-in-cp|gtweight|\n"
      "           mixweight|maxweight|unif]                   --- primary heuristic\n"
      "DEF=n1:VAR=n2:f=n3:g=n4...:                            --- symbol weights\n"
      "statistics=[on|off]:                                   --- statistics\n" 
      "killer[R|E|RE]=[regular|half|ultimate|double|last]:    --- killer criterion\n"
      "echild=[regular|half|ultimate|double|last]:            --- children of equations\n"
      "initial=[regular|half|ultimate|double|last]:           --- initial equations\n"
      "killedRE=[regular|half|ultimate|double|last]:          --- killed rules/equations\n"
      "statint=n                                              --- intervall for time distribution of weights\n";
   AufrufOptionen[OptNrClassification].ParameterArt      = 3;
   AufrufOptionen[OptNrClassification].ParameterTyp.InfoString.DefaultInfoString = "DEF=1:VAR=1:"
      "heuristic=mixweight:initial=ultimate:database=ultimate:statistics=off:statint=1";
      
   AufrufOptionen[OptNrReClassification].Kuerzel           = "-reclas";
   AufrufOptionen[OptNrReClassification].Name              = "'Re-Clas'sification";
   AufrufOptionen[OptNrReClassification].Bedeutung         = "Reclassification of unselected equations during SUE-. Syntax:\n"
     "killer[R|E|RE]=[regular|half|ultimate|double|penalty]:\n"
     "echild=[regular|half|ultimate|double|penalty]:\n"
     "perm[3|4|5|6]=[regular|half|ultimate|double|penalty]\n"; 
   AufrufOptionen[OptNrReClassification].ParameterArt      = 3;
   AufrufOptionen[OptNrReClassification].ParameterTyp.InfoString.DefaultInfoString = "";

   AufrufOptionen[OptNrCGClassification].Kuerzel           = "-cgclas";
   AufrufOptionen[OptNrCGClassification].Name              = "'CG-Clas'sification";
   AufrufOptionen[OptNrCGClassification].Bedeutung         = "classification of critical goals. Syntax like clas\n"; 
   AufrufOptionen[OptNrCGClassification].ParameterArt      = 3;
   AufrufOptionen[OptNrCGClassification].ParameterTyp.InfoString.DefaultInfoString = 
      "g1=prod.plus.length.plus.unif:g2=fifo:def=2";

   AufrufOptionen[OptNrCGInterreduktion].Kuerzel           = "-cg";
   AufrufOptionen[OptNrCGInterreduktion].Name              = "'CG'-set normalization";
   AufrufOptionen[OptNrCGInterreduktion].Bedeutung         = "interreduction of critical goals. Syntax similar to ki\n"; 
   AufrufOptionen[OptNrCGInterreduktion].ParameterArt      = 3;
   AufrufOptionen[OptNrCGInterreduktion].ParameterTyp.InfoString.DefaultInfoString = "";
      
/*------ Priority Queue ---------------------------------*/

   AufrufOptionen[OptNrPriorityQueue].Kuerzel           = "-pq";
   AufrufOptionen[OptNrPriorityQueue].Name              = "'P'riority-'Q'ueue";
   AufrufOptionen[OptNrPriorityQueue].Bedeutung         = "'P'riority-'Q'ueue. Syntax:\n"
      "statistics=[on|off]:\n"
      "domain=Granularity";
   AufrufOptionen[OptNrPriorityQueue].ParameterArt      = 3;
   AufrufOptionen[OptNrPriorityQueue].ParameterTyp.InfoString.DefaultInfoString = 
      normal_power("heap:e1:domain=30:statistics=on","heap:e1:domain=30:statistics=off");
   /* XXX capacity=20000 */

/*------ Termpair-Representation ------------------------*/

   AufrufOptionen[OptNrTermpairRepresentation].Kuerzel           = "-tpr";
   AufrufOptionen[OptNrTermpairRepresentation].Name              = "'T'erm'p'air-'r'epresentation";
   AufrufOptionen[OptNrTermpairRepresentation].Bedeutung         = "Syntax:\n"
      "[packed|implicit]";
   AufrufOptionen[OptNrTermpairRepresentation].ParameterArt      = 3;
   AufrufOptionen[OptNrTermpairRepresentation].ParameterTyp.InfoString.DefaultInfoString = 
      "packed";

/*------ CP-Criteria ------------------------------------*/

   AufrufOptionen[OptNrKapur].Kuerzel           = "-subcon";
   AufrufOptionen[OptNrKapur].Name              = "'Subcon'nectedness-criterion";
   AufrufOptionen[OptNrKapur].Bedeutung         = "Vermeidung unnoetiger KPs. Argument = Staerke des Tests\n"
      "['r'|'e']" ;
   AufrufOptionen[OptNrKapur].ParameterArt      = 0;
   AufrufOptionen[OptNrKapur].ParameterTyp.frei.wurdeAngegeben = FALSE;
   AufrufOptionen[OptNrKapur].ParameterTyp.frei.defaultWert = "";

/*------ Eins-Stern -------------------------------------*/

   AufrufOptionen[OptNrEinsStern].Kuerzel           = "-einsstern";
   AufrufOptionen[OptNrEinsStern].Name              = "'Eins-Stern'-Kriterium";
   AufrufOptionen[OptNrEinsStern].Bedeutung         = "nur kritische Paare an 1* betrachten";
   AufrufOptionen[OptNrEinsStern].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrEinsStern].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrEinsStern].ParameterArt      = 1;
   AufrufOptionen[OptNrEinsStern].ParameterTyp.einstellig.defaultWert = FALSE;


/*------ Nusfu ------------------------------------------*/

   AufrufOptionen[OptNrNusfu].Kuerzel           = "-nusfu";
   AufrufOptionen[OptNrNusfu].Name              = "Nusfu-Kriterium";
   AufrufOptionen[OptNrNusfu].Bedeutung         = "nicht unterhalb von Skolemfunktionen ueberlappen";
   AufrufOptionen[OptNrNusfu].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrNusfu].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrNusfu].ParameterArt      = 1;
   AufrufOptionen[OptNrNusfu].ParameterTyp.einstellig.defaultWert = TRUE;

/*------ Kern -------------------------------------------*/

   AufrufOptionen[OptNrKern].Kuerzel           = "-kern";
   AufrufOptionen[OptNrKern].Name              = "'Kern'-Strategie";
   AufrufOptionen[OptNrKern].Bedeutung         = "auf den Kopf gestellte Vervollstaendigung";
   AufrufOptionen[OptNrKern].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrKern].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrKern].ParameterArt      = 1;
   AufrufOptionen[OptNrKern].ParameterTyp.einstellig.defaultWert = FALSE;

/*------ Golem ------------------------------------------*/

   AufrufOptionen[OptNrGolem].Kuerzel           = "-golem";
   AufrufOptionen[OptNrGolem].Name              = "'Golem";
   AufrufOptionen[OptNrGolem].Bedeutung         = "";
   AufrufOptionen[OptNrGolem].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrGolem].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrGolem].ParameterArt      = 1;
   AufrufOptionen[OptNrGolem].ParameterTyp.einstellig.defaultWert = TRUE;


/*------ Back ------------------------------------------*/

   AufrufOptionen[OptNrBack].Kuerzel           = "-back";
   AufrufOptionen[OptNrBack].Name              = "'back'ward reasoning";
   AufrufOptionen[OptNrBack].Bedeutung         = "Rueckwaertsargumentieren bei kritischen Zielen";
   AufrufOptionen[OptNrBack].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrBack].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrBack].ParameterArt      = 1;
   AufrufOptionen[OptNrBack].ParameterTyp.einstellig.defaultWert = FALSE;

/*------ Cancellation ----------------------------------*/

   AufrufOptionen[OptNrCancellation].Kuerzel           = "-cc";
   AufrufOptionen[OptNrCancellation].Name              = "cancellation";
   AufrufOptionen[OptNrCancellation].Bedeutung         = 
     "Say in group theory, if + and - are declared cancellative, \n"
     "then simplify e.g. s+(-t)=s+(-t') to t=t'.";
   AufrufOptionen[OptNrCancellation].ParameterArt      = 3;
   AufrufOptionen[OptNrCancellation].ParameterTyp.InfoString.DefaultInfoString = "";

/*------ Grundzusammenfuehrbarkeit ---------------------*/

   AufrufOptionen[OptNrGZFB].Kuerzel           = "-gj";
   AufrufOptionen[OptNrGZFB].Name              = "ground joinability";
   AufrufOptionen[OptNrGZFB].Bedeutung         = "use ground joinability as redundancy criterion";
   AufrufOptionen[OptNrGZFB].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrGZFB].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrGZFB].ParameterArt      = 1;
   AufrufOptionen[OptNrGZFB].ParameterTyp.einstellig.defaultWert = FALSE;

/*------ ACGrundzusammenfuehrbarkeit ---------------------*/

   AufrufOptionen[OptNrACG].Kuerzel           = "-acg";
   AufrufOptionen[OptNrACG].Name              = "AC ground redundancy";
   AufrufOptionen[OptNrACG].Bedeutung         = "use AC ground reducibility as redundancy criterion";
   AufrufOptionen[OptNrACG].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrACG].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrACG].ParameterArt      = 1;
   AufrufOptionen[OptNrACG].ParameterTyp.einstellig.defaultWert = TRUE;

/*------ ACGrundzusammenfuehrbarkeit (Strenge Form)---------------------*/

   AufrufOptionen[OptNrSK].Kuerzel           = "-sk";
   AufrufOptionen[OptNrSK].Name              = "strong AC ground redundancy (implies -acg on)";
   AufrufOptionen[OptNrSK].Bedeutung         = "use AC ground reducibility as redundancy criterion";
   AufrufOptionen[OptNrSK].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrSK].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrSK].ParameterArt      = 1;
   AufrufOptionen[OptNrSK].ParameterTyp.einstellig.defaultWert = TRUE;

/*------ ACGrundzusammenfuehrbarkeit check---------------------*/

   AufrufOptionen[OptNrACGSKCheck].Kuerzel           = "-acgskcheck";
   AufrufOptionen[OptNrACGSKCheck].Name              = "check various aspects when AC ground redundancy enabled";
   AufrufOptionen[OptNrACGSKCheck].Bedeutung         = "use AC ground reducibility as redundancy criterion";
   AufrufOptionen[OptNrACGSKCheck].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrACGSKCheck].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrACGSKCheck].ParameterArt      = 1;
   AufrufOptionen[OptNrACGSKCheck].ParameterTyp.einstellig.defaultWert = FALSE;

/*------ ACGrundzusammenfuehrbarkeit rechts -------------------*/

   AufrufOptionen[OptNrACGRechts].Kuerzel           = "-acgRechts";
   AufrufOptionen[OptNrACGRechts].Name              = "use additionally right hand sides for acg";
   AufrufOptionen[OptNrACGRechts].Bedeutung         = "use additionally right hand sides for acg";
   AufrufOptionen[OptNrACGRechts].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrACGRechts].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrACGRechts].ParameterArt      = 1;
   AufrufOptionen[OptNrACGRechts].ParameterTyp.einstellig.defaultWert = TRUE;

/*------ ACGrundzusammenfuehrbarkeit rechts -------------------*/

   AufrufOptionen[OptNrSKRechts].Kuerzel           = "-skRechts";
   AufrufOptionen[OptNrSKRechts].Name              = "use additionally right hand sides for sk";
   AufrufOptionen[OptNrSKRechts].Bedeutung         = "use additionally right hand sides for sk";
   AufrufOptionen[OptNrSKRechts].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrSKRechts].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrSKRechts].ParameterArt      = 1;
   AufrufOptionen[OptNrSKRechts].ParameterTyp.einstellig.defaultWert = TRUE;

/*------ neues Permsub (AC) -----------------------------*/

   AufrufOptionen[OptNrPermsubAC].Kuerzel           = "-permsubAC";
   AufrufOptionen[OptNrPermsubAC].Name              = "Permsub";
   AufrufOptionen[OptNrPermsubAC].Bedeutung         = "neues permsub, nur AC";
   AufrufOptionen[OptNrPermsubAC].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrPermsubAC].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrPermsubAC].ParameterArt      = 1;
   AufrufOptionen[OptNrPermsubAC].ParameterTyp.einstellig.defaultWert = FALSE;

/*------ neues Permsub (ACI) ----------------------------*/

   AufrufOptionen[OptNrPermsubACI].Kuerzel           = "-permsubACI";
   AufrufOptionen[OptNrPermsubACI].Name              = "Permsub";
   AufrufOptionen[OptNrPermsubACI].Bedeutung         = "neues permsub, ACI";
   AufrufOptionen[OptNrPermsubACI].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrPermsubACI].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrPermsubACI].ParameterArt      = 1;
   AufrufOptionen[OptNrPermsubACI].ParameterTyp.einstellig.defaultWert = FALSE;

/*------ Statistik Intervall ----------------------------*/

   AufrufOptionen[OptNrStatIntervall].Kuerzel       = "-wct";
   AufrufOptionen[OptNrStatIntervall].Name          = "intervall for statistics";
   AufrufOptionen[OptNrStatIntervall].Bedeutung     = "measured in abstract time";
   AufrufOptionen[OptNrStatIntervall].ParameterArt  = 0;
   AufrufOptionen[OptNrStatIntervall].ParameterTyp.frei.wurdeAngegeben = FALSE;
   AufrufOptionen[OptNrStatIntervall].ParameterTyp.frei.defaultWert = "";

/*------ FullPermLemmata --------------------------------*/

   AufrufOptionen[OptNrFullPermLemmata].Kuerzel       = "-fullPermLemmata";
   AufrufOptionen[OptNrFullPermLemmata].Name          = "all permutative lemmata";
   AufrufOptionen[OptNrFullPermLemmata].Bedeutung     = "all permutative lemmata up to size <n>";
   AufrufOptionen[OptNrFullPermLemmata].ParameterArt  = 0;
   AufrufOptionen[OptNrFullPermLemmata].ParameterTyp.frei.wurdeAngegeben = FALSE;
   AufrufOptionen[OptNrFullPermLemmata].ParameterTyp.frei.defaultWert = "";

/*------ permLemmata ------------------------------------*/

   AufrufOptionen[OptNrPermLemmata].Kuerzel           = "-permLemmata";
   AufrufOptionen[OptNrPermLemmata].Name              = "Permutative Lemmata";
   AufrufOptionen[OptNrPermLemmata].Bedeutung         = "Sprungpermutationen";
   AufrufOptionen[OptNrPermLemmata].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrPermLemmata].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrPermLemmata].ParameterArt      = 1;
   AufrufOptionen[OptNrPermLemmata].ParameterTyp.einstellig.defaultWert = FALSE;

/*------ idempotenz Lemmata -----------------------------*/

   AufrufOptionen[OptNrIdempotenzLemmata].Kuerzel           = "-idempotenzLemmata";
   AufrufOptionen[OptNrIdempotenzLemmata].Name              = "idempotenz Lemmata";
   AufrufOptionen[OptNrIdempotenzLemmata].Bedeutung         = "Erweiterungsregeln Idempotenz";
   AufrufOptionen[OptNrIdempotenzLemmata].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrIdempotenzLemmata].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrIdempotenzLemmata].ParameterArt      = 1;
   AufrufOptionen[OptNrIdempotenzLemmata].ParameterTyp.einstellig.defaultWert = FALSE;

/*------ C' Lemmata -------------------------------------*/

   AufrufOptionen[OptNrC_Lemma].Kuerzel           = "-C_Lemma";
   AufrufOptionen[OptNrC_Lemma].Name              = "C' Lemma";
   AufrufOptionen[OptNrC_Lemma].Bedeutung         = "Erweiterungsgleichung Kommutativitaet";
   AufrufOptionen[OptNrC_Lemma].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrC_Lemma].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrC_Lemma].ParameterArt      = 1;
   AufrufOptionen[OptNrC_Lemma].ParameterTyp.einstellig.defaultWert = FALSE;


/*------ Frischzellenkur --------------------------------*/

   AufrufOptionen[OptNrFrischzellen].Kuerzel           = "-Frischzellen";
   AufrufOptionen[OptNrFrischzellen].Name              = "Frischzellenkur";
   AufrufOptionen[OptNrFrischzellen].Bedeutung         = "Frischzellenkur fuer Aktiva";
   AufrufOptionen[OptNrFrischzellen].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrFrischzellen].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrFrischzellen].ParameterArt      = 1;
   AufrufOptionen[OptNrFrischzellen].ParameterTyp.einstellig.defaultWert = TRUE;

/*------ WeightAxiom ------------------------------------*/

   AufrufOptionen[OptNrWeightAxiom].Kuerzel           = "-weightaxiom";
   AufrufOptionen[OptNrWeightAxiom].Name              = "WeightAxiom";
   AufrufOptionen[OptNrWeightAxiom].Bedeutung         = "Axiome untereinander gewichten";
   AufrufOptionen[OptNrWeightAxiom].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrWeightAxiom].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrWeightAxiom].ParameterArt      = 1;
   AufrufOptionen[OptNrWeightAxiom].ParameterTyp.einstellig.defaultWert = FALSE;

/*------ neue KPV: Terme aufheben -----------------------*/

   AufrufOptionen[OptNrSaveTerms].Kuerzel           = "-saveterms";
   AufrufOptionen[OptNrSaveTerms].Name              = "Save Terms";
   AufrufOptionen[OptNrSaveTerms].Bedeutung         = "save terms or recompute?";
   AufrufOptionen[OptNrSaveTerms].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrSaveTerms].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrSaveTerms].ParameterArt      = 1;
   AufrufOptionen[OptNrSaveTerms].ParameterTyp.einstellig.defaultWert = FALSE;


/*------ Forken oder nicht Forken ----------------------*/

   AufrufOptionen[OptNrFork].Kuerzel           = "-fork";
   AufrufOptionen[OptNrFork].Name              = "fork processes";
   AufrufOptionen[OptNrFork].Bedeutung         = "fork processes when appropriate";
   AufrufOptionen[OptNrFork].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrFork].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrFork].ParameterArt      = 1;
   AufrufOptionen[OptNrFork].ParameterTyp.einstellig.defaultWert = TRUE;

/*------ Behandlung von Waisen -------------------------*/

   AufrufOptionen[OptNrWaisenmord].Kuerzel           = "-ocrit";
   AufrufOptionen[OptNrWaisenmord].Name              = "orphan criterion";
   AufrufOptionen[OptNrWaisenmord].Bedeutung         = "handling of critical pairs with deleted parents";
   AufrufOptionen[OptNrWaisenmord].ParameterTyp.einstellig.BedeutungT        = "delete cp";
   AufrufOptionen[OptNrWaisenmord].ParameterTyp.einstellig.BedeutungF        = "keep cp";
   AufrufOptionen[OptNrWaisenmord].ParameterArt      = 1;
   AufrufOptionen[OptNrWaisenmord].ParameterTyp.einstellig.defaultWert = TRUE;

/*------ Auto-Modus -------------------------------------*/

   /* wird extra behandelt ! */
   AufrufOptionen[OptNrAutomodus].Kuerzel           = "-auto";
   AufrufOptionen[OptNrAutomodus].Name              = "Automodus";
   AufrufOptionen[OptNrAutomodus].Bedeutung         = "automagic";
   AufrufOptionen[OptNrAutomodus].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrAutomodus].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrAutomodus].ParameterArt      = 1;
#if COMP_COMP
   /* wird eigentlich in PA_InitApriori gesetzt */
   AufrufOptionen[OptNrAutomodus].ParameterTyp.einstellig.defaultWert = TRUE;
#else
   AufrufOptionen[OptNrAutomodus].ParameterTyp.einstellig.defaultWert = FALSE;
#endif
/*------ Treatment of CPs -------------------------------*/

   AufrufOptionen[OptNrKPInterreduktion].Kuerzel           = "-ki";
   AufrufOptionen[OptNrKPInterreduktion].Name              = "'K'P-Mengen-'I'nterreduktions";
   AufrufOptionen[OptNrKPInterreduktion].Bedeutung         = "Durchreduzieren von SUE. Syntax:\n"
      "period=<periode>[res]:[res]=<stichpunkt>.<stichpunkt>:[res]=<stichpunkt>.<stichpunkt>...[:statistis=[on|off]]";
   AufrufOptionen[OptNrKPInterreduktion].ParameterArt      = 3;
   AufrufOptionen[OptNrKPInterreduktion].ParameterTyp.InfoString.DefaultInfoString = 
      normal_power("statistics=on","statistics=off");

   AufrufOptionen[OptNrKPgen].Kuerzel           = "-kg";
   AufrufOptionen[OptNrKPgen].Name              = "'K'P-Behandlung 'g'eneration";
   AufrufOptionen[OptNrKPgen].Bedeutung         = "Behandlung der KPs after generation.Syntax\n"
      "r:e:s:p";
   AufrufOptionen[OptNrKPgen].ParameterArt      = 3;
   AufrufOptionen[OptNrKPgen].ParameterTyp.InfoString.DefaultInfoString = "r";

   AufrufOptionen[OptNrKPsel].Kuerzel           = "-ks";
   AufrufOptionen[OptNrKPsel].Name              = "'K'P-Behandlung 's'election";
   AufrufOptionen[OptNrKPsel].Bedeutung         = "Behandlung der KPs after selection.Syntax\n"
      "r:e:s:p";
   AufrufOptionen[OptNrKPsel].ParameterArt      = 3;
   AufrufOptionen[OptNrKPsel].ParameterTyp.InfoString.DefaultInfoString = "r:e:s:p";

/*------ ConfluenceTree Varianten -----------------------*/

   AufrufOptionen[OptNrCTVersion].Kuerzel           = "-CTreeVersion";
   AufrufOptionen[OptNrCTVersion].Name              = "Version of confluence trees";
   AufrufOptionen[OptNrCTVersion].Bedeutung         = "Version of confluence trees";
   AufrufOptionen[OptNrCTVersion].ParameterArt      = 0;
   AufrufOptionen[OptNrCTVersion].ParameterTyp.frei.wurdeAngegeben = FALSE;
   AufrufOptionen[OptNrCTVersion].ParameterTyp.frei.defaultWert = "";

   AufrufOptionen[OptNrMaxNodeCount].Kuerzel           = "-maxNodeCount";
   AufrufOptionen[OptNrMaxNodeCount].Name              = "max. number of nodes in confluence trees";
   AufrufOptionen[OptNrMaxNodeCount].Bedeutung         = "max. number of nodes in confluence trees";
   AufrufOptionen[OptNrMaxNodeCount].ParameterArt      = 0;
   AufrufOptionen[OptNrMaxNodeCount].ParameterTyp.frei.wurdeAngegeben = FALSE;
   AufrufOptionen[OptNrMaxNodeCount].ParameterTyp.frei.defaultWert = "";

   AufrufOptionen[OptNrCTS].Kuerzel           = "-CTreeS";
   AufrufOptionen[OptNrCTS].Name              = "use subsumption steps in confluence trees";
   AufrufOptionen[OptNrCTS].Bedeutung         = "use subsumption steps in confluence trees";
   AufrufOptionen[OptNrCTS].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrCTS].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrCTS].ParameterArt      = 1;
   AufrufOptionen[OptNrCTS].ParameterTyp.einstellig.defaultWert = TRUE;

   AufrufOptionen[OptNrCTP].Kuerzel           = "-CTreeP";
   AufrufOptionen[OptNrCTP].Name              = "use permutative subsumption steps in confluence trees";
   AufrufOptionen[OptNrCTP].Bedeutung         = "use permutative subsumption steps in confluence trees";
   AufrufOptionen[OptNrCTP].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrCTP].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrCTP].ParameterArt      = 1;
   AufrufOptionen[OptNrCTP].ParameterTyp.einstellig.defaultWert = TRUE;

   AufrufOptionen[OptNrSortDCons].Kuerzel           = "-sortDCons";
   AufrufOptionen[OptNrSortDCons].Name              = "in confluence trees: sort elements of disjunctions";
   AufrufOptionen[OptNrSortDCons].Bedeutung         = "in confluence trees: sort elements of disjunctions";
   AufrufOptionen[OptNrSortDCons].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrSortDCons].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrSortDCons].ParameterArt      = 1;
   AufrufOptionen[OptNrSortDCons].ParameterTyp.einstellig.defaultWert = TRUE;

   AufrufOptionen[OptNrOptDCons].Kuerzel           = "-optDCons";
   AufrufOptionen[OptNrOptDCons].Name              = "in confluence trees: optimize splits in disjunctions";
   AufrufOptionen[OptNrOptDCons].Bedeutung         = "in confluence trees: optimize splits in disjunctions";
   AufrufOptionen[OptNrOptDCons].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrOptDCons].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrOptDCons].ParameterArt      = 1;
   AufrufOptionen[OptNrOptDCons].ParameterTyp.einstellig.defaultWert = FALSE;

   AufrufOptionen[OptNrMergeDCons].Kuerzel           = "-mergeDCons";
   AufrufOptionen[OptNrMergeDCons].Name              = "in confluence trees: use merging to optimize splits in disjunctions";
   AufrufOptionen[OptNrMergeDCons].Bedeutung         = "in confluence trees: use merging to optimize splits in disjunctions";
   AufrufOptionen[OptNrMergeDCons].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrMergeDCons].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrMergeDCons].ParameterArt      = 1;
   AufrufOptionen[OptNrMergeDCons].ParameterTyp.einstellig.defaultWert = FALSE;

   AufrufOptionen[OptNrPreferD1].Kuerzel           = "-preferD1";
   AufrufOptionen[OptNrPreferD1].Name              = "tackle rewritten equation first";
   AufrufOptionen[OptNrPreferD1].Bedeutung         = "tackle rewritten equation first";
   AufrufOptionen[OptNrPreferD1].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrPreferD1].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrPreferD1].ParameterArt      = 1;
   AufrufOptionen[OptNrPreferD1].ParameterTyp.einstellig.defaultWert = FALSE;

/*------ NormalformStrategien ---------------------------*/

   AufrufOptionen[OptNrNFBildung].Kuerzel                   = "-nf";
   AufrufOptionen[OptNrNFBildung].Name              = "'N'ormal'f'ormbildung";
   AufrufOptionen[OptNrNFBildung].Bedeutung = "Normalization-Strategy and Backtracking-Behaviour.";
   AufrufOptionen[OptNrNFBildung].ParameterArt = 3;
   AufrufOptionen[OptNrNFBildung].ParameterTyp.InfoString.DefaultInfoString = 
      "mixmost";

/*------ Zielbehandlung ---------------------------------*/

   AufrufOptionen[OptNrZielbehandlung].Kuerzel           = "-zb";
   AufrufOptionen[OptNrZielbehandlung].Name              = "'Z'iel-'B'ehandlung";
   AufrufOptionen[OptNrZielbehandlung].Bedeutung         = "Proof-Modus: Behandlung der Ziele";
   AufrufOptionen[OptNrZielbehandlung].ParameterArt      = 3;
   AufrufOptionen[OptNrZielbehandlung].ParameterTyp.InfoString.DefaultInfoString = 
      "continue:rnf";
   /*"continue:mnf=re.0:s:mnfanalysis=1000.2000.10:mnfantiWOVar:mnfantir=5:mnfup=5"*/;

/*------ Ordnungen ---------------------------------*/

   AufrufOptionen[OptNrOrdnungen].Kuerzel           = "-ord";
   AufrufOptionen[OptNrOrdnungen].Name              = "Orderings";
   AufrufOptionen[OptNrOrdnungen].Bedeutung         = "Override ordering from specification ";
   AufrufOptionen[OptNrOrdnungen].ParameterArt      = 3;
   AufrufOptionen[OptNrOrdnungen].ParameterTyp.InfoString.DefaultInfoString = "";

/*------ Dispatcher --------------------------------*/

   AufrufOptionen[OptNrDis].Kuerzel           = "-dis";
   AufrufOptionen[OptNrDis].Name              = "Dispatcher";
   AufrufOptionen[OptNrDis].Bedeutung         = "Set Dispatcher parameters";
   AufrufOptionen[OptNrDis].ParameterArt      = 3;
   AufrufOptionen[OptNrDis].ParameterTyp.InfoString.DefaultInfoString = "start=50:slaves=0:jobs=1";
   
/*------ Symbolnormierung---------------------------*/

   AufrufOptionen[OptNrSymbnorm].Kuerzel           = "-symbnorm";
   AufrufOptionen[OptNrSymbnorm].Name              = "Symbol setting";
   AufrufOptionen[OptNrSymbnorm].Bedeutung         = "normalize arrangement of symbols\n"
      "This parameter activates the unique reordering of\n"
      "the symbols from the specification file";
   AufrufOptionen[OptNrSymbnorm].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrSymbnorm].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrSymbnorm].ParameterArt      = 1;
   AufrufOptionen[OptNrSymbnorm].ParameterTyp.einstellig.defaultWert = TRUE;
   
/*------ TechnischeDetails ------------------------------*/

   AufrufOptionen[OptNrAusgabeLevel].Kuerzel                   = "-a";
   AufrufOptionen[OptNrAusgabeLevel].Name              = "'A'usgabe-Level";
   AufrufOptionen[OptNrAusgabeLevel].Bedeutung = "Einstellen des Ausgabe-Levels (Wert ist proportional zur Geschwaetzigkeit)";
   AufrufOptionen[OptNrAusgabeLevel].ParameterArt = 2;
   AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.defaultWert = 2;
   AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.AnzahlVerschiedenerWerte = 5;
   AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.WerteImKlartext[0] = "0";
   AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.WerteImKlartext[1] = "1";
   AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.WerteImKlartext[2] = "2";
   AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.WerteImKlartext[3] = "3";
   AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.WerteImKlartext[4] = "4";

   AufrufOptionen[OptNrPCLProt].Kuerzel           = "-pcl";
   AufrufOptionen[OptNrPCLProt].Name              = "'PCL' protocol";
   AufrufOptionen[OptNrPCLProt].Bedeutung         = "Generation of a proof protocol.\n";
   AufrufOptionen[OptNrPCLProt].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrPCLProt].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrPCLProt].ParameterArt      = 1;
   AufrufOptionen[OptNrPCLProt].ParameterTyp.einstellig.defaultWert = TRUE;

   AufrufOptionen[OptNrPCLVerbose].Kuerzel           = "-pclVerbose";
   AufrufOptionen[OptNrPCLVerbose].Name              = "'PCL' protocol for each activated fact";
   AufrufOptionen[OptNrPCLVerbose].Bedeutung         = "Generation of a proof protocol.\n";
   AufrufOptionen[OptNrPCLVerbose].ParameterTyp.einstellig.BedeutungT        = "On";
   AufrufOptionen[OptNrPCLVerbose].ParameterTyp.einstellig.BedeutungF        = "Off";
   AufrufOptionen[OptNrPCLVerbose].ParameterArt      = 1;
   AufrufOptionen[OptNrPCLVerbose].ParameterTyp.einstellig.defaultWert = FALSE;

   AufrufOptionen[OptNrPCLFile].Kuerzel       = "-pclOutput";
   AufrufOptionen[OptNrPCLFile].Name          = "output file for PCL protocol";
   AufrufOptionen[OptNrPCLFile].Bedeutung     = "where to write PCL protocol, default is stdout";
   AufrufOptionen[OptNrPCLFile].ParameterArt  = 0;
   AufrufOptionen[OptNrPCLFile].ParameterTyp.frei.wurdeAngegeben = FALSE;
   AufrufOptionen[OptNrPCLFile].ParameterTyp.frei.defaultWert = "";
   
   AufrufOptionen[OptNrZeitlimit].Kuerzel           = "-tl";
   AufrufOptionen[OptNrZeitlimit].Name              = "set 't'ime-'l'imit";
   AufrufOptionen[OptNrZeitlimit].Bedeutung         = "Waldmeister terminates after <sec> process time";
   AufrufOptionen[OptNrZeitlimit].ParameterArt      = 0;
   AufrufOptionen[OptNrZeitlimit].ParameterTyp.frei.defaultWert = "0";

   
   AufrufOptionen[OptNrRealZeitlimit].Kuerzel           = "-rtl";
   AufrufOptionen[OptNrRealZeitlimit].Name              = "set 'r'eal-'t'ime-'l'imit";
   AufrufOptionen[OptNrRealZeitlimit].Bedeutung         = "Waldmeister terminates after <sec> real time";
   AufrufOptionen[OptNrRealZeitlimit].ParameterArt      = 0;
   AufrufOptionen[OptNrRealZeitlimit].ParameterTyp.frei.defaultWert = "0";
   AufrufOptionen[OptNrRealZeitlimit].ParameterTyp.frei.wurdeAngegeben = FALSE;

   AufrufOptionen[OptNrMemlimit].Kuerzel           = "-ml";
   AufrufOptionen[OptNrMemlimit].Name              = "set 'm'emory-'l'imit";
   AufrufOptionen[OptNrMemlimit].Bedeutung         = "Waldmeister should use only <mem> megabytes of memory";
   AufrufOptionen[OptNrMemlimit].ParameterArt      = 0;
   AufrufOptionen[OptNrMemlimit].ParameterTyp.frei.defaultWert = "0";
   
   AufrufOptionen[OptNrAllocLog].Kuerzel           = "-allocLog";
   AufrufOptionen[OptNrAllocLog].Name              = "logging of memory allocations";
   AufrufOptionen[OptNrAllocLog].Bedeutung         = "";
   AufrufOptionen[OptNrAllocLog].ParameterTyp.einstellig.BedeutungT        = "An";
   AufrufOptionen[OptNrAllocLog].ParameterTyp.einstellig.BedeutungF        = "Aus";
   AufrufOptionen[OptNrAllocLog].ParameterArt      = 1;
   AufrufOptionen[OptNrAllocLog].ParameterTyp.einstellig.defaultWert = FALSE;

   AufrufOptionen[OptNrDummy].Kuerzel           = "-dummy";
   AufrufOptionen[OptNrDummy].Name              = "dummy";
   AufrufOptionen[OptNrDummy].Bedeutung         = "Not used by Waldmeister";
   AufrufOptionen[OptNrDummy].ParameterArt      = 0;
   AufrufOptionen[OptNrDummy].ParameterTyp.frei.defaultWert = "0";

   for (i=0; i<OptNrLAST; i++)
    switch (AufrufOptionen[i].ParameterArt){
    case 0:                     /* frei */
      AufrufOptionen[i].ParameterTyp.frei.eingestellterWert =
        AufrufOptionen[i].ParameterTyp.frei.defaultWert;
      AufrufOptionen[i].ParameterTyp.frei.wurdeAngegeben = FALSE;
      break;
    case 1:                     /* einstellig */
      AufrufOptionen[i].ParameterTyp.einstellig.eingestellterWert =
        AufrufOptionen[i].ParameterTyp.einstellig.defaultWert;
      break;
    case 2:                     /* zweistellig */
      AufrufOptionen[i].ParameterTyp.zweistellig.eingestellterWert =
        AufrufOptionen[i].ParameterTyp.zweistellig.defaultWert;
      break;
    case 3:                     /* InfoString */
      AufrufOptionen[i].ParameterTyp.InfoString.UserInfoString = "";
      break;
    default: break;
    }

   /*   PA_Paras.MNFString = AufrufOptionen[OptNrZielbehandlung].ParameterTyp.InfoString.DefaultInfoString;  XXX BL*/
}

static void PA_initDispatcher(void)
{
  char *valstr;
  unsigned int val;

  if (PA_Slave()){
    return;
  }

  valstr  = IS_ValueOfOption (AufrufOptionen[OptNrDis].ParameterTyp.InfoString.DefaultInfoString,
                              AufrufOptionen[OptNrDis].ParameterTyp.InfoString.UserInfoString, "start");
  if (valstr != NULL){
    val = atoi(valstr);
  }
  else {
    val = 50;
  }
  DIS_SetStartTime(val);

  valstr  = IS_ValueOfOption (AufrufOptionen[OptNrDis].ParameterTyp.InfoString.DefaultInfoString,
                                AufrufOptionen[OptNrDis].ParameterTyp.InfoString.UserInfoString, "jobs");
  if (valstr != NULL){
    val = atoi(valstr);
  }
  else {
    val = 1;
  }
  DIS_SetNoOfJobs(val);

  valstr  = IS_ValueOfOption (AufrufOptionen[OptNrDis].ParameterTyp.InfoString.DefaultInfoString,
                                AufrufOptionen[OptNrDis].ParameterTyp.InfoString.UserInfoString, "slaves");
  if (valstr != NULL){
    val = atoi(valstr);
  }
  else {
    val = 0;
  }
  DIS_SetNoOfSlaves(val);
}

static BOOLEAN pa_ParseCGCP (const char *str, int *cg, int *cp)
{
  char *start = strstr(str,"cgcp=");
  return (start != NULL) && (sscanf(start, "cgcp=%d.%d", cg, cp) == 2);
}

static BOOLEAN pa_ParseInterleave (const char *str, int *fifo, int *heuristic)
{
  char *start = strstr(str,"interleave=");
  return (start != NULL) && (sscanf(start, "interleave=%d.%d", fifo, heuristic) == 2);
}

static void pa_ParameterAuswerten(void)
{
  PA_Paras.FailingCompletion = AufrufOptionen[OptNrFailing].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.EinsSternAn = AufrufOptionen[OptNrEinsStern].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.NusfuAn = AufrufOptionen[OptNrNusfu].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.KernAn = AufrufOptionen[OptNrKern].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.GolemAn = AufrufOptionen[OptNrGolem].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.BackAn = AufrufOptionen[OptNrBack].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.PermsubAC = AufrufOptionen[OptNrPermsubAC].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.PermsubACI = AufrufOptionen[OptNrPermsubACI].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.SaveTerms = AufrufOptionen[OptNrSaveTerms].ParameterTyp.einstellig.eingestellterWert;

  PA_Paras.PermLemmata = AufrufOptionen[OptNrPermLemmata].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.IdempotenzLemmata = AufrufOptionen[OptNrIdempotenzLemmata].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.C_Lemma = AufrufOptionen[OptNrC_Lemma].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.Frischzellenkur = AufrufOptionen[OptNrFrischzellen].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.WeightAxiom = AufrufOptionen[OptNrWeightAxiom].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.Cancellation = AufrufOptionen[OptNrCancellation].ParameterTyp.InfoString.UserInfoString[0] != '\0';
  PA_Paras.doGZFB = AufrufOptionen[OptNrGZFB].ParameterTyp.einstellig.eingestellterWert;

  PA_Paras.doACG = AufrufOptionen[OptNrACG].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.doSK = AufrufOptionen[OptNrSK].ParameterTyp.einstellig.eingestellterWert;
  if (PA_doSK()) PA_Paras.doACG = TRUE;
  PA_Paras.doACGSKCheck = AufrufOptionen[OptNrACGSKCheck].ParameterTyp.einstellig.eingestellterWert && PA_doACG();
  PA_Paras.ACGRechts = AufrufOptionen[OptNrACGRechts].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.SKRechts = AufrufOptionen[OptNrSKRechts].ParameterTyp.einstellig.eingestellterWert;

  PA_Paras.Waisenmord = AufrufOptionen[OptNrWaisenmord].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.allocLog = AufrufOptionen[OptNrAllocLog].ParameterTyp.einstellig.eingestellterWert;

  PA_Paras.doPCL = AufrufOptionen[OptNrPCLProt].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.PCLVerbose = AufrufOptionen[OptNrPCLVerbose].ParameterTyp.einstellig.eingestellterWert;
  if (AufrufOptionen[OptNrPCLFile].ParameterTyp.frei.wurdeAngegeben){
    PA_Paras.pclFileName = AufrufOptionen[OptNrPCLFile].ParameterTyp.frei.eingestellterWert;
  }
  else {
    PA_Paras.pclFileName = NULL;
  }
  {
    PA_Paras.StatistikIntervall = COMP_COMP ? 0 : 100;
    if (AufrufOptionen[OptNrStatIntervall].ParameterTyp.frei.wurdeAngegeben){
      unsigned int val;
      if (sscanf(AufrufOptionen[OptNrStatIntervall].ParameterTyp.frei.eingestellterWert,
                 "%u", &val) == 1){
         PA_Paras.StatistikIntervall = val;
      }
      else {
        our_error("problem parsing argument for -wct: unsigned value n expected!"
                  " For example:  -wct 1000");
      }
    }
  }
  {
    PA_Paras.FullPermLemmata = 0;
    if (AufrufOptionen[OptNrFullPermLemmata].ParameterTyp.frei.wurdeAngegeben){
      unsigned int val;
      if (sscanf(AufrufOptionen[OptNrFullPermLemmata].ParameterTyp.frei.eingestellterWert,
                 "%u", &val) == 1){
        if ((4 <= val) && (val <= 6)){
          PA_Paras.FullPermLemmata = val;
        }
        else {
          our_error("value for -fullPermLemmata out of range: 4 <= n <= 6 required!"
                    " For example:  -fullPermLemmata 4");
        }
      }
      else {
        our_error("problem parsing argument for -fullPermLemmata: value n with 4 <= n <= 6 expected!"
                  " For example:  -fullPermLemmata 4");
      }
    }
  }
  {
    char *KapurString = AufrufOptionen[OptNrKapur].ParameterTyp.frei.eingestellterWert;
    PA_Paras.SubconR = strchr(KapurString, 'r') != NULL;
    PA_Paras.SubconE = strchr(KapurString, 'e') != NULL;
    PA_Paras.SubconSubst = strchr(KapurString, 's') != NULL;
    PA_Paras.SubconP = strchr(KapurString, 'p') != NULL;
  }

  {
    InfoStringT DEF = AufrufOptionen[OptNrNFBildung].ParameterTyp.InfoString.DefaultInfoString;
    InfoStringT USR = AufrufOptionen[OptNrNFBildung].ParameterTyp.InfoString.UserInfoString;
    char *name = NULL;
    
    switch (IS_FindOption(DEF, USR, 7, "outermost", "innermost", "mixmost", 
                          "finnermost", "foutermost", "mixmost2", "mixmost3")){
    case 0:
      name = "Leftmost-Outermost";
      PA_Paras.NFStrategie = PA_NFoutermost;
      break;
    case 1:
      name = "Leftmost-Innermost";
      PA_Paras.NFStrategie = PA_NFinnermost;
      break;
    case 2:
      name = "Mix-Most";
      PA_Paras.NFStrategie = PA_NFmixmost;
      break;
    case 3:
      name = "Leftmost-Innermost (mit Flags)";
      PA_Paras.NFStrategie = PA_NFinnermostFlags;
      break;
    case 4:
      name = "Leftmost-Outermost (mit Flags)";
      PA_Paras.NFStrategie = PA_NFoutermostFlags;
      break;
    case 5:
      name = "Mix-Most (Variante 2)";
      PA_Paras.NFStrategie = PA_NFmixmost2;
      break;
    case 6:
      name = "Mix-Most (Variante 3)";
      PA_Paras.NFStrategie = PA_NFmixmost3;
      break;
    default:
      our_error("Nicht implementierte NF-Variante!\n");
      break;
    }
#if 0 && !COMP_POWER
   printf("\n* NORMALIZATION:    %s\n", name);
#endif
  }
  {
    InfoStringT DEF = AufrufOptionen[OptNrSUE].ParameterTyp.InfoString.DefaultInfoString;
    InfoStringT USR = AufrufOptionen[OptNrSUE].ParameterTyp.InfoString.UserInfoString;
    int cg, cp;
    if (!(pa_ParseCGCP (USR, &cg, &cp) || pa_ParseCGCP (DEF, &cp, &cg))) {
      cp = cg = 1;
    }
    PA_Paras.modulo = cg + cp;
    PA_Paras.threshold = cg;
  }
  {
    InfoStringT DEF = AufrufOptionen[OptNrPriorityQueue].ParameterTyp.InfoString.DefaultInfoString;
    InfoStringT USR = AufrufOptionen[OptNrPriorityQueue].ParameterTyp.InfoString.UserInfoString;
    int fifo, heu;
    char *capacStr;
    unsigned long capac;

    if (!(pa_ParseInterleave (USR, &fifo, &heu) || pa_ParseInterleave (DEF, &fifo, &heu))) {
      fifo = 0;
      heu = 1;
    }
    PA_Paras.moduloCP = fifo + heu;
    PA_Paras.thresholdCP = fifo;

    capacStr = IS_ValueOf("capacity", USR);
    if (capacStr == NULL) capacStr = IS_ValueOf("capacity", DEF);
    if ((capacStr != NULL) && (sscanf(capacStr, "%lu", &capac) == 1)){
      PA_Paras.cacheSize = capac;
    }
    else {
      PA_Paras.cacheSize = 100000;
    }
  }
  {
    InfoStringT DEF = AufrufOptionen[OptNrCGInterreduktion].ParameterTyp.InfoString.DefaultInfoString;
    InfoStringT USR = AufrufOptionen[OptNrCGInterreduktion].ParameterTyp.InfoString.UserInfoString;
    int fifo, heu;
    if (!(pa_ParseInterleave (USR, &fifo, &heu) || pa_ParseInterleave (DEF, &fifo, &heu))) {
      fifo = 0;
      heu = 1;
    }
    PA_Paras.moduloCG = fifo + heu;
    PA_Paras.thresholdCG = fifo;
  }
  {
    InfoStringT DEF = AufrufOptionen[OptNrKPgen].ParameterTyp.InfoString.DefaultInfoString;
    InfoStringT USR = AufrufOptionen[OptNrKPgen].ParameterTyp.InfoString.UserInfoString;

    PA_Paras.GenR = (IS_FindOption(DEF, USR, 1, "r") == 0);
    PA_Paras.GenE = (IS_FindOption(DEF, USR, 1, "e") == 0);
    PA_Paras.GenS = (IS_FindOption(DEF, USR, 1, "s") == 0);
    PA_Paras.GenP = (IS_FindOption(DEF, USR, 1, "p") == 0);

    if (0&& !COMP_POWER){
      printf("\n* TREATMENT OF CPS ON GENERATION:    ");
      if (PA_GenR() || PA_GenE() || PA_GenS() || PA_GenP()){
        if (PA_GenR()) printf("R");
        if (PA_GenE()) printf("E");
        if (PA_GenS()) printf("S");
        if (PA_GenP()) printf("P");
        printf("\n");
      }
      else 
        printf("-\n");
    }
  }
  {
    InfoStringT DEF = AufrufOptionen[OptNrKPsel].ParameterTyp.InfoString.DefaultInfoString;
    InfoStringT USR = AufrufOptionen[OptNrKPsel].ParameterTyp.InfoString.UserInfoString;

    PA_Paras.SelR = (IS_FindOption(DEF, USR, 1, "r") == 0);
    PA_Paras.SelE = (IS_FindOption(DEF, USR, 1, "e") == 0);
    PA_Paras.SelS = (IS_FindOption(DEF, USR, 1, "s") == 0);
    PA_Paras.SelP = (IS_FindOption(DEF, USR, 1, "p") == 0);

    if (0&& !COMP_POWER){
      printf("\n* TREATMENT OF CPS ON SELECTION:     ");
      if (PA_SelR() || PA_SelE() || PA_SelS() || PA_SelP()){
        if (PA_SelR()) printf("R");
        if (PA_SelE()) printf("E");
        if (PA_SelS()) printf("S");
        if (PA_SelP()) printf("P");
        printf("\n");
      }
      else 
        printf("-\n");
    }
  }

  {
    InfoStringT DEF = AufrufOptionen[OptNrZielbehandlung].ParameterTyp.InfoString.DefaultInfoString;
    InfoStringT USR = AufrufOptionen[OptNrZielbehandlung].ParameterTyp.InfoString.UserInfoString;
    char          *valueStr, *token;
    unsigned long tokenValue;

    switch (IS_FindOption (DEF, USR, 3, "snf", "rnf", "mnf")) {
    default:
    case 0:
      PA_Paras.zielBehandlung = PA_snf;
      break;
    case 1:
      PA_Paras.zielBehandlung = PA_rnf;
      break;
    case 2:
      PA_Paras.zielBehandlung = PA_mnf;
      break;
    }
   
    switch (IS_FindOption(DEF, USR, 2, "stop", "continue")){
    case 0: 
      PA_Paras.allGoals = FALSE;
      break;
    case 1: 
    default: 
      PA_Paras.allGoals = TRUE; 
      break;
    }
    switch (IS_FindOption(DEF, USR, 1, "xstat")){
    case 0:
      PA_Paras.statisticsAfterProved = TRUE;
      break;
    default:
      PA_Paras.statisticsAfterProved = FALSE;
    }

   
    PA_Paras.useE             = TRUE;
    PA_Paras.useR             = TRUE;
    PA_Paras.PQ_Art           = PA_PQstack;
    PA_Paras.MNFmaxSize       = 0;
    PA_Paras.maxGlSchritte    = 0;
    PA_Paras.TryAllElements   = TRUE;
    PA_Paras.maxAntiRSchritte = 0;
    PA_Paras.antiKind         = PA_noAnti;
    PA_Paras.useAnalyse       = FALSE;
    PA_Paras.SwitchIrred      = 0;
    PA_Paras.SwitchNF         = 0;
    PA_Paras.CountNF          = 0;
    PA_Paras.HashSize         = 1023;
    
    valueStr = IS_ValueOfOption (DEF, USR, "mnf");
    /* valueStr ist leer, defaults uebernehmen */
    if (valueStr != NULL){
      token = valueStr;
      while (token != NULL) {
        if ((strncmp (token, "re", 2) == 0) || (strncmp(token, "er", 2) == 0)) {
          PA_Paras.useR = TRUE;
          PA_Paras.useE = TRUE;
        }
        else if (strncmp(token, "e", 1) == 0) {
          PA_Paras.useR = FALSE;
          PA_Paras.useE = TRUE;
        }
        else if (strncmp(token, "r", 1) == 0) {
          PA_Paras.useR = TRUE;
          PA_Paras.useE = FALSE;
       }
        else if (strncmp(token, "stack", 5) == 0) {
          PA_Paras.PQ_Art = PA_PQstack;
        }
        else if (strncmp(token, "deque", 5) == 0) {
          PA_Paras.PQ_Art = PA_PQdeque;
        }
        else if (strncmp(token, "fifo", 4) == 0) {
          PA_Paras.PQ_Art = PA_PQfifo;
       }
        else if (sscanf(token, "%ld", &tokenValue) == 1) {
          PA_Paras.MNFmaxSize = tokenValue;
        }
        else {
          fprintf(stderr, "cannot parse mnf option: >>%s<<\n", token);
          our_error("wrong arguments for mnf option");
        }
        token = strchr(token,'.');
        if (token != NULL) token++;
      }
    }
    if (IS_FindValuedOption (DEF, USR, 1, "mnfup") == 0) {
      valueStr = IS_ValueOfOption (DEF, USR, "mnfup");
      if (valueStr) {
        tokenValue = strtoul (valueStr, NULL, 10);
        PA_Paras.maxGlSchritte = tokenValue;
      }
    }
    if (IS_FindValuedOption (DEF, USR, 1, "mnfantir") == 0) {
      valueStr = IS_ValueOfOption (DEF, USR, "mnfantir");
      if (valueStr) {
        tokenValue = strtoul (valueStr, NULL, 10);
        PA_Paras.maxAntiRSchritte = tokenValue;
        PA_Paras.antiKind = PA_antiWOVar;
      }
    }
    if (IS_FindValuedOption (DEF, USR, 1, "mnfanalysis") == 0) {
      valueStr = IS_ValueOfOption (DEF, USR, "mnfanalysis");
      PA_Paras.useAnalyse = TRUE;
      
      token = valueStr;
      if (token != NULL) {
        PA_Paras.SwitchIrred = strtoul (token, NULL, 10);
        token = strchr(token,'.');
        if (token != NULL) token++;
      }
      if (token != NULL) {
        PA_Paras.SwitchNF = strtoul (token, NULL, 10);
        token = strchr(token,'.');
        if (token != NULL) token++;
      }
      if (token != NULL) {
        PA_Paras.CountNF = strtoul (token, NULL, 10);
      }
    }
    if (IS_FindOption (DEF,USR, 1, "mnftrynotall") == 0) {
      PA_Paras.TryAllElements = FALSE;
    }
    
    if (IS_FindOption (DEF,USR, 1, "mnfantiWOVar") == 0) {
      PA_Paras.antiKind = PA_antiWOVar;
    }
    if (IS_FindOption (DEF,USR, 1, "mnfantiBoundVar") == 0) {
      PA_Paras.antiKind = PA_antiBoundVar;
    }
    if (IS_FindOption (DEF,USR, 1, "mnfantiFreeVar") == 0) {
      PA_Paras.antiKind = PA_antiFreeVar;
    }
    if (IS_FindOption (DEF, USR, 1, "stack") == 0) {
      PA_Paras.PQ_Art = PA_PQstack;
    } 
    if (IS_FindOption (DEF, USR, 1, "deque") == 0) {
      PA_Paras.PQ_Art = PA_PQdeque;
    }
    if (IS_FindOption (DEF, USR, 1, "fifo") == 0) {
      PA_Paras.PQ_Art = PA_PQfifo;
    }
    
    if (IS_FindValuedOption (DEF, USR, 1, "hash") == 0) {
      valueStr = IS_ValueOfOption (DEF, USR, "hash");
      if (valueStr) {
        tokenValue = strtoul (valueStr, NULL, 10);
        PA_Paras.HashSize = tokenValue;
      }
    }
  }

  if (AufrufOptionen[OptNrCTVersion].ParameterTyp.frei.wurdeAngegeben){
    PA_Paras.CTreeVersion = atoi(AufrufOptionen[OptNrCTVersion].ParameterTyp.frei.eingestellterWert);
  }
  else {
    PA_Paras.CTreeVersion = 4;
  }

  if (AufrufOptionen[OptNrMaxNodeCount].ParameterTyp.frei.wurdeAngegeben){
    PA_Paras.maxNodeCount = atoi(AufrufOptionen[OptNrMaxNodeCount].ParameterTyp.frei.eingestellterWert);
  }
  else {
    PA_Paras.maxNodeCount = 0;
  }

  PA_Paras.CTreeS   = AufrufOptionen[OptNrCTS].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.CTreeP   = AufrufOptionen[OptNrCTP].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.sortDCons = AufrufOptionen[OptNrSortDCons].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.optDCons = AufrufOptionen[OptNrOptDCons].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.mergeDCons = AufrufOptionen[OptNrMergeDCons].ParameterTyp.einstellig.eingestellterWert;
  PA_Paras.preferD1 = AufrufOptionen[OptNrPreferD1].ParameterTyp.einstellig.eingestellterWert;
  
  if (AufrufOptionen[OptNrZeitlimit].ParameterTyp.frei.wurdeAngegeben){
    PA_Paras.Zeitlimit = atoi(AufrufOptionen[OptNrZeitlimit].ParameterTyp.frei.eingestellterWert);
  }
  else {
    PA_Paras.Zeitlimit = 0;
  }
  if (AufrufOptionen[OptNrRealZeitlimit].ParameterTyp.frei.wurdeAngegeben){
    PA_Paras.RealZeitlimit = atoi(AufrufOptionen[OptNrRealZeitlimit].ParameterTyp.frei.eingestellterWert);
  }
  else {
    PA_Paras.RealZeitlimit = 0;
  }

  if (AufrufOptionen[OptNrMemlimit].ParameterTyp.frei.wurdeAngegeben){
    PA_Paras.Memlimit = atoi(AufrufOptionen[OptNrMemlimit].ParameterTyp.frei.eingestellterWert);
  }
  else {
    PA_Paras.Memlimit = 0;
  }
}

void PA_InitAposteriori(void)
{
   int i;
   
   for (i=0; i<OptNrLAST; i++)
      if (AufrufOptionen[i].ParameterArt == 3){ /* InfoString */
         if (IS_IsIn("-", AufrufOptionen[i].ParameterTyp.InfoString.UserInfoString))
            AufrufOptionen[i].ParameterTyp.InfoString.DefaultInfoString = "";
      }

   pa_ParameterAuswerten();


   ORD_InitAposteriori (AufrufOptionen[OptNrOrdnungen].ParameterTyp.InfoString.DefaultInfoString,
			AufrufOptionen[OptNrOrdnungen].ParameterTyp.InfoString.UserInfoString);
   
   BK_InitAposteriori();

   /* An dieser Stelle muessten noch Ueberpruefungen der Gleichungen und Ziele stattfinden:
      - verwendete Argumentsorten;
      - Verwendung von Funktionen nur mit den richtigen Stelligkeiten.*/
   /* Skolemisierung der Ziele: Passiert aus dem Hauptprogramm heraus.*/
   /* clear_spec(); Erst einfuegen, wenn NACH Ablauf dieser Funktion nicht mehr auf 
		    die eingelesenen Daten zugegriffen werden muss.
		    ! Kann hier noch nicht aufgerufen werden, 
		    da die Ziele erst spaeter eingebaut werden. */

   NF_InitAposteriori(AufrufOptionen[OptNrCancellation].ParameterTyp.InfoString.UserInfoString);

   U1_InitAposteriori();

   BOB_InitAposteriori();

   C_InitAposteriori(AufrufOptionen[OptNrClassification].ParameterTyp.InfoString.DefaultInfoString,
                       AufrufOptionen[OptNrClassification].ParameterTyp.InfoString.UserInfoString,
                       AufrufOptionen[OptNrReClassification].ParameterTyp.InfoString.DefaultInfoString,
                       AufrufOptionen[OptNrReClassification].ParameterTyp.InfoString.UserInfoString,
                       AufrufOptionen[OptNrCGClassification].ParameterTyp.InfoString.DefaultInfoString,
                       AufrufOptionen[OptNrCGClassification].ParameterTyp.InfoString.UserInfoString);

   KPV_InitAposteriori();  


   Z_InitAposteriori();
   KO_InitAposteriori();
   CO_ConstraintsInitialisieren();
   PA_initDispatcher();
}


/* ------------------------------------------------------------------------------------*/
/* -------------------- Ausgabe der Einstellungen im Statistik-Kopf -------------------*/
/* ------------------------------------------------------------------------------------*/

void PA_DruckeModi(void)
/* Gibt die eingestellten Modi aus.
   Verwendung bei Ausgabe der Statistik.*/
{
  if (Vervollstaendigung){
    printf("\n* MODE:  %s completion\n", 
            AufrufOptionen[OptNrFailing].ParameterTyp.einstellig.eingestellterWert?
            "failing":"unfailing");
  }
  
  pa_IO_ParamInfo(ParamEvaluate, atoi(AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.
         WerteImKlartext[AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.eingestellterWert]));

  ORD_ParamInfo(ParamEvaluate, AufrufOptionen[OptNrOrdnungen].ParameterTyp.InfoString.DefaultInfoString,
			AufrufOptionen[OptNrOrdnungen].ParameterTyp.InfoString.UserInfoString);
  
  pa_NF_ParamInfo(ParamEvaluate, AufrufOptionen[OptNrNFBildung].ParameterTyp.InfoString.UserInfoString,
               AufrufOptionen[OptNrNFBildung].ParameterTyp.InfoString.DefaultInfoString);

  pa_U1_ParamInfo(ParamEvaluate, AufrufOptionen[OptNrKapur].ParameterTyp.frei.eingestellterWert);

#if 0
  KPB_ParamInfo(ParamEvaluate, AufrufOptionen[OptNrKPgen].ParameterTyp.InfoString.DefaultInfoString,
                AufrufOptionen[OptNrKPgen].ParameterTyp.InfoString.UserInfoString);
  KPS_ParamInfo(ParamEvaluate, AufrufOptionen[OptNrKPsel].ParameterTyp.InfoString.DefaultInfoString,
                AufrufOptionen[OptNrKPsel].ParameterTyp.InfoString.UserInfoString);

  SUE_ParamInfo(ParamEvaluate, 99, 
                AufrufOptionen[OptNrSUE].ParameterTyp.InfoString.DefaultInfoString,
                AufrufOptionen[OptNrSUE].ParameterTyp.InfoString.UserInfoString,
                AufrufOptionen[OptNrClassification].ParameterTyp.InfoString.DefaultInfoString,
                AufrufOptionen[OptNrClassification].ParameterTyp.InfoString.UserInfoString,
                AufrufOptionen[OptNrReClassification].ParameterTyp.InfoString.DefaultInfoString,
                AufrufOptionen[OptNrReClassification].ParameterTyp.InfoString.UserInfoString,
                AufrufOptionen[OptNrCGClassification].ParameterTyp.InfoString.DefaultInfoString,
                AufrufOptionen[OptNrCGClassification].ParameterTyp.InfoString.UserInfoString,
                AufrufOptionen[OptNrPriorityQueue].ParameterTyp.InfoString.DefaultInfoString,
                AufrufOptionen[OptNrPriorityQueue].ParameterTyp.InfoString.UserInfoString,
                AufrufOptionen[OptNrTermpairRepresentation].ParameterTyp.InfoString.DefaultInfoString,
                AufrufOptionen[OptNrTermpairRepresentation].ParameterTyp.InfoString.UserInfoString,
                AufrufOptionen[OptNrCGInterreduktion].ParameterTyp.InfoString.DefaultInfoString,
                AufrufOptionen[OptNrCGInterreduktion].ParameterTyp.InfoString.UserInfoString);
#endif
  
#if !COMP_POWER
   printf("\n* GROUND JOINABILITY:  %s\n", 
	   AufrufOptionen[OptNrGZFB].ParameterTyp.einstellig.eingestellterWert?
             "ON":"OFF");
#endif
  if (PA_ExtModus() == proof){
#if 0
     Z_ParamInfo(ParamEvaluate, AufrufOptionen[OptNrZielbehandlung].ParameterTyp.InfoString.DefaultInfoString,
                 AufrufOptionen[OptNrZielbehandlung].ParameterTyp.InfoString.UserInfoString);
#endif
  }
  pa_PCL_ParamInfo(ParamEvaluate, AufrufOptionen[OptNrPCLProt].ParameterTyp.InfoString.DefaultInfoString,
                AufrufOptionen[OptNrPCLProt].ParameterTyp.InfoString.UserInfoString);

  printf("\n\n");
}  

static void DruckeOptionDefault(FILE *fp, AufrufOptionenTyp i)
/* Gibt eine einzelne Option mit Kuerzel und Namen auf Strom fp aus.*/
{
      switch (AufrufOptionen[i].ParameterArt){
      case 0:
         if (AufrufOptionen[i].ParameterTyp.frei.eingestellterWert!=NULL)
            fprintf(fp, "%s %s ",AufrufOptionen[i].Kuerzel, AufrufOptionen[i].ParameterTyp.frei.eingestellterWert);
         break;
      case 1:
         fprintf(fp, "%s %s ",AufrufOptionen[i].Kuerzel,(AufrufOptionen[i].ParameterTyp.einstellig.eingestellterWert)?" ON":"OFF");
         break;
      case 2:
         fprintf(fp, "%s %30s ",AufrufOptionen[i].Kuerzel,
                 AufrufOptionen[i].ParameterTyp.zweistellig.WerteImKlartext[AufrufOptionen[i].ParameterTyp.zweistellig.eingestellterWert]);
      case 3:
         fprintf(fp, "%s %s%s%s ",
                 AufrufOptionen[i].Kuerzel,
                 AufrufOptionen[i].ParameterTyp.InfoString.UserInfoString,
                 AufrufOptionen[i].ParameterTyp.InfoString.UserInfoString?":":"",
                 AufrufOptionen[i].ParameterTyp.InfoString.DefaultInfoString);
         break;
      default:
         break;
      }
}

void PA_DefaultsAusgeben(void)
{
   int i;
   
   for (i=0; i<OptNrLAST; i++)
     DruckeOptionDefault(stdout,i);
   
   fprintf(stdout, "\n");
}

/* ------------------------------------------------------------------------*/
/* -------------------- Einstellen von Optionen ---------------------------*/
/* ------------------------------------------------------------------------*/

static BOOLEAN NeuerWertVonToggle(BOOLEAN AlterWert, char *Argument, BOOLEAN *ArgumentVerbraucht)
/* Liefert den neuen Wert des Toggles in Abhaengigkeit von Argument. */
{
   *ArgumentVerbraucht = FALSE;

   if (Argument!=NULL){
      if ((strcasecmp(Argument,"an")==0)||
          (strcasecmp(Argument,"on")==0)) {
         AlterWert = TRUE;
         *ArgumentVerbraucht = TRUE;
      }
      else if ((strcasecmp(Argument,"aus")==0)||
               (strcasecmp(Argument,"off")==0)){
         AlterWert = FALSE;
         *ArgumentVerbraucht = TRUE;
      }
      else
         AlterWert = !AlterWert;
   }
   else 
      AlterWert = !AlterWert;      
   return AlterWert;
}

static BOOLEAN OptionEingestellt(AufrufOptionenTyp OptNr, BOOLEAN ArgumentAlsIntErlaubt, BOOLEAN AnAus, 
                                 char *Argument, BOOLEAN *ArgumentVerbraucht)
{
   BOOLEAN Gefunden = FALSE;

   *ArgumentVerbraucht = TRUE;

   switch (AufrufOptionen[OptNr].ParameterArt){
   case 0:
      if (Argument){
         if (AufrufOptionen[OptNr].ParameterTyp.frei.wurdeAngegeben)
            our_dealloc (AufrufOptionen[OptNr].ParameterTyp.frei.eingestellterWert);
         AufrufOptionen[OptNr].ParameterTyp.frei.wurdeAngegeben = TRUE;
         AufrufOptionen[OptNr].ParameterTyp.frei.eingestellterWert = (char *) our_alloc(1+strlen(Argument));
         strcpy(AufrufOptionen[OptNr].ParameterTyp.frei.eingestellterWert, Argument);
         Gefunden = TRUE;
      }
      else{
         printf("*** Missing argument ");
         *ArgumentVerbraucht = FALSE;
      }
      break;
   case 1:
      AufrufOptionen[OptNr].ParameterTyp.einstellig.eingestellterWert = 
         NeuerWertVonToggle(AufrufOptionen[OptNr].ParameterTyp.einstellig.eingestellterWert, 
                            AnAus?Argument:NULL, ArgumentVerbraucht);
      Gefunden = TRUE;
      break;
   case 2:
   {
      int wert , i;
      if (ArgumentAlsIntErlaubt && ((wert= atoi(Argument))>0) &&
          (wert<=HoechsterSichtbarerIndex(OptNr))){
         AufrufOptionen[OptNr].ParameterTyp.zweistellig.eingestellterWert = wert-1;
         Gefunden = TRUE;
      }
      else
        if (Argument)
            for (i=0; i<HoechsterSichtbarerIndex(OptNr); i++)
               if (strstr(AufrufOptionen[OptNr].ParameterTyp.zweistellig.WerteImKlartext[i], Argument)){
                  AufrufOptionen[OptNr].ParameterTyp.zweistellig.eingestellterWert = i;
                  Gefunden = TRUE;
                  break;
               }
      if (Gefunden) {
      }
      else if (Argument){
         *ArgumentVerbraucht = FALSE;
         printf("%s *** unknown value ", Argument);
      }
      else{
         *ArgumentVerbraucht = FALSE;
         printf("*** missing argument ");
      }

      if (OptNr == OptNrAusgabeLevel)
        IO_AusgabeLevelBesetzen(atoi(AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.
           WerteImKlartext[AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.eingestellterWert]));
      break;
   }
   case 3:
      if (Argument){
         if (strlen (AufrufOptionen[OptNr].ParameterTyp.InfoString.UserInfoString)) {
            AufrufOptionen[OptNr].ParameterTyp.InfoString.UserInfoString =
               (char *) our_realloc(AufrufOptionen[OptNr].ParameterTyp.InfoString.UserInfoString,
                                    3+strlen(Argument)+
                                    strlen(AufrufOptionen[OptNr].ParameterTyp.InfoString.UserInfoString),
                                    "OptionEingestellt: Str erweitern");
            strcat(AufrufOptionen[OptNr].ParameterTyp.InfoString.UserInfoString, ":");            
            strcat(AufrufOptionen[OptNr].ParameterTyp.InfoString.UserInfoString, Argument);
         }
         else {
            AufrufOptionen[OptNr].ParameterTyp.InfoString.UserInfoString = (char *) our_alloc(1+strlen(Argument));
            strcpy(AufrufOptionen[OptNr].ParameterTyp.InfoString.UserInfoString, Argument);
         }
         Gefunden = TRUE;
      }
      else{
         printf("*** missing argument ");
         *ArgumentVerbraucht = FALSE;
      }
      break;
      
   default:
      *ArgumentVerbraucht = FALSE;
      our_error("\nError in SetOption.\n\n");
      break;
   }
   return Gefunden;
}
   
/* ------------------------------------------------------------------------*/
/* -------------------- Parameter-Einstellung -----------------------------*/
/* ------------------------------------------------------------------------*/

void PA_Vorbehandeln(int argc, char *argv[])
/* - Einstellung der Pseudo-Parameter, soweit moeglich
   Rueckgabe ist FALSE, wenn das Vorbehandeln bereits einen Abbruchgrund lieferte.
   Die vernutzten Argumente werden auf NULL gesetzt.
*/
{
   BOOLEAN Verbraucht = FALSE;
   int pos;
   char *argument;

   PA_Paras.ForkErlaubt = TRUE; /*generell erlauben*/

   pos = 1; /* argv[0] == Programmname, deshalb ist argc >= 2 
               sonst wird diese Prozedur nicht aufgerufen */
   
#if IF_TERMZELLEN_JAGD
   PA_HACK = atoi(argv[argc-1]);
   argv[argc-1] = NULL;
   argc--;
#endif

   while (pos < argc){
      if (strcmp(argv[pos], "-def") ==0){
        PA_DefaultsAusgeben();
        exit(0);
      }
#ifdef CCE_Protocol
      else if ((strcmp(argv[pos], "-cce") == 0)){
         argv[pos] = NULL;
         if (pos+1 < argc){
            IO_CCEinit(argv[pos+1]);
            pos++;
            argv[pos]=NULL;
         }
         pos++;
      }
#endif
      /* Die folgenden Optionen muessen bereits hier geparst werden, damit -auto funktioniert */
      else if (strcmp(argv[pos], "-slave") == 0) {
	argv[pos] = NULL;
	pos++;
	PA_Paras.Slave = TRUE;
	if (!COMP_DIST){
	  our_error("Not a distributed Waldmeister! Do not give -slave!");
	}
      }
      else if (strcmp(argv[pos], "-v") == 0) {
	argv[pos] = NULL;
	pos++;
        pa_verbose = TRUE;
      }
      else if (strcmp(argv[pos], "-auto") == 0) {
         argv[pos] = NULL;
         pos++;
         argument = (pos < argc) ? argv[pos] : NULL; /* argv[pos] ist nur gueltig fuer pos < argc !*/
         PA_Paras.Automodus = NeuerWertVonToggle(PA_Automodus(), argument, &Verbraucht);
         if (Verbraucht){
            argv[pos] = NULL;
            pos++;
         }
      }
      else if (strcmp(argv[pos], "-fork") == 0) {
         argv[pos] = NULL;
         pos++;
         argument = (pos < argc) ? argv[pos] : NULL; /* argv[pos] ist nur gueltig fuer pos < argc !*/
         PA_Paras.ForkErlaubt = NeuerWertVonToggle(PA_ForkErlaubt(), argument, &Verbraucht);
         if (Verbraucht){
            argv[pos] = NULL;
            pos++;
         }
      }
      else if (strcmp (argv[pos], "-symbnorm") == 0) {
         argv[pos] = NULL;
         pos++;
         argument = (pos < argc) ? argv[pos] : NULL; /* argv[pos] ist nur gueltig fuer pos < argc !*/
         OptionEingestellt (OptNrSymbnorm, FALSE, TRUE, argument, &Verbraucht);
         if (Verbraucht){
            argv[pos] = NULL;
            pos++;
         }
      }
      else                      /* sonst: Weiterlaufen */
         pos++;
   }
   
}


void PA_InitAposterioriEarly (void)
{
  SN_SpezifikationAuswerten(AufrufOptionen[OptNrSymbnorm].ParameterTyp.einstellig.eingestellterWert);
  SO_InitAposteriori();
  PR_PraezedenzVonSpecLesen();
  SG_KBOGewichteVonSpecLesen();
  TO_InitAposteriori();
  IO_InitAposterioriEarly ();
}


void PA_AufrufParameterEinstellen (int argc, char *argv[], mode Modus)
/* Einstellen der beim Aufruf mitgegebenen Parameter. argc und argv sind die Parameter von main,
   es wird also davon ausgegangen, dass argv[0] den Programmnamen enthaelt, und das argv[argc] der
   Name der zu bearbeitenden Spezifikation ist. Die Angaben zwischen argv[1] und argv[argc-1] werden
   ausgewertet. Dabei werden die in Parameter.c spezifizierten Aufruf-Optionen herangezogen.
   Inkorrekte Parameter fuehren zum Programmabbruch.
*/
{
   BOOLEAN ArgumentVerbraucht;
   int pos;
   int option;
   char *argument;

   PA_Paras.ExtModus = Modus;
   Vervollstaendigung = (PA_ExtModus() == proof) || (PA_ExtModus() == completion);
   PA_Paras.Ordering = ord_name(get_ordering());


   pos = 1;    /* argv[0] == Programmname, deshalb ist argc >= 2
                  sonst wird diese Prozedur nicht aufgerufen */

   while (pos < argc){
     if (argv[pos] == NULL){ /* Ueberspringen von schon erledigten Pseudooptionen/Dateinamen */
       pos++;
       continue;
     }
     for (option = 0; option < OptNrLAST; option++){
       if (strcmp(AufrufOptionen[option].Kuerzel, argv[pos]) == 0){
         break;
       }
     }
     if (option == OptNrLAST){
       printf("Unknown option or invalid filename: %s\n", argv[pos]);
       exit(1);
     }

     pos++; /* pos steht jetzt auf dem naechsten Element der Kommando-Zeile */
     argument = (pos < argc) ? argv[pos] : NULL; /* argv[pos] ist nur gueltig fuer pos < argc !*/
     if (!OptionEingestellt(option, FALSE, TRUE, argument, &ArgumentVerbraucht)){
         printf("Cannot parse option argument >>%s<< for option %s\n", argument, argv[pos-1]);
         exit(1);
     }
     else if (ArgumentVerbraucht) 
       pos++;
   }
   
   IO_InitAposteriori(atoi(AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.
         WerteImKlartext[AufrufOptionen[OptNrAusgabeLevel].ParameterTyp.zweistellig.eingestellterWert]));

}

/* Fuer Ausgabe der internen Parametrisierung. */

static char* ordname(ordering o)
{
  switch (o){
  case ordering_kbo:   return "ordering_kbo  ";
  case ordering_lpo:   return "ordering_lpo  ";
  case ordering_empty: return "ordering_empty";
  case ordering_col3:  return "ordering_col3 ";
  case ordering_col4:  return "ordering_col4 ";
  case ordering_cons:  return "ordering_cons ";
  }
  return "UNKNOWN";
}
static char* extname (mode e)
{
  switch (e){
  case completion:  return "completion ";
  case confluence:  return "confluence ";
  case convergence: return "convergence"; 
  case proof:       return "proof      ";
  case reduction:   return "reduction  ";
  case termination: return "termination"; 
  case database:    return "database   "; 
  }
  return "UNKNOWN";
}
static char* nfname(PA_NFStratT n)
{
  switch (n){
  case PA_NFoutermost:      return "NFoutermost     ";
  case PA_NFinnermost:      return "NFinnermost     ";
  case PA_NFmixmost:        return "NFmixmost       ";
  case PA_NFmixmost2:       return "NFmixmost2      ";
  case PA_NFmixmost3:       return "NFmixmost3      ";
  case PA_NFoutermostFlags: return "NFoutermostFlags";
  case PA_NFinnermostFlags: return "NFinnermostFlags";
  }
  return "UNKNOWN";
}
static char* pqartname(PA_PQArtT a)
{
  switch (a){
  case PA_PQstack: return "stack";
  case PA_PQdeque: return "deque";
  case PA_PQfifo:  return "fifo "; 
  }
  return "UNKNOWN";
}
static char* antikname(PA_MNFAntiT a)
{
  switch (a){
  case PA_noAnti:       return "noAnti      ";
  case PA_antiWOVar:    return "antiWOVar   ";
  case PA_antiBoundVar: return "antiBoundVar";
  case PA_antiFreeVar:  return "antiFreeVar ";
  }
  return "UNKNOWN";
}
static char* zbname(PA_zbT m)
{
  switch (m){
  case PA_snf: return "snf";
  case PA_rnf: return "rnf";
  case PA_mnf: return "mnf";
  }
  return "UNKNOWN";
}

void PA_PrintParas(void)
{
  if (pa_verbose){
    IO_DruckeFlex("-------------------------------------------------------------"
                  "-------------------\n");
    IO_DruckeFlex("PA_Ordering()             = %s\n", ordname(PA_Ordering())    );
    IO_DruckeFlex("PA_ExtModus()             = %s\n", extname(PA_ExtModus())    );
    IO_DruckeFlex("PA_FailingCompletion()    = %b\n", PA_FailingCompletion()    );
    IO_DruckeFlex("PA_EinsSternAn()          = %b\n", PA_EinsSternAn()          );
    IO_DruckeFlex("PA_NusfuAn()              = %b\n", PA_NusfuAn()              );
    IO_DruckeFlex("PA_KernAn()               = %b\n", PA_KernAn()               );
    IO_DruckeFlex("PA_GolemAn()              = %b\n", PA_GolemAn()              );
    IO_DruckeFlex("PA_BackAn()               = %b\n", PA_BackAn()               );
    IO_DruckeFlex("PA_PermsubAC()            = %b\n", PA_PermsubAC()            );
    IO_DruckeFlex("PA_PermsubACI()           = %b\n", PA_PermsubACI()           );
    IO_DruckeFlex("PA_Automodus()            = %b\n", PA_Automodus()            );
    IO_DruckeFlex("PA_ForkErlaubt()          = %b\n", PA_ForkErlaubt()          );
    IO_DruckeFlex("PA_Slave()                = %b\n", PA_Slave()                );
    IO_DruckeFlex("PA_SaveTerms()            = %b\n", PA_SaveTerms()            );
    IO_DruckeFlex("PA_StatistikIntervall()   = %u\n", PA_StatistikIntervall()   );
    IO_DruckeFlex("PA_PermLemmata()          = %b\n", PA_PermLemmata()          );
    IO_DruckeFlex("PA_FullPermLemmata()      = %u\n", PA_FullPermLemmata()      );
    IO_DruckeFlex("PA_IdempotenzLemmata()    = %b\n", PA_IdempotenzLemmata()    );
    IO_DruckeFlex("PA_C_Lemma()              = %b\n", PA_C_Lemma()              );
    IO_DruckeFlex("PA_Frischzellenkur()      = %b\n", PA_Frischzellenkur()      );
    IO_DruckeFlex("PA_SubconR()              = %b\n", PA_SubconR()              );
    IO_DruckeFlex("PA_SubconE()              = %b\n", PA_SubconE()              );
    IO_DruckeFlex("PA_SubconSubst()          = %b\n", PA_SubconSubst()          );
    IO_DruckeFlex("PA_SubconP()              = %b\n", PA_SubconP()              );
    IO_DruckeFlex("PA_doPCL()                = %b\n", PA_doPCL()                );
    IO_DruckeFlex("PA_PCLVerbose()           = %b\n", PA_PCLVerbose()           );
    IO_DruckeFlex("PA_pclFileName()          = %s\n", PA_pclFileName()          );
    IO_DruckeFlex("PA_NFStrategie()          = %s\n", nfname(PA_NFStrategie())  );
    IO_DruckeFlex("PA_doGZFB()               = %b\n", PA_doGZFB()               );
    IO_DruckeFlex("PA_doACG()                = %b\n", PA_doACG()                );
    IO_DruckeFlex("PA_doSK()                 = %b\n", PA_doSK()                 );
    IO_DruckeFlex("PA_doACGSKCheck()         = %b\n", PA_doACGSKCheck()         );
    IO_DruckeFlex("PA_acgRechts()            = %b\n", PA_acgRechts()            );
    IO_DruckeFlex("PA_skRechts()             = %b\n", PA_skRechts()             );
    IO_DruckeFlex("PA_GenR()                 = %b\n", PA_GenR()                 );
    IO_DruckeFlex("PA_GenE()                 = %b\n", PA_GenE()                 );
    IO_DruckeFlex("PA_GenS()                 = %b\n", PA_GenS()                 );
    IO_DruckeFlex("PA_GenP()                 = %b\n", PA_GenP()                 );
    IO_DruckeFlex("PA_SelR()                 = %b\n", PA_SelR()                 );
    IO_DruckeFlex("PA_SelE()                 = %b\n", PA_SelE()                 );
    IO_DruckeFlex("PA_SelS()                 = %b\n", PA_SelS()                 );
    IO_DruckeFlex("PA_SelP()                 = %b\n", PA_SelP()                 );
    IO_DruckeFlex("PA_modulo()               = %u\n", PA_modulo()               );
    IO_DruckeFlex("PA_threshold()            = %u\n", PA_threshold()            );
    IO_DruckeFlex("PA_moduloCP()             = %u\n", PA_moduloCP()             );
    IO_DruckeFlex("PA_thresholdCP()          = %u\n", PA_thresholdCP()          );
    IO_DruckeFlex("PA_moduloCG()             = %u\n", PA_moduloCG()             );
    IO_DruckeFlex("PA_thresholdCG()          = %u\n", PA_thresholdCG()          );
    IO_DruckeFlex("PA_Waisenmord()           = %b\n", PA_Waisenmord()           );
    IO_DruckeFlex("PA_cacheSize()            = %l\n", PA_cacheSize()            );
    IO_DruckeFlex("PA_useE()                 = %b\n", PA_useE()                 );
    IO_DruckeFlex("PA_useR()                 = %b\n", PA_useR()                 );
    IO_DruckeFlex("PA_PQ_Art()               = %s\n", pqartname(PA_PQ_Art())    );
    IO_DruckeFlex("PA_MNFmaxSize()           = %u\n", PA_MNFmaxSize()           );
    IO_DruckeFlex("PA_maxGlSchritte()        = %u\n", PA_maxGlSchritte()        );
    IO_DruckeFlex("PA_TryAllElements()       = %b\n", PA_TryAllElements()       );
    IO_DruckeFlex("PA_maxAntiRSchritte()     = %u\n", PA_maxAntiRSchritte()     );
    IO_DruckeFlex("PA_antiKind()             = %b\n", antikname(PA_antiKind())  );
    IO_DruckeFlex("PA_useAnalyse()           = %b\n", PA_useAnalyse()           );
    IO_DruckeFlex("PA_SwitchIrred()          = %u\n", PA_SwitchIrred()          );
    IO_DruckeFlex("PA_SwitchNF()             = %u\n", PA_SwitchNF()             );
    IO_DruckeFlex("PA_CountNF()              = %u\n", PA_CountNF()              );
    IO_DruckeFlex("PA_HashSize()             = %u\n", PA_HashSize()             );
    IO_DruckeFlex("PA_CTreeVersion()         = %u\n", PA_CTreeVersion()         );
    IO_DruckeFlex("PA_maxNodeCount()         = %u\n", PA_maxNodeCount()         );
    IO_DruckeFlex("PA_CTreeS()               = %b\n", PA_CTreeS()               );
    IO_DruckeFlex("PA_CTreeP()               = %b\n", PA_CTreeP()               );
    IO_DruckeFlex("PA_sortDCons()            = %b\n", PA_sortDCons()            );
    IO_DruckeFlex("PA_optDCons()             = %b\n", PA_optDCons()             );
    IO_DruckeFlex("PA_mergeDCons()           = %b\n", PA_mergeDCons()           );
    IO_DruckeFlex("PA_preferD1()             = %b\n", PA_preferD1()             );
    IO_DruckeFlex("PA_zielBehandlung()       = %s\n", zbname(PA_zielBehandlung()));
    IO_DruckeFlex("PA_statisticsAfterProved()= %b\n", PA_statisticsAfterProved());
    IO_DruckeFlex("PA_allGoals()             = %b\n", PA_allGoals()             );
    IO_DruckeFlex("PA_ZeitlimitGesetzt()     = %b\n", PA_ZeitlimitGesetzt()     );
    IO_DruckeFlex("PA_Zeitlimit()            = %d\n", PA_Zeitlimit()            );
    IO_DruckeFlex("PA_RealZeitlimitGesetzt() = %b\n", PA_RealZeitlimitGesetzt() );
    IO_DruckeFlex("PA_RealZeitlimit()        = %d\n", PA_RealZeitlimit()        );
    IO_DruckeFlex("PA_MemlimitGesetzt()      = %b\n", PA_MemlimitGesetzt()      );
    IO_DruckeFlex("PA_Memlimit()             = %d\n", PA_Memlimit()             );
    IO_DruckeFlex("PA_WeightAxiom()          = %b\n", PA_WeightAxiom()          );
    IO_DruckeFlex("PA_allocLog()             = %b\n", PA_allocLog()             );
    IO_DruckeFlex("-------------------------------------------------------------"
                  "-------------------\n");
  }
}


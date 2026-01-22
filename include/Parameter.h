#ifndef PARAMETER_H
#define PARAMETER_H

/* ------------------------------------------------*/
/* ------------- Includes -------------------------*/
/* ------------------------------------------------*/

#include "parser.h"
#include "general.h"

/*-------------------------------------------------------*/
/*------ Parametrisierungsflags -------------------------*/
/*-------------------------------------------------------*/

typedef enum {
  PA_NFoutermost, PA_NFinnermost, 
  PA_NFmixmost, PA_NFmixmost2, PA_NFmixmost3, 
  PA_NFoutermostFlags, PA_NFinnermostFlags
} PA_NFStratT;

typedef enum {
  PA_noAnti, PA_antiWOVar, PA_antiBoundVar, PA_antiFreeVar
} PA_MNFAntiT;

typedef enum {
   PA_PQstack, PA_PQdeque, PA_PQfifo
} PA_PQArtT;

typedef enum {
   PA_snf, PA_rnf, PA_mnf
} PA_zbT;

typedef struct {
  ordering Ordering;
  mode     ExtModus;
  BOOLEAN  FailingCompletion;
  BOOLEAN  EinsSternAn;
  BOOLEAN  NusfuAn;
  BOOLEAN  KernAn;
  BOOLEAN  GolemAn;
  BOOLEAN  BackAn;
  BOOLEAN  PermsubAC;
  BOOLEAN  PermsubACI;
  BOOLEAN  Automodus;
  BOOLEAN  ForkErlaubt;
  BOOLEAN  Slave;
  BOOLEAN  SaveTerms;
  unsigned StatistikIntervall;
  BOOLEAN  PermLemmata;
  unsigned int FullPermLemmata;
  BOOLEAN  IdempotenzLemmata;
  BOOLEAN  C_Lemma;
  BOOLEAN  Frischzellenkur;
  BOOLEAN  SubconR;
  BOOLEAN  SubconE;
  BOOLEAN  SubconSubst;
  BOOLEAN  SubconP;
  BOOLEAN  doPCL;
  BOOLEAN  PCLVerbose;
  char *   pclFileName;
  PA_NFStratT NFStrategie;
  BOOLEAN  Cancellation;
  BOOLEAN  doGZFB;
  BOOLEAN  doACG;
  BOOLEAN  doSK;
  BOOLEAN  doACGSKCheck;
  BOOLEAN  ACGRechts;
  BOOLEAN  SKRechts;
  BOOLEAN  GenR;
  BOOLEAN  GenE;
  BOOLEAN  GenS;  
  BOOLEAN  GenP;  
  BOOLEAN  SelR;
  BOOLEAN  SelE;
  BOOLEAN  SelS;  
  BOOLEAN  SelP;  
  unsigned int modulo;
  unsigned int threshold;
  unsigned int moduloCP;
  unsigned int thresholdCP;
  unsigned int moduloCG;
  unsigned int thresholdCG;
  BOOLEAN Waisenmord;
  unsigned long int cacheSize;
  BOOLEAN useE;
  BOOLEAN useR;
  PA_PQArtT PQ_Art;
  unsigned int MNFmaxSize;
  unsigned int maxGlSchritte;
  BOOLEAN TryAllElements;
  unsigned int maxAntiRSchritte;
  PA_MNFAntiT antiKind;
  BOOLEAN useAnalyse;
  unsigned int SwitchIrred;
  unsigned int SwitchNF;
  unsigned int CountNF;
  unsigned int HashSize;
  PA_zbT  zielBehandlung;
  BOOLEAN statisticsAfterProved;
  BOOLEAN allGoals;
  unsigned int CTreeVersion;
  unsigned int maxNodeCount;
  BOOLEAN  CTreeS;
  BOOLEAN  CTreeP;
  BOOLEAN  sortDCons;
  BOOLEAN  optDCons;
  BOOLEAN  mergeDCons;
  BOOLEAN  preferD1;
  int Zeitlimit;
  int RealZeitlimit;
  int Memlimit;
  BOOLEAN WeightAxiom;
  BOOLEAN allocLog;
} PA_ParasT;

extern PA_ParasT PA_Paras;

#define /*ordering*/ PA_Ordering()           (PA_Paras.Ordering)           
#define /*mode    */ PA_ExtModus()           (PA_Paras.ExtModus)           
#define /*BOOLEAN */ PA_FailingCompletion()  (PA_Paras.FailingCompletion)
#define /*BOOLEAN */ PA_EinsSternAn()        (PA_Paras.EinsSternAn)        
#define /*BOOLEAN */ PA_NusfuAn()            (PA_Paras.NusfuAn)            
#define /*BOOLEAN */ PA_KernAn()             (PA_Paras.KernAn)             
#define /*BOOLEAN */ PA_GolemAn()            (PA_Paras.GolemAn)            
#define /*BOOLEAN */ PA_BackAn()             (PA_Paras.BackAn)             
#define /*BOOLEAN */ PA_PermsubAC()          (PA_Paras.PermsubAC)          
#define /*BOOLEAN */ PA_PermsubACI()         (PA_Paras.PermsubACI)         
#define /*BOOLEAN */ PA_Automodus()          (PA_Paras.Automodus)          
#define /*BOOLEAN */ PA_ForkErlaubt()        (PA_Paras.ForkErlaubt)        
#define /*BOOLEAN */ PA_Slave()              (PA_Paras.Slave)              
#define /*BOOLEAN */ PA_SaveTerms()          (PA_Paras.SaveTerms)          

#define /*void    */ PA_SetGolem(v)          (PA_Paras.GolemAn = (v))            
#define /*void    */ PA_SetOrdering(v)       (PA_Paras.Ordering = (v))            

#define /*unsigned*/ PA_StatistikIntervall() (PA_Paras.StatistikIntervall)
#define /*BOOLEAN */ PA_PermLemmata()        (PA_Paras.PermLemmata)
#define /*unsigned*/ PA_FullPermLemmata()    (PA_Paras.FullPermLemmata)
#define /*BOOLEAN */ PA_IdempotenzLemmata()  (PA_Paras.IdempotenzLemmata)
#define /*BOOLEAN */ PA_C_Lemma()            (PA_Paras.C_Lemma)
#define /*BOOLEAN */ PA_Frischzellenkur()    (PA_Paras.Frischzellenkur)          

#define /*BOOLEAN */ PA_SubconR()            (PA_Paras.SubconR)
#define /*BOOLEAN */ PA_SubconE()            (PA_Paras.SubconE) 
#define /*BOOLEAN */ PA_SubconSubst()        (PA_Paras.SubconSubst)
#define /*BOOLEAN */ PA_SubconP()            (PA_Paras.SubconP) 

#define /*BOOLEAN */ PA_doPCL()              (PA_Paras.doPCL)
#define /*BOOLEAN */ PA_PCLVerbose()         (PA_Paras.PCLVerbose)
#define /*char *  */ PA_pclFileName()        (PA_Paras.pclFileName)
#define /*PA_NFStratT*/ PA_NFStrategie()     (PA_Paras.NFStrategie)
#define /*BOOLEAN */ PA_Cancellation()       (PA_Paras.Cancellation)
#define /*BOOLEAN */ PA_doGZFB()             (PA_Paras.doGZFB)
#define /*BOOLEAN */ PA_doACG()              (PA_Paras.doACG)
#define /*BOOLEAN */ PA_doSK()               (PA_Paras.doSK)
#define /*BOOLEAN */ PA_doACGSKCheck()       (PA_Paras.doACGSKCheck)
#define /*BOOLEAN */ PA_acgRechts()          (PA_Paras.ACGRechts)
#define /*BOOLEAN */ PA_skRechts()           (PA_Paras.SKRechts)

#define /*BOOLEAN */ PA_GenR()               (PA_Paras.GenR)
#define /*BOOLEAN */ PA_GenE()               (PA_Paras.GenE)
#define /*BOOLEAN */ PA_GenS()               (PA_Paras.GenS)  
#define /*BOOLEAN */ PA_GenP()               (PA_Paras.GenP)  
#define /*BOOLEAN */ PA_SelR()               (PA_Paras.SelR)
#define /*BOOLEAN */ PA_SelE()               (PA_Paras.SelE)
#define /*BOOLEAN */ PA_SelS()               (PA_Paras.SelS)  
#define /*BOOLEAN */ PA_SelP()               (PA_Paras.SelP)  
#define /*unsigned int */ PA_modulo()        (PA_Paras.modulo)
#define /*unsigned int */ PA_threshold()     (PA_Paras.threshold)
#define /*unsigned int */ PA_moduloCP()      (PA_Paras.moduloCP)
#define /*unsigned int */ PA_thresholdCP()   (PA_Paras.thresholdCP)
#define /*unsigned int */ PA_moduloCG()      (PA_Paras.moduloCG)
#define /*unsigned int */ PA_thresholdCG()   (PA_Paras.thresholdCG)
#define /*BOOLEAN */ PA_Waisenmord()         (PA_Paras.Waisenmord)

#define /*unsigned long int */ PA_cacheSize()(PA_Paras.cacheSize)

#define /*BOOLEAN      */ PA_useE()             (PA_Paras.useE)              
#define /*BOOLEAN      */ PA_useR()             (PA_Paras.useR)              
#define /*PA_PQArtT    */ PA_PQ_Art()           (PA_Paras.PQ_Art)            
#define /*unsigned int */ PA_MNFmaxSize()       (PA_Paras.MNFmaxSize)        
#define /*unsigned int */ PA_maxGlSchritte()    (PA_Paras.maxGlSchritte)     
#define /*BOOLEAN      */ PA_TryAllElements()   (PA_Paras.TryAllElements)    
#define /*unsigned int */ PA_maxAntiRSchritte() (PA_Paras.maxAntiRSchritte)  
#define /*PA_MNFAntiT  */ PA_antiKind()         (PA_Paras.antiKind)          
#define /*BOOLEAN      */ PA_useAnalyse()       (PA_Paras.useAnalyse)        
#define /*unsigned int */ PA_SwitchIrred()      (PA_Paras.SwitchIrred)       
#define /*unsigned int */ PA_SwitchNF()         (PA_Paras.SwitchNF)          
#define /*unsigned int */ PA_CountNF()          (PA_Paras.CountNF)           
#define /*unsigned int */ PA_HashSize()         (PA_Paras.HashSize)

#define /*unsigned int */ PA_CTreeVersion()     (PA_Paras.CTreeVersion)
#define /*unsigned int */ PA_maxNodeCount()     (PA_Paras.maxNodeCount)
#define /*BOOLEAN      */ PA_CTreeS()           (PA_Paras.CTreeS)
#define /*BOOLEAN      */ PA_CTreeP()           (PA_Paras.CTreeP)
#define /*BOOLEAN      */ PA_sortDCons()        (PA_Paras.sortDCons)
#define /*BOOLEAN      */ PA_optDCons()         (PA_Paras.optDCons)
#define /*BOOLEAN      */ PA_mergeDCons()       (PA_Paras.mergeDCons)
#define /*BOOLEAN      */ PA_preferD1()         (PA_Paras.preferD1)

#define /*PA_zbT  */ PA_zielBehandlung()        (PA_Paras.zielBehandlung)
#define /*BOOLEAN */ PA_statisticsAfterProved() (PA_Paras.statisticsAfterProved)
#define /*BOOLEAN */ PA_allGoals()              (PA_Paras.allGoals)

#define /*BOOLEAN */ PA_ZeitlimitGesetzt()      (PA_Paras.Zeitlimit > 0)
#define /*int     */ PA_Zeitlimit()             (PA_Paras.Zeitlimit)
#define /*BOOLEAN */ PA_RealZeitlimitGesetzt()  (PA_Paras.RealZeitlimit > 0)
#define /*int     */ PA_RealZeitlimit()         (PA_Paras.RealZeitlimit)
#define /*BOOLEAN */ PA_MemlimitGesetzt()       (PA_Paras.Memlimit > 0)
#define /*int     */ PA_Memlimit()              (PA_Paras.Memlimit)
#define /*BOOLEAN */ PA_WeightAxiom()           (PA_Paras.WeightAxiom)

#define /*BOOLEAN */ PA_allocLog()              (PA_Paras.allocLog)

#define /*PA_PQArtT    */ PA_Set_PQ_Art(/*PA_PQArtT*/ art) (PA_Paras.PQ_Art = (art))            

/* ------------------------------------------------*/
/* -------- Einstellen der Aufruf-Optionen --------*/
/* ------------------------------------------------*/
/* Die Funktionen stehen in der Reihenfolge, in der sie aufzurufen sind. */

void PA_InitApriori(void);
/* Parameter besetzen */


void PA_Vorbehandeln(int argc, char *argv[]);
/* - Reaktion auf Hilfe-Anforderung
   - Einstellung der Pseudo-Parameter, soweit moeglich
   Die vernutzten Argumente werden auf NULL gesetzt.
*/

void PA_AufrufParameterEinstellen(int argc, char *argv[], mode Modus);
/* Einstellen der beim Aufruf mitgegebenen Parameter. argc und argv sind die Parameter von main,
   es wird also davon ausgegangen, dass argv[0] den Programmnamen enthaelt, und das argv[argc] der
   Name der zu bearbeitenden Spezifikation ist. Die Angaben zwischen argv[1] und argv[argc-1] werden
   ausgewertet. Dabei werden die in Parameter.c spezifizierten Aufruf-Optionen herangezogen.
   Inkorrekte Parameter fuehren zum Programmabbruch.
*/

void PA_InitAposterioriEarly (void);
/* Aufruf vor PM_SpezifikationAnalysieren() */
   
void PA_InitAposteriori(void);
/* Daten aus der eingelesenen Spezifikation auswerten.
   Anschliessend die betroffenen Module mit den eingelesenen Parametern
   aposteriori initialisieren.
*/

/* -----------------------------------------------------*/
/* -------- Eingestellte Modi ausgeben -----------------*/
/* -----------------------------------------------------*/

void PA_PrintParas(void);

void PA_DruckeModi(void);
/* Gibt die eingestellten Modi aus.
   Verwendung bei Ausgabe der Statistik.*/

void PA_DefaultsAusgeben(void);

#endif

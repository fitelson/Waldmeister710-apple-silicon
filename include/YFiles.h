/*=================================================================================================================
  =                                                                                                               =
  =  YFiles.h                                                                                                     =
  =                                                                                                               =
  =  Kapselung der Zeichenketten fuer kommandozeilenartige Parametereinstellungen                                 =
  =                                                                                                               =
  =  18.08.98 Aus PhilMarlow.c extrahiert. Thomas.                                                                =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef YFILES_H
#define YFILES_H

#include "XFilesFuerPMh.h"
#include "Praezedenzgenerator.h"

typedef enum {add, Add, mix, gt, Mix, pol, ngt} HeuristikartenT;
typedef enum {rnf, small, mnf} ZielbehandlungsartenT;
typedef enum {cp, cg} PassivaartenT;
typedef enum {Morgen, Mittag, Abend} SchrittzahlT;
typedef enum {da, re, mi, so} VerschnittT;

void empty(void);


void lpo_(PraezedenzartenT,Signatur_SymbolT,Signatur_ZeichenT);
void kbo_(PraezedenzartenT,Signatur_SymbolT,Signatur_ZeichenT);
void nao(PraezedenzartenT);
void cph_(HeuristikartenT,Signatur_SymbolT,Signatur_ZeichenT);
void cgh_(HeuristikartenT,Signatur_SymbolT,Signatur_ZeichenT);
void zb(ZielbehandlungsartenT);
void gj(int);
void einsstern(void);
void golem(void);
void cgcp(void);
void back(void);
void rose(void);
void kern(void);
void bis(SchrittzahlT);
void itl(VerschnittT);
void cc_(char[],Signatur_SymbolT,Signatur_ZeichenT);

#define /*void*/ lpo(/*PraezedenzartenT*/ Art)  lpo_(Art,*sig,*sig_c)
#define /*void*/ kbo(/*PraezedenzartenT*/ Art)  kbo_(Art,*sig,*sig_c)
#define /*void*/ cph(/*HeuristikartenT*/ Art)   cph_(Art,*sig,*sig_c)
#define /*void*/ cgh(/*HeuristikartenT*/ Art)   cgh_(Art,*sig,*sig_c)
#define /*void*/ cc(/*char*/ welche /*[]*/)     cc_(welche,*sig,*sig_c)

#endif

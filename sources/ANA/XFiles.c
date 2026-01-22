/*=================================================================================================================
  =                                                                                                               =
  =  XFiles.c                                                                                                     =
  =                                                                                                               =
  =  Typdefinitionen und Iteratoren fuer Gesetzestafeln                                                           =
  =                                                                                                               =
  =  28.04.98 Aus XFiles.h extrahiert. Thomas.                                                                    =
  =                                                                                                               =
  =================================================================================================================*/


#include "XFiles.h"


/*=================================================================================================================*/
/*= III. GesetzT und Tafel1 mit Iteratoren                                                                        =*/
/*=================================================================================================================*/

#define __ Mantelung(
#define _ ,__
#if XF_TEST
#define Mantelung(Gesetzname,Signatur,Gleichung) {Gesetzname,#Gesetzname,Signatur,Gleichung, NULL,{0}}
#else
#define Mantelung(Gesetzname,Signatur,Gleichung) {Gesetzname,            Signatur,Gleichung, NULL,{0}}
#endif

GesetzT Tafel1[AnzGesetze+1] = {
  Tafel1(),
  Mantelung(Unbekannt, "", "")
};

#undef Mantelung



/*=================================================================================================================*/
/*= IV. StrukturnamenT und Tafel2 mit Iteratoren                                                                  =*/
/*=================================================================================================================*/

#define Mantelung(Strukturname, Strategien) {Strukturname,#Strukturname}

StrukturT Tafel2[AnzStrukturen+1] = {
  Tafel2(),
  Mantelung(Orkus,OrkusStrategie())
};

#undef Mantelung



/*=================================================================================================================*/
/*= V. PraemissenT, ZuordnungsT und Tafel3 mit Iteratoren                                                         =*/
/*=================================================================================================================*/

#define Mantelung(Strukturname, Signatur, Praemissen) {Strukturname,Signatur,{Praemissen}, {0}}
#define ____(X,Y) {X,Y}
#define ___(X,Y,Z) ____(X,Y),Z

ZuordnungsT Tafel3[] = {
  Tafel3()
};

#undef Mantelung


const unsigned int AnzZuordnungen = sizeof(Tafel3)/sizeof(ZuordnungsT);

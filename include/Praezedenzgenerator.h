/*=================================================================================================================
  =                                                                                                               =
  =  Praezedenzgenerator.h                                                                                        =
  =                                                                                                               =
  =                                                                                                               =
  =  26.07.97 Angelegt. Thomas.                                                                                   =
  =  19.06.98 Durch Ausduennen aus altem Ordnungsgenerator erzeugt. Thomas.                                       =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef PRAEZEDENZGENERATOR_H
#define PRAEZEDENZGENERATOR_H

#include "parser.h"
#include "XFilesFuerPMh.h"


/*=================================================================================================================*/
/*= I. Tafel4 und Aufzaehlungstyp PraezedenzartenT                                                                =*/
/*=================================================================================================================*/

#define Tafel4()                                                                                                 ___\
  lat, semantisch,   "*UO/1",    {_(2,_(2,_(2,_(1,0))))},                "*O/U1ecp"                              )__\
  Lat, semantisch,   "*UO/1",    {_(2,_(2,_(2,_(1,0))))},                "/*UO1ecp"                              )__\
  inv, semantisch,   "*UO/1ABC", {_(2,_(2,_(2,_(1,_(0,_(0,_(0,0)))))))}, "/OU*CABe1cp"                           )__\
  nar, semantisch,   "A+*C-0",   {_(3,_(2,_(2,_(2,_(1,0)))))},           "CA*-+0ecp"                             )__\
  std, syntaktisch,  "",         {0},                                    ""                                      )__\
  Std, syntaktisch,  "",         {0},                                    ""                                      )  \

#if 0

Syntaxconstraints fuer semantische Praezedenzen
-----------------------------------------------

Die Benennung ist eine korrekt gebildete Operatorsignatur nach den Konventionen von Phil Marlow.
Jedes Zeichen aus der Benennung kommt im Praezedenzmuster vor.
Jedes der Metazeichen e, c, und p kommt im Praezedenzmuster vor.
Jedes Zeichen im Praezedenzmuster kommt in der Benennung vor oder ist ein Metazeichen.
Im Praezedenzmuster kommt kein Zeichen mehrfach vor.
Die Operatoren in der Benennung sind nach fallender Stelligkeit angeordnet.
Beim Aufruf der Praezedenzgenerierung wird eine Signatur aus Operatoren uebergeben.
  Diese hat dieselbe Laenge wie die Benennung.
  Die Stelligkeiten der uebergebenen Signatur entsprechen den in Tafel 4 deklarierten.
     
#endif


#define ___ Mantelung(
#define __ ,___
#define Mantelung(Kurzname,Gattung,Benennung,Stelligkeiten,Muster) Kurzname
#define _(X,Y) X,Y

typedef enum {
  Tafel4()
} PraezedenzartenT;

#undef Mantelung
#undef ___
#undef __
#undef _



/*=================================================================================================================*/
/*= V. Verteilerfunktion                                                                                          =*/
/*=================================================================================================================*/

char *PG_Praezedenz(PraezedenzartenT Art, Signatur_SymbolT sig, Signatur_ZeichenT sig_c, ordering Familie);



#endif

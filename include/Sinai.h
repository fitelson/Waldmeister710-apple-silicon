/*=================================================================================================================
  =                                                                                                               =
  =  Sinai.h                                                                                                      =
  =                                                                                                               =
  =  Definition der Gesetzes- und Strukturtafeln                                                                  =
  =                                                                                                               =
  =  27.04.98 Aus Ordnungsgenerator extrahiert. Thomas.                                                           =
  =                                                                                                               =
  =================================================================================================================*/


#ifndef SINAI_H
#define SINAI_H


/*=================================================================================================================*/
/*= I. Tafeln                                                                                                     =*/
/*=================================================================================================================*/

#define Tafel1()                                                                                                  __\
  Assoziativitaet,        "+",    "+(+(x,y),z) = +(x,+(y,z))"                                                     )_\
  Linksidentitaet,        "+0",   "+(0,x) = x"                                                                    )_\
  Rechtsidentitaet,       "+0",   "+(x,0) = x"                                                                    )_\
  Linksinverses,          "+-0",  "+(-(x),x) = 0"                                                                 )_\
  Rechtsinverses,         "+-0",  "+(x,-(x)) = 0"                                                                 )_\
  Linksdistributivitaet,  "*+",   "*(+(y,z),x) = +(*(y,x),*(z,x))"                                                )_\
  Rechtsdistributivitaet, "*+",   "*(x,+(y,z)) = +(*(x,y),*(x,z))"                                                )_\
  Linksloesbarkeit,       "*/",   "*(/(x,y),y) = x"                                                               )_\
  Rechtsloesbarkeit,      "*\\",  "*(x,\\(x,y)) = y"                                                              )_\
/*Linkseindeutigkeit,     "/ *",   "/(*(x,y),y) = x"                                                              */\
/*Rechtseindeutigkeit,    "\\*",  "\\(x,*(x,y)) = y"                                                              */\
  Kommutativitaet,        "F",    "F(x,y) = F(y,x)"                                                               )_\
  Idempotenz,             "F",    "F(x,x) = x"                                                                    )_\
  TernaerAssoziativitaet, "*",    "*(*(x,y,z),u,*(x,y,v)) = *(x,y,*(z,u,v))"                                      )_\
  Linksdominanz,          "*",    "*(x,x,y) = x"                                                                  )_\
  Rechtsdominanz,         "*",    "*(x,y,y) = y"                                                                  )_\
  TernaerLinksinverses,   "*-",   "*(-(x),x,y) = y"                                                               )_\
  TernaerRechtsinverses,  "*-",   "*(x,y,-(y)) = x"                                                               )_\
  VerbandAbsorbtion,      "OU",   "O(x,U(x,y)) = x"                                                               )_\
  RobbinsRR,              "ON",   "N(O(N(O(x,y)),N(O(x,N(y))))) = x"                                              )_\
  RobbinsRL,              "ON",   "N(O(N(O(x,y)),N(O(N(x),y)))) = y"                                              )_\
  RobbinsLR,              "ON",   "N(O(N(O(x,N(y))),N(O(x,y)))) = x"                                              )_\
  RobbinsLL,              "ON",   "N(O(N(O(N(x),y)),N(O(x,y)))) = y"                                              )_\
  Assoziator,             "A+*-", "A(x,y,z) = +(*(*(x,y),z),-(*(x,*(y,z))))"                                      )_\
  Kommutator,             "C+*-", "C(x,y) = +(*(y,x),-(*(x,y)))"                                                  )_\
  Linksalternative,       "*",    "*(*(x,x),y) = *(x,*(x,y))"                                                     )_\
  Rechtsalternative,      "*",    "*(*(x,y),y) = *(x,*(y,y))"                                                     )_\
/*Wajsberg1,              "IT",   "I(T,x) = x"                                                                    */\
  Wajsberg2,              "IT",   "I(I(x,y),I(I(y,z),I(x,z))) = T"                                                )_\
  Wajsberg3,              "I",    "I(I(x,y),y) = I(I(y,x),x)"                                                     )_\
  Wajsberg4,              "I!T",  "I(I(!(x),!(y)),I(y,x)) = T"                                                    )_\
  AlternativeWajsberg1,   "^!T",  "!(x) = ^(x,T)"                                                                 )_\
/*AlternativeWajsberg2,   "^F",   "^(x,F) = x"                                                                    */\
  AlternativeWajsberg3,   "^F",   "^(x,x) = F"                                                                    )_\
/*AlternativeWajsberg4,   "*T",   "*(x,T) = x"                                                                    */\
  AlternativeWajsberg5,   "*F",   "*(x,F) = F"                                                                    )_\
  AlternativeWajsberg6,   "*^TF", "*(^(T,x),x) = F"                                                               )_\
  AlternativeWajsberg7,   "^T",   "^(x,^(T,y)) = ^(^(x,T),y)"                                                     )_\
  AlternativeWajsberg8,   "*^T",  "*(^(*(^(T,x),y),T),y) = *(^(*(^(T,y),x),T),x)"                                 )_\
  LDAxiom,                "F",    "F(x,F(y,z)) = F(F(x,y),F(x,z))"                                                )_\
  SKombinator,            "_S",   "_(_(_(S,x),y),z) = _(_(x,z),_(y,z))"                                           )_\
/*KKombinator,            "_K",   "_(_(K,x),y) = x"                                                               */\
  BKombinator,            "_B",   "_(_(_(B,x),y),z) = _(x,_(y,z))"                                                )_\
  TKombinator,            "_T",   "_(_(T,x),y) = _(y,x)"                                                          )_\
  OKombinator,            "_O",   "_(_(O,x),y) = _(y,_(x,y))"                                                     )_\
  QKombinator,            "_Q",   "_(_(_(Q,x),y),z) = _(y,_(x,z))"                                                ) \

#define Wajsberg1                 Linksidentitaet
#define AlternativeWajsberg2      Rechtsidentitaet
#define AlternativeWajsberg4      Rechtsidentitaet
#define Linkseindeutigkeit        Linksloesbarkeit
#define Rechtseindeutigkeit       Rechtsloesbarkeit


#define StdS                      kbo(std),itl(mi),zb(mnf)
#define GtS                       kbo(std),cph(gt),itl(mi)
#define KombS                     rose(),kbo(std),cph(add),bis(Morgen), kern()

#define Tafel2()                                                                                                  __\
  BoolescheAlgebra,               ( cc("-"),GtS                                                                  ))_\
  TernaereBoolescheAlgebra,       ( GtS                                                                          ))_\
  Gruppe,                         ( cc("-+"),GtS                                                                 ))_\
  VerbandsgeordneteGruppe,        ( cc("/*"),lpo(lat),itl(da),zb(mnf),gj(2),bis(Abend),                             \
                                    cc("/*"),kbo(Lat),cph(gt),itl(da),gj(2)                                      ))_\
  WajsbergAlgebra,                ( StdS                                                                         ))_\
  AlternativeWajsbergAlgebra,     ( StdS                                                                         ))_\
  Verband,                        ( kbo(Std),cph(gt),gj(1)                                                       ))_\
  RobbinsAlgebra,                 ( kbo(std),cph(ngt),itl(so),gj(2),cgcp()                                       ))_\
  Ring,                           ( cc("-+"),kbo(Std)                                                            ))_\
  NichtassoziativerRing,          ( cc("-+"),lpo(nar),cph(pol)                                                   ))_\
  Quasigruppe,                    ( itl(re),StdS,cc("*\\/")                                                      ))_\
  Praeloop,                       ( itl(re),StdS                                                                 ))_\
  Loop,                           ( itl(re),StdS,cc("*\\/")                                                      ))_\
  LDAlgebra,                      ( StdS                                                                         ))_\
  KombinatorlogikQ,               ( KombS                                                                        ))_\
  KombinatorlogikB,               ( KombS                                                                        ))_\
  KombinatorlogikO,               ( KombS                                                                        ))_\
  KombinatorlogikS,               ( KombS                                                                        ))_\
  KombinatorlogikBT,              ( rose(),kbo(std),cgh(add),einsstern()                                         )) \

#define Orkusstrategien           ( StdS                                                                          ) \


#define Tafel3()                                                                                                  __\
       BoolescheAlgebra           ,"+*-01"                                                                     ,___(\
                   Linksidentitaet,"+0"                                                                        ,___(\
                     Linksinverses,"+-1"                                                                       ,___(\
                   Kommutativitaet,"+"                                                                         ,___(\
                   Linksidentitaet,"*1"                                                                        ,___(\
                     Linksinverses,"*-0"                                                                       ,___(\
                   Kommutativitaet,"*"                                                                         ,___(\
             Linksdistributivitaet,"*+"                                                                       ,____(\
             Linksdistributivitaet,"+*"                                                                   )))))))))_\
                                                                                                                    \
       BoolescheAlgebra           ,"+*-01"                                                                     ,___(\
                  Rechtsidentitaet,"+0"                                                                        ,___(\
                    Rechtsinverses,"+-1"                                                                       ,___(\
                   Kommutativitaet,"+"                                                                         ,___(\
                  Rechtsidentitaet,"*1"                                                                        ,___(\
                    Rechtsinverses,"*-0"                                                                       ,___(\
                   Kommutativitaet,"*"                                                                         ,___(\
            Rechtsdistributivitaet,"*+"                                                                       ,____(\
            Rechtsdistributivitaet,"+*"                                                                   )))))))))_\
                                                                                                                    \
       TernaereBoolescheAlgebra   ,"*-"                                                                        ,___(\
            TernaerAssoziativitaet,"*"                                                                         ,___(\
                     Linksdominanz,"*"                                                                         ,___(\
                    Rechtsdominanz,"*"                                                                        ,____(\
              TernaerLinksinverses,"*-"                                                                       )))))_\
                                                                                                                    \
       TernaereBoolescheAlgebra   ,"*-"                                                                        ,___(\
            TernaerAssoziativitaet,"*"                                                                         ,___(\
                     Linksdominanz,"*"                                                                         ,___(\
                    Rechtsdominanz,"*"                                                                        ,____(\
             TernaerRechtsinverses,"*-"                                                                       )))))_\
                                                                                                                    \
       TernaereBoolescheAlgebra   ,"*-"                                                                        ,___(\
            TernaerAssoziativitaet,"*"                                                                         ,___(\
                     Linksdominanz,"*"                                                                         ,___(\
              TernaerLinksinverses,"*-"                                                                       ,____(\
             TernaerRechtsinverses,"*-"                                                                       )))))_\
                                                                                                                    \
       TernaereBoolescheAlgebra   ,"*-"                                                                        ,___(\
            TernaerAssoziativitaet,"*"                                                                         ,___(\
                    Rechtsdominanz,"*"                                                                         ,___(\
              TernaerLinksinverses,"*-"                                                                       ,____(\
             TernaerRechtsinverses,"*-"                                                                       )))))_\
                                                                                                                    \
       Gruppe                     ,"+-0"                                                                       ,___(\
                   Assoziativitaet,"+"                                                                         ,___(\
                   Linksidentitaet,"+0"                                                                       ,____(\
                     Linksinverses,"+-0"                                                                       ))))_\
                                                                                                                    \
       Gruppe                     ,"+-0"                                                                       ,___(\
                   Assoziativitaet,"+"                                                                         ,___(\
                  Rechtsidentitaet,"+0"                                                                       ,____(\
                    Rechtsinverses,"+-0"                                                                       ))))_\
                                                                                                                    \
       VerbandsgeordneteGruppe    ,"*UO/1"                                                                     ,___(\
                   Assoziativitaet,"*"                                                                         ,___(\
                   Linksidentitaet,"*1"                                                                        ,___(\
                     Linksinverses,"*/1"                                                                       ,___(\
                   Assoziativitaet,"U"                                                                         ,___(\
                   Kommutativitaet,"U"                                                                         ,___(\
                        Idempotenz,"U"                                                                         ,___(\
             Linksdistributivitaet,"*U"                                                                        ,___(\
            Rechtsdistributivitaet,"*U"                                                                        ,___(\
                   Assoziativitaet,"O"                                                                         ,___(\
                   Kommutativitaet,"O"                                                                         ,___(\
                        Idempotenz,"O"                                                                         ,___(\
             Linksdistributivitaet,"*O"                                                                        ,___(\
            Rechtsdistributivitaet,"*O"                                                                        ,___(\
                 VerbandAbsorbtion,"OU"                                                                       ,____(\
                 VerbandAbsorbtion,"UO"                                                            ))))))))))))))))_\
                                                                                                                    \
       VerbandsgeordneteGruppe    ,"*UO/1"                                                                     ,___(\
                   Assoziativitaet,"*"                                                                         ,___(\
                  Rechtsidentitaet,"*1"                                                                        ,___(\
                    Rechtsinverses,"*/1"                                                                       ,___(\
                   Assoziativitaet,"U"                                                                         ,___(\
                   Kommutativitaet,"U"                                                                         ,___(\
                        Idempotenz,"U"                                                                         ,___(\
             Linksdistributivitaet,"*U"                                                                        ,___(\
            Rechtsdistributivitaet,"*U"                                                                        ,___(\
                   Assoziativitaet,"O"                                                                         ,___(\
                   Kommutativitaet,"O"                                                                         ,___(\
                        Idempotenz,"O"                                                                         ,___(\
             Linksdistributivitaet,"*O"                                                                        ,___(\
            Rechtsdistributivitaet,"*O"                                                                        ,___(\
                 VerbandAbsorbtion,"OU"                                                                       ,____(\
                 VerbandAbsorbtion,"UO"                                                            ))))))))))))))))_\
                                                                                                                    \
       RobbinsAlgebra             ,"ON"                                                                        ,___(\
                   Kommutativitaet,"O"                                                                         ,___(\
                   Assoziativitaet,"O"                                                                        ,____(\
                         RobbinsRR,"ON"                                                                        ))))_\
                                                                                                                    \
       RobbinsAlgebra             ,"ON"                                                                        ,___(\
                   Kommutativitaet,"O"                                                                         ,___(\
                   Assoziativitaet,"O"                                                                        ,____(\
                         RobbinsRL,"ON"                                                                        ))))_\
                                                                                                                    \
       RobbinsAlgebra             ,"ON"                                                                        ,___(\
                   Kommutativitaet,"O"                                                                         ,___(\
                   Assoziativitaet,"O"                                                                        ,____(\
                         RobbinsLR,"ON"                                                                        ))))_\
                                                                                                                    \
       RobbinsAlgebra             ,"ON"                                                                        ,___(\
                   Kommutativitaet,"O"                                                                         ,___(\
                   Assoziativitaet,"O"                                                                        ,____(\
                         RobbinsLL,"ON"                                                                        ))))_\
                                                                                                                    \
       Ring                       ,"+*-0"                                                                      ,___(\
                   Assoziativitaet,"+"                                                                         ,___(\
                   Linksidentitaet,"+0"                                                                        ,___(\
                     Linksinverses,"+-0"                                                                       ,___(\
                   Kommutativitaet,"+"                                                                         ,___(\
                   Assoziativitaet,"*"                                                                         ,___(\
             Linksdistributivitaet,"*+"                                                                       ,____(\
            Rechtsdistributivitaet,"*+"                                                                    ))))))))_\
                                                                                                                    \
       Ring                       ,"+*-0"                                                                      ,___(\
                   Assoziativitaet,"+"                                                                         ,___(\
                  Rechtsidentitaet,"+0"                                                                        ,___(\
                    Rechtsinverses,"+-0"                                                                       ,___(\
                   Kommutativitaet,"+"                                                                         ,___(\
                   Assoziativitaet,"*"                                                                         ,___(\
             Linksdistributivitaet,"*+"                                                                       ,____(\
            Rechtsdistributivitaet,"*+"                                                                    ))))))))_\
                                                                                                                    \
       NichtassoziativerRing      ,"A+*C-0"                                                                    ,___(\
                   Assoziativitaet,"+"                                                                         ,___(\
                   Linksidentitaet,"+0"                                                                        ,___(\
                     Linksinverses,"+-0"                                                                       ,___(\
                   Kommutativitaet,"+"                                                                         ,___(\
                  Linksalternative,"*"                                                                         ,___(\
                 Rechtsalternative,"*"                                                                         ,___(\
             Linksdistributivitaet,"*+"                                                                        ,___(\
            Rechtsdistributivitaet,"*+"                                                                        ,___(\
                        Assoziator,"A+*-"                                                                     ,____(\
                        Kommutator,"C+*-"                                                               )))))))))))_\
                                                                                                                    \
       NichtassoziativerRing      ,"A+*C-0"                                                                    ,___(\
                   Assoziativitaet,"+"                                                                         ,___(\
                  Rechtsidentitaet,"+0"                                                                        ,___(\
                    Rechtsinverses,"+-0"                                                                       ,___(\
                   Kommutativitaet,"+"                                                                         ,___(\
                  Linksalternative,"*"                                                                         ,___(\
                 Rechtsalternative,"*"                                                                         ,___(\
             Linksdistributivitaet,"*+"                                                                        ,___(\
            Rechtsdistributivitaet,"*+"                                                                        ,___(\
                        Assoziator,"A+*-"                                                                     ,____(\
                        Kommutator,"C+*-"                                                               )))))))))))_\
                                                                                                                    \
       NichtassoziativerRing      ,"A+*C-0"                                                                    ,___(\
                   Assoziativitaet,"+"                                                                         ,___(\
                   Linksidentitaet,"+0"                                                                        ,___(\
                     Linksinverses,"+-0"                                                                       ,___(\
                   Kommutativitaet,"+"                                                                         ,___(\
                  Linksalternative,"*"                                                                         ,___(\
             Linksdistributivitaet,"*+"                                                                        ,___(\
            Rechtsdistributivitaet,"*+"                                                                        ,___(\
                        Assoziator,"A+*-"                                                                     ,____(\
                        Kommutator,"C+*-"                                                                ))))))))))_\
                                                                                                                    \
       NichtassoziativerRing      ,"A+*C-0"                                                                    ,___(\
                   Assoziativitaet,"+"                                                                         ,___(\
                  Rechtsidentitaet,"+0"                                                                        ,___(\
                    Rechtsinverses,"+-0"                                                                       ,___(\
                   Kommutativitaet,"+"                                                                         ,___(\
                  Linksalternative,"*"                                                                         ,___(\
             Linksdistributivitaet,"*+"                                                                        ,___(\
            Rechtsdistributivitaet,"*+"                                                                        ,___(\
                        Assoziator,"A+*-"                                                                     ,____(\
                        Kommutator,"C+*-"                                                                ))))))))))_\
                                                                                                                    \
       NichtassoziativerRing      ,"A+*C-0"                                                                    ,___(\
                   Assoziativitaet,"+"                                                                         ,___(\
                   Linksidentitaet,"+0"                                                                        ,___(\
                     Linksinverses,"+-0"                                                                       ,___(\
                   Kommutativitaet,"+"                                                                         ,___(\
                 Rechtsalternative,"*"                                                                         ,___(\
             Linksdistributivitaet,"*+"                                                                        ,___(\
            Rechtsdistributivitaet,"*+"                                                                        ,___(\
                        Assoziator,"A+*-"                                                                     ,____(\
                        Kommutator,"C+*-"                                                                ))))))))))_\
                                                                                                                    \
       NichtassoziativerRing      ,"A+*C-0"                                                                    ,___(\
                   Assoziativitaet,"+"                                                                         ,___(\
                  Rechtsidentitaet,"+0"                                                                        ,___(\
                    Rechtsinverses,"+-0"                                                                       ,___(\
                   Kommutativitaet,"+"                                                                         ,___(\
                 Rechtsalternative,"*"                                                                         ,___(\
             Linksdistributivitaet,"*+"                                                                        ,___(\
            Rechtsdistributivitaet,"*+"                                                                        ,___(\
                        Assoziator,"A+*-"                                                                     ,____(\
                        Kommutator,"C+*-"                                                                ))))))))))_\
                                                                                                                    \
       WajsbergAlgebra            ,"I!T"                                                                       ,___(\
                         Wajsberg1,"IT"                                                                        ,___(\
                         Wajsberg2,"IT"                                                                        ,___(\
                         Wajsberg3,"I"                                                                        ,____(\
                         Wajsberg4,"I!T"                                                                      )))))_\
                                                                                                                    \
       AlternativeWajsbergAlgebra ,"^*!TF"                                                                     ,___(\
              AlternativeWajsberg1,"^!T"                                                                       ,___(\
              AlternativeWajsberg2,"^F"                                                                        ,___(\
              AlternativeWajsberg3,"^F"                                                                        ,___(\
              AlternativeWajsberg4,"*T"                                                                        ,___(\
              AlternativeWajsberg5,"*F"                                                                        ,___(\
              AlternativeWajsberg6,"*^TF"                                                                      ,___(\
              AlternativeWajsberg7,"^T"                                                                       ,____(\
              AlternativeWajsberg8,"*^T"                                                                  )))))))))_\
                                                                                                                    \
       LDAlgebra                  ,"F"                                                                        ,____(\
                           LDAxiom,"F"                                                                           ))_\
                                                                                                                    \
       KombinatorlogikS           ,"_S"                                                                       ,____(\
                       SKombinator,"_S"                                                                          ))_\
                                                                                                                    \
       KombinatorlogikB           ,"_B"                                                                       ,____(\
                       BKombinator,"_B"                                                                          ))_\
                                                                                                                    \
       KombinatorlogikBT          ,"_BT"                                                                       ,___(\
                       BKombinator,"_B"                                                                       ,____(\
                       TKombinator,"_T"                                                                         )))_\
                                                                                                                    \
       KombinatorlogikO           ,"_O"                                                                       ,____(\
                       OKombinator,"_O"                                                                          ))_\
                                                                                                                    \
       KombinatorlogikQ           ,"_Q"                                                                       ,____(\
                       QKombinator,"_Q"                                                                          ))_\
                                                                                                                    \
       Verband                    ,"OU"                                                                        ,___(\
                   Assoziativitaet,"O"                                                                         ,___(\
                   Kommutativitaet,"O"                                                                         ,___(\
                   Assoziativitaet,"U"                                                                        ,____(\
                   Kommutativitaet,"U"                                                                        )))))_\
                                                                                                                    \
       Quasigruppe                ,"*\\/"                                                                      ,___(\
                  Linksloesbarkeit,"*/"                                                                        ,___(\
                 Rechtsloesbarkeit,"*\\"                                                                       ,___(\
                Linkseindeutigkeit,"/*"                                                                       ,____(\
               Rechtseindeutigkeit,"\\*"                                                                      )))))_\
                                                                                                                    \
       Praeloop                   ,"*1"                                                                        ,___(\
                   Linksidentitaet,"*1"                                                                       ,____(\
                  Rechtsidentitaet,"*1"                                                                         )))_\
                                                                                                                    \
       Loop                       ,"*\\/1"                                                                     ,___(\
                  Linksloesbarkeit,"*/"                                                                        ,___(\
                 Rechtsloesbarkeit,"*\\"                                                                       ,___(\
                Linkseindeutigkeit,"/*"                                                                        ,___(\
               Rechtseindeutigkeit,"\\*"                                                                       ,___(\
                   Linksidentitaet,"*1"                                                                       ,____(\
                  Rechtsidentitaet,"*1"                                                                     ))))))) \



/*=================================================================================================================*/
/*= II. Kommentar: Syntaxconstraints                                                                              =*/
/*=================================================================================================================*/

#if 0

Tafel1
------
Es muss mindestens ein Gesetzname angegeben werden.
Die zu je zwei Gesetzen gehoerenden Gleichungen duerfen keine Varianten sein, auch nicht ueber Kreuz.
  Synonyme koennen aber ausserhalb von Tafel1 mittels #define festgelegt werden.
Fuer jedes Gesetz:
  Jeder Term muss syntaktisch korrekt aufgebaut sein.
    Dies beinhaltet die Korrektheit von Operator- und Variablensymbolen
    und die Stelligkeitskonsistenz der Operatoren.
    Variablen sind Kleinbuchstaben,
    Syntaxzeichen sind Komma, runde Klammern, Leer- und Gleichheitszeichen,
    Operatorzeichen der druckbare Rest.
    In sowie vor oder nach Termen stehen keine Leerzeichen.
    Beachte: Konstanten werden ohne Klammern geschrieben.
  Jeder Gleichungsstring muss syntaktisch korrekt aufgebaut sein.
    Der String darf nicht leer sein.
    Auch im gesamten Gleichungsstring muss jeder Operator von konstanter Stelligkeit sein.
    Zwischen den Termen wird der Trenner " = " erwartet.
  Es duerfen nicht mehr als SHRT_MIN Variablen in einem Gleichungsstring verwendet werden.
  In der Signatur muessen die Operatoren je verschiedene gueltige Operatorzeichen sein.
    Sie sind nach fallender Stelligkeit und Auftreten anzuordnen. 
    Die Signatur darf nicht leer sein und auch nicht laenger als maxAnzOperatoren.
  Die Mengen der in der Signatur und im Gleichungsstring verwendeten Operatoren muessen gleich sein.
  Das Gesetz muss in einer Praemisse einer Zuordnung in Tafel3 verwendet werden.


Tafel2
------
Es muss mindestens ein Strukturname angegeben werden.
Fuer jede Struktur:
  In Tafel3 muss mindestens eine Zuordnung auf diese Struktur vorgenommen werden.
  Der Strategiensatz soll aus einer Aneinanderreihung von Strategien bestehen,
    getrennt von Strategiewechselerzeugern "bis(n)".
  Die einzelnen Strategien sollen aus einer Aneinanderreihung von Optionsproduzenten bestehen.
  Bei den Strategiewechselerzeugern "bis(n)" muss die Schrittzahl n positiv sein.
  Mit "empty()" wird ein leerer Optionsstring produziert.
  Der Optionsproduzent "kern()" bewirkt implizit einen Strategienwechsel,
    wenn die Beweisaufgabe unpassend ist.
  Steht ein Strategiewechselerzeuger direkt vor einem weiteren oder am Anfang oder am Ende des Strategiensatzes,
    so erzeugt dies an entsprechender Position jeweils die Strategie mit leerem Optionsstring.


Tafel3
------
Es muss mindestens eine Zuordnung angegeben werden.
Fuer jede Zuordnung:
  Im Namensfeld muss ein gueltiger Strukturname eingetragen sein.
  In der Signatur muessen die Operatoren je verschiedene gueltige Operatorzeichen sein.
    Sie sind nach fallender Stelligkeit, nicht jedoch nach Auftreten anzuordnen. 
    Die Signatur darf nicht leer sein und auch nicht laenger als maxAnzOperatoren.
  Die Mengen der in der Signatur und in den Signaturen der Praemissen verwendeten Operatoren muessen gleich sein.
  Tritt ein Operator in mehreren Praemissenfeldern auf, so muss er stets dieselbe Stelligkeit haben.
  Es sind ein bis maxAnzPraemissen viele Praemissen anzugeben.
  Je zwei Praemissen muessen sich im Gesetznamen oder in der Signatur unterscheiden.
  Fuer jede Praemisse:
    Im Namensfeld muss ein gueltiger Gesetzname eingetragen sein.
    Wie bei Tafel1 gilt: In der Signatur muessen die Operatoren je verschiedene gueltige Operatorzeichen sein.
      Die Signatur darf nicht leer sein und auch nicht laenger als maxAnzOperatoren.
    Die Signatur muss dieselbe Laenge haben wie in der Definition des Gesetzes (aber nicht dieselben Zeichen!).
Fuer je zwei Zuordnungen auf dieselbe Struktur:
  Die Signaturen muessen als Zeichenketten identisch sein.
  Fuer jeden Operator:
    Die Stelligkeiten in den beiden Signaturen sind dieselbe.
#endif

#endif

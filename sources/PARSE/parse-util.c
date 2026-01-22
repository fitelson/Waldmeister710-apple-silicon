#include "parse-util.h"
#include "ConfluenceTrees.h"
#include "FehlerBehandlung.h"
#include "Grundzusammenfuehrung.h"
#include "RUndEVerwaltung.h"
#include "Ordnungen.h"
#include "NFBildung.h"
#include "Constraints.h"
#include "Unifikation1.h"
#include <malloc.h>
#include <string.h>

#include <sys/time.h>

extern void yyerror(char *);

int arity;
eqmode_type eqmode;
static func_symb nextFSym = 1;
static int nextVar = -1;
static struct orderdecS the_order = {-1, NULL};
ordering_type PA_Ordering;
static mode_type mode;

/* Symboltabelle : */
typedef struct P_Symbol_TS {
  char *name;
  func_symb fsym;
  SymbolT wmsym;
  int no_args;
  int weight;
  int type;
} P_Symbol_T, *P_Symbol_Ptr;

static P_Symbol_Ptr *Symbole;
static int next_symbol = 0;
static int no_of_symbols = 0;
#define INITIAL_SIZE 20
#define DELTA_SIZE 20

static int noexpected = 0;
static char *typenames[] = {"SORT", "FUNCTION", "VARIABLE"};
char *error_msg = NULL;
static size_t error_msg_len;

static void adapt_error_msg(char *name, int i)
{
  unsigned int len = strlen(name) + i;
  if (error_msg == NULL) {
    error_msg_len = len;
    error_msg = malloc (error_msg_len);
  }
  else if (len > error_msg_len) {
    error_msg_len += len;
    error_msg = realloc (error_msg, error_msg_len);
  }
}

int add_symbol (char *name, sym_type tp, int no_args)
{
#if 0  
  printf ("check_symbol: %s - %d\n", name, no_args);
#endif
  {
    int i;
    for (i = 0; i < next_symbol; i++) {
      if (strcmp (Symbole[i]->name, name) == 0)	{
        adapt_error_msg(name, 60);
        sprintf (error_msg,
                 "`%s' previously defined as %s-symbol!",
                 name, typenames[Symbole[i]->type]);
        return 0;
      }
    }
    if (next_symbol == 0){
      no_of_symbols = INITIAL_SIZE;
      Symbole = malloc (no_of_symbols * sizeof(P_Symbol_Ptr));
    }
    else if (next_symbol == no_of_symbols){
      no_of_symbols = no_of_symbols + DELTA_SIZE;
      Symbole = realloc (Symbole, sizeof(P_Symbol_Ptr) * no_of_symbols);
    }
    Symbole[next_symbol]          = malloc(sizeof(P_Symbol_T));
    Symbole[next_symbol]->name    = strdup (name);
    Symbole[next_symbol]->no_args = no_args;
    Symbole[next_symbol]->type    = tp;
    if (tp == fun_sym){
      Symbole[next_symbol]->weight = 1;
      Symbole[next_symbol]->fsym   = nextFSym;
      Symbole[next_symbol]->wmsym  = SN_WMvonDISCOUNT(nextFSym);
      nextFSym++;
    }
    else if (tp == var_sym) {
      Symbole[next_symbol]->weight = 0;
      Symbole[next_symbol]->fsym   = 0;
      Symbole[next_symbol]->wmsym  = nextVar;
      nextVar--;
    }

    next_symbol++;

    return 1;
  }
}

static P_Symbol_Ptr lookup_Name (char *name)
{
  int i;
  for (i = 0; i < next_symbol; i++){
    if(strcmp(Symbole[i]->name, name) == 0){
      return Symbole[i];
    }
  }
  return NULL;
}

int get_symbol_type (char *name)
{
#if 0  
  printf ("get_symbol_type: %s\n", name);
#endif
  {
    P_Symbol_Ptr p = lookup_Name(name);
    if (p != NULL){
      return p->type;
    }
    return -1;
  }
}

void update_arities (int arity)
{
#if 0  
  printf ("update_arity: %d\n", arity);
#endif
  {
    int i;
    for (i = 0; i < next_symbol; i++) {
      if (Symbole[i]->no_args == -1){
        Symbole[i]->no_args = arity;
      }
    }
  }
}

static int check_symbol_arity (char *name, int no_args)
{
#if 0  
  printf ("check_symbol_arity: %s %d\n", name, no_args);
#endif
  {
    P_Symbol_Ptr p = lookup_Name(name);
    if (p != NULL){
      return p->no_args == no_args;
    }
    return 0;
  }
}

void print_symbols (void)
{
  int i;
  for (i = 0; i < next_symbol; i++){
    printf ("name: %s, fsym: %d, wmsym: %d, arity: %d, weight %d, type: %s\n", 
            Symbole[i]->name, Symbole[i]->fsym, Symbole[i]->wmsym, 
            Symbole[i]->no_args, Symbole[i]->weight, 
            typenames[Symbole[i]->type]);
  }
}

void expectNo(void)
{
  noexpected = 1;
}

void expectId(void)
{
  noexpected = 0;
}

int noExpected(void)
{
  return noexpected;
}

TermT prepend(TermT t1, TermT rest)
{
  t1->Ende->Nachf = rest;
  return t1;
}

TermT mkTerm(char *name, TermT args)
{
  P_Symbol_Ptr p = lookup_Name(name);
  UTermT lauf;
  int arity = 0;
  TermzellenT *cell = malloc (sizeof(TermzellenT));

  if ((p == NULL) || (p->type == sort_sym)){
    /* error setzen .. */
    return NULL;
  }
  cell->Symbol = p->wmsym;
  cell->Nachf = args;
  lauf = cell;
  while (lauf->Nachf != NULL){
    lauf = lauf->Nachf->Ende;
    arity++;
  }
  cell->Ende = lauf;
  if (p->no_args == arity){
    return cell;
  }
  else {
    free(cell);
    adapt_error_msg(name,80);
    sprintf(error_msg, 
            "arity mismatch for symbol `%s': defined: %d, actual: %d",
            name, p->no_args, arity);
    return NULL;
  }
}

void set_ordering (ordering_type o, order chainlist)
{
  the_order.name = o;
  the_order.chain_list = chainlist;
  PA_Ordering = o;
  PA_Ordering = ordering_cons;
}

void set_weight (char *name, int val)
{
  P_Symbol_Ptr p = lookup_Name(name);
  if (p != NULL){
    p->weight = val;
  }
}

order cons_chains (ord_chain chain, order chain_list)
{
  order res  = malloc (sizeof (orderT));
  res->chain = chain;
  res->rest  = chain_list;
  return res;
}

ord_chain start_chain (char* name)
{
  ord_chain res = malloc(sizeof(ord_chainT));
  P_Symbol_Ptr p = lookup_Name(name);
  res->rest = NULL;
  res->first = p->fsym;
  return res;
}

ord_chain cons_chain (char* name, ord_chain chain)
{
  ord_chain res = malloc(sizeof(ord_chainT));
  P_Symbol_Ptr p = lookup_Name(name);
  res->rest = chain;
  res->first = p->fsym;
  return res;
}

void set_mode(mode_type m)
{
  mode = m;
}

void init_apriori(void)
{
  SV_InitApriori();
  IO_InitApriori();
  RE_InitApriori();
  VM_InitApriori();
  TP_InitApriori();
  MO_InitApriori();
  BK_InitApriori();
  BO_InitApriori();
  U1_InitApriori();
  U2_InitApriori();
  CO_ConstraintsInitialisieren();
}

static void init_aposteriori(void)
{
  SO_InitAposteriori(); 
  PR_PraezedenzVonSpecLesen();
  SG_KBOGewichteVonSpecLesen();
  TO_InitAposteriori();
  ORD_InitAposteriori();
  NF_InitAposteriori();
  BK_InitAposteriori();
}
void end_of_decls(void)
{
  if(0)printf("end_of_decls\n");
  init_aposteriori();
}

void end_of_file(void)
{
  if(0)printf("That's all, folks!\n");
}

static void print_DConsKette(DConsT d)
{
  DConsT drun = d;
  int i = 1;
  if (d == NULL){
    IO_DruckeFlex(" NIX !!!\n");
    return;
  }
  IO_DruckeFlex("%d: %C\n", i, drun->c);
  drun = drun->Nachf;
  while (drun != NULL){
    i++;
    IO_DruckeFlex("%d: %C\n", i, drun->c);
    drun = drun->Nachf;
  }
}


void constraintChecken(TermT lhs, TermT rhs, Constraint tp)
{
  EConsT    e = CO_newEcons(lhs,rhs,NULL,tp);
  ContextT  c = CO_newContext(CO_allocBindings());
  IO_DruckeFlex("\nconstraintChecken: %E %C\n", e, c);
  print_DConsKette(CO_getAllSolutions(e,c));
}

void constraintsChecken(TermT lhs, TermT rhs)
{
  return;
  constraintChecken(lhs,rhs,Gt_C);
  constraintChecken(lhs,rhs,Eq_C);
  constraintChecken(lhs,rhs,Ge_C);

  constraintChecken(rhs,lhs,Gt_C);
  constraintChecken(rhs,lhs,Eq_C);
  constraintChecken(rhs,lhs,Ge_C);
}

void add_rule(TermT lhs, TermT rhs)
{
  RegelT r;
  SelectRecT selrec = {lhs,rhs,NULL,NULL,NULL,Groesser,FALSE,FALSE,{0,{0,0,0}}};
  constraintsChecken(lhs,rhs);
  r = RE_ErzeugtesFaktum(selrec);
  if(0)IO_DruckeFlex( "new rule:            %d  %t -> %t\n", r->Nummer,TP_LinkeSeite(r), TP_RechteSeite(r));
  if (mode == acg){
    MM_untersuchen(r);
    RE_deleteActivum(r);
  }
  else {
    RE_FaktumEinfuegen(r);
    MM_untersuchen(r);
  }
}

void add_eqn(TermT lhs, TermT rhs)
{
  GleichungsT e;
  SelectRecT selrec = {lhs,rhs,NULL,NULL,NULL,Unvergleichbar,FALSE,FALSE,{0,{0,0,0}}};
  constraintsChecken(lhs,rhs);
  e = RE_ErzeugtesFaktum(selrec);
  if(0)IO_DruckeFlex( "new equation:        %d  %t -> %t\n", -e->Nummer,TP_LinkeSeite(e), TP_RechteSeite(e));
  if (mode == acg){
    MM_untersuchen(e);
    RE_deleteActivum(e);
  }
  else {
    RE_FaktumEinfuegen(e);
    MM_untersuchen(e);
  }
}

void del_rule(TermT lhs, TermT rhs)
{
  RegelT r;
  RE_forRegelnRobust(r, {
    if (TO_TermeGleich(TP_LinkeSeite(r),lhs) && TO_TermeGleich(TP_RechteSeite(r),rhs)){
      printf("remove rule:         %d\n", TP_ElternNr(r));
      RE_RegelEntfernen(r);
      return;
    }
  });
}

void del_eqn(TermT lhs, TermT rhs)
{
  GleichungsT e;
  RE_forGleichungenRobust(e, {
    if (TP_RichtungAusgezeichnet(e) &&
        ((TO_TermeGleich(TP_LinkeSeite(e),lhs) && TO_TermeGleich(TP_RechteSeite(e),rhs)) ||
	 (TO_TermeGleich(TP_LinkeSeite(e),rhs) && TO_TermeGleich(TP_RechteSeite(e),lhs))   )){
      printf("remove equation:     %d\n", -TP_ElternNr(e));
      RE_GleichungEntfernen(e);
      return;
    }
  });
}

static void my_gettimeofday(struct timeval *tvp)
{
  if (gettimeofday(tvp,NULL) == -1){
    fprintf(stderr,"problem getting time of day...\n");
    tvp->tv_sec = 0;
    tvp->tv_usec = 0;
  }
}
static void reduce_eqn(TermT lhs, TermT rhs)
{
  unsigned long diff;
  struct timeval start, stop;
  if(0)IO_DruckeFlex("Normalize %t and %t...\n", lhs, rhs);
  my_gettimeofday(&start);
  NF_NormalformRE(lhs);
  NF_NormalformRE(rhs);
  my_gettimeofday(&stop);
  if(0){
    IO_DruckeFlex("%t = %t\n", lhs, rhs);
    TO_TermeLoeschen(lhs,rhs);
  }
  else {
    if (TO_TermeGleich(lhs,rhs)){
      IO_DruckeFlex("joined into %t\n", lhs);
    }
    else {
      IO_DruckeFlex("not joined, normal forms are %t and %t\n", lhs, rhs);
    }
    diff = (stop.tv_sec - start.tv_sec)*1000000 + 
      (stop.tv_usec - start.tv_usec);
    printf("time needed: %d.%03d %03d sec\n\n", 
           diff / 1000000, (diff / 1000) % 1000, diff % 1000);
  }
}

void test_eqn(TermT lhs, TermT rhs)
{
  if (mode == termination){
    BOOLEAN res;
    int lenl, lenr;
    unsigned long diff;
    struct timeval start, stop;
    my_gettimeofday(&start);
    res = ORD_TermGroesserRed(lhs,rhs);
    my_gettimeofday(&stop);
    lenl = TO_Termlaenge(lhs);
    lenr = TO_Termlaenge(rhs);
    diff = (stop.tv_sec - start.tv_sec)*1000000 + 
      (stop.tv_usec - start.tv_usec);
    printf("%d\t%d\t%ld #%s\n", lenl, lenr, diff, res ? "True" : "False");
    return;
  }
  if (mode == reduction){
    reduce_eqn(lhs,rhs);
    return;
  }
  CO_GroundJoinable(60,lhs,rhs);
#if 1
  /* unschoener Hack (GZ_GrundzusammenfuehrbarVorwaerts) */
  BOOLEAN res;
  struct GNTermpaarTS reStruct;
  BO_TermpaarNormierenAlsKP(lhs,rhs);
  reStruct.linkeSeite = lhs;
  reStruct.rechteSeite = rhs;
  reStruct.gzfbStatus = GZ_undef;
  reStruct.GescheitertL = NULL;
  reStruct.GescheitertR = NULL;
  res = GZ_GrundzusammenfuehrbarVorwaerts(&reStruct);

  if (res){
    IO_DruckeFlex("GRUNDZUSAMMENFUEHRBAR:       %t = %t\n", lhs, rhs);
  }
  else {
    IO_DruckeFlex("NICHT grundzusammenfuehrbar: %t = %t\n", lhs, rhs);
  }
#else
  IO_DruckeFlex("%t = %t", lhs, rhs);
  NF_NormalformRE(lhs);
  NF_NormalformRE(rhs);
  if (TO_TermeGleich(lhs,rhs)){
    IO_DruckeFlex(" joinable into %t\n", lhs);
  }
  else {
    IO_DruckeFlex(" NOT joinable; normal forms are %t = %t\n", lhs, rhs);
  }
  TO_TermeLoeschen(lhs,rhs);
#endif
}
void handle_equation(eqmode_type md, TermT lhs, TermT rhs)
{
#if 0
  IO_DruckeFlex("handle_equation: %t %t\n", lhs, rhs);
  ORD_VergleicheTerme(lhs,rhs);
#elif 0
  switch (ORD_VergleicheTerme(lhs,rhs)){
  case Groesser:       IO_DruckeFlex("%t > %t\n", lhs, rhs); break;
  case Kleiner:        IO_DruckeFlex("%t < %t\n", lhs, rhs); break;
  case Gleich:         IO_DruckeFlex("%t = %t\n", lhs, rhs); break;
  case Unvergleichbar: IO_DruckeFlex("%t # %t\n", lhs, rhs); break;
  }
  /*  TO_TermeFreigeben(lhs,rhs);*/
#else
  switch (md){
  case add_mode:
  case eqn_mode:
    if (0&&(mode == reduction)) {
      add_rule(lhs, rhs); break;
    }
    switch (ORD_VergleicheTerme(lhs,rhs)){
    case Groesser:       add_rule(lhs, rhs); break;
    case Kleiner:        add_rule(rhs, lhs); break;
    case Gleich:         IO_DruckeFlex("HUCH: %t = %t\n", lhs, rhs); break;
    case Unvergleichbar: add_eqn(lhs, rhs); break;
    }
    break;
  case del_mode:
    switch (ORD_VergleicheTerme(lhs,rhs)){
    case Groesser:       del_rule(lhs, rhs); break;
    case Kleiner:        del_rule(rhs, lhs); break;
    case Gleich:         IO_DruckeFlex("HUCH: %t = %t\n", lhs, rhs); break;
    case Unvergleichbar: del_eqn(lhs, rhs); break;
    }
    break;
  case conc_mode:
  case test_mode:
    test_eqn(lhs, rhs); 
    return;
  default:
    our_error("unknown mode!");
  }
#if 0
  RE_AusgabeRegelmenge(1);
  RE_AusgabeGleichungsmenge(1);
#endif
#endif
}

/* ************************************************************************** */
/*                                                                            */
/* alte Parser-Schnittstelle emulieren ;-|                                    */
/*                                                                            */
/* ************************************************************************** */

static P_Symbol_Ptr lookup_fs (func_symb s)
{
  int i;
  for (i = 0; i < next_symbol; i++){
    if(Symbole[i]->type == fun_sym &&
       Symbole[i]->fsym == s){
      return Symbole[i];
    }
  }
  our_error("internal error in lookup_fs!");
  return NULL;
}

int            sig_arity      (func_symb x)     {return lookup_fs(x)->no_args;}
func_symb_list get_sig        (void )           {return nextFSym==1?0:1;}
func_symb      sig_first      (func_symb_list x){return x;}
func_symb_list sig_rest       (func_symb_list x){return x+1==nextFSym?0:x+1;}
orderdec       get_ordering   (void)            {return &the_order;}
ordering_type  ord_name       (orderdec x)      {return x->name;}
order          ord_chain_list (orderdec x)      {return x->chain_list;}
ord_chain      ord_first      (order x)         {return x->chain;}
order          ord_rest       (order x)         {return x->rest;}
func_symb      ord_chain_first(ord_chain x)     {return x->first;}
ord_chain      ord_chain_rest (ord_chain x)     {return x->rest;}
weight         ord_func_weight(func_symb x)     {return lookup_fs(x)->weight;}
char*          map_func       (func_symb x)     {return lookup_fs(x)->name;}

/* ************************************************************************** */
/*                                                                            */
/* EOF                                                                        */
/*                                                                            */
/* ************************************************************************** */

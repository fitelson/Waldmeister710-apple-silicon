#if COMP_DIST

#include "general.h"
#include "Ausgaben.h"
#include "DCommunicator.h"
#include "Dispatcher.h"
#include "Hauptkomponenten.h"
#include "KDHeapElements.h"
#include "Stringterms.h"

#define OLD_DISPATCHER 0

BOOLEAN MASTER_RECHNET_MIT = TRUE;
BOOLEAN SLAVE_VERBOSE = FALSE;
BOOLEAN MASTER_VERBOSE = FALSE;
extern int     HK_noOfSlaves;


static unsigned no_slaves = 3;
static unsigned start_time = 50;
static unsigned no_open_jobs;
static unsigned job_buf_size;
static int next_slave;

typedef enum {
  DIS_State1, DIS_State2, DIS_State3, DIS_State4, DIS_State5, DIS_State6
} DIS_StateT;

static  DIS_StateT state = DIS_State1;
/* Statemachine:
   State1  --Add--> State2
   State2  --Del--> State3
   State3  --Del--> State3
   State2  --GJ --> State4  write 0 Entry
   State3  --GJ --> State4  write 0 Entry
   State4  --GJ --> State4
   State2  --Flush--> State1 write 0 Entry twice
   State3  --Flush--> State1 write 0 Entry twice
   State4  --Flush--> State1 write 0 Entry
   State1  --End--> State5
   State2  --End--> State5
   State3  --End--> State5
   State4  --End--> State5

   State1  --GetA-> State6
   State6  --GetI-> State1 -- if empty
   State6  --GetI-> State6 -- if not empty
   State6  --End--> State5

   other transitions: error
*/

/* ---------------------------------------------------------------------------------- */
/* Ringpuffer Slaves                                                                  */
/* ---------------------------------------------------------------------------------- */

static int *SlaveRB;
static unsigned int SRBsize;
static unsigned int SRBnextSlave;
static unsigned int SRBnextFree;
static unsigned int SRBfill;

static unsigned SRBnextIndex(unsigned i)
{
  i++;
  if (i == SRBsize){
    i = 0;
  }
  return i;
}

static BOOLEAN SRBslaveAvail(void)
{
  if (OLD_DISPATCHER){
    return TRUE;
  }
  else {
    return SRBfill > 0;
  }
}

static int calcNextSlave(unsigned int i)
{
  i++;
  if (i == no_slaves){
    i = 0;
  }
  return i;
}

static int SRBgetNextSlave(void)
{
  int res;
  if (OLD_DISPATCHER){
    res = next_slave;
    next_slave = calcNextSlave(next_slave);
  }
  else {
    if (SRBfill == 0){
      our_error("SRBgetNextSlave on empty SlaveRB");
    }
    res = SlaveRB[SRBnextSlave];
    SRBfill--;
    SRBnextSlave = SRBnextIndex(SRBnextSlave);
  }
  return res;
}

static void SRBputFreeSlave(int slave)
{
  if (OLD_DISPATCHER){
    /* nothing */
  }
  else {
    if (SRBfill == SRBsize){
      our_error("SRBputFreeSlave on full SlaveRB");
    }
    SlaveRB[SRBnextFree] = slave;
    SRBnextFree = SRBnextIndex(SRBnextFree);
    SRBfill++;
  }
}

static void SRBinitSlaveRB(unsigned noOfSlaves)
{
  if (OLD_DISPATCHER){
    fprintf(stderr,"OLD DISPATCHER\n");
    next_slave = 0;
  }
  else {
    unsigned i;
    fprintf(stderr,"NEW DISPATCHER\n");
    SRBsize = noOfSlaves;
    SlaveRB = our_alloc(SRBsize * sizeof(*SlaveRB));
    SRBnextFree = 0;
    SRBnextSlave = 0;
    SRBfill = 0;
    for (i = 0; i < noOfSlaves; i++){
      SRBputFreeSlave(i);
    }
  }
}

/* ---------------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------------- */

typedef enum {
  DIS_JStateI, DIS_JStateW, DIS_JStateS, DIS_JStateB, DIS_JStateR, DIS_JStateA, 
  DIS_JStateG, DIS_JStateD, DIS_JStateX, DIS_JStateY
} DIS_JStateT;

/* Statemachine for each job:
 *
 *	 ---------------------------
 *      /        		    \
 *     /   /---D 		     \
 *    /   /    ^ ^		      \
 *    |  /     |  \		       \
 *    | /      |  |			\
 *    vv       |  |			 \
 *    I -----> W -----> S -----> B -----> R
 *    |\       |  |
 *    | \      | /|
 *    \  \     V/ |
 *     \  ---> G  |
 *      \      ^  |
 *	 \     | /
 *        \    |/
 *	   --> A
 *
 *  etc. ..
 *  I == initialer Zustand
 *  A == ist ACG
 *  G == ist GZFB
 *  D == Job geloescht
 *  W == Job wartet
 *  S == Job verschickt
 *  B == Job wird zwischengepuffert
 *  R == Job wird ausgelesen
 * X,Y == Zwischenzustaende, wenn Job verschickt, danach geloescht/gzfb
 */

typedef struct {
  long w1, w2;
  unsigned long t1, t2;
  long xp;
} sendEntryT;

typedef struct {
  DIS_JStateT state;
  AbstractTime time;
  int slave;              /* -1 unassigned, otherwise 0 <= slave < DCM_GetNoOfSlaves() */
  ElementT aEntry;
  sendEntryT *iEntries;   /* read and decoded i-Entries */
  unsigned int inpLength; /* no. of i-entries in buffer that are not yet delivered */
  unsigned int curIEntry;
} dis_JobT;

static dis_JobT *dis_Jobs;
static unsigned ms_left;
static unsigned ms_right;
static unsigned ms_fill;
static unsigned next_unassigned;

static void clearJob(unsigned int index)
{
  dis_Jobs[index].state     = DIS_JStateI;
  dis_Jobs[index].time      = 0;
  dis_Jobs[index].slave     = -1;
  /*  dis_Jobs[index].aEntry  = ...;*/
  dis_Jobs[index].iEntries  = NULL;
  dis_Jobs[index].inpLength = 0;
  dis_Jobs[index].curIEntry = 0;
}

static void initJobs(void)
{
  unsigned i;
  for (i = 0; i < job_buf_size; i++){
    clearJob(i);
  }
  ms_left = 0;
  ms_right = 0;
  ms_fill = 0;
  next_unassigned = 0;
}

static unsigned nextIndex(unsigned i)
{
  i++;
  if (i == job_buf_size){
    i = 0;
  }
  return i;
}

static BOOLEAN dis_IsInJobBuffer(RegelOderGleichungsT opfer)
{
  return (ms_fill > 0) && (TP_getGeburtsTag_M(opfer) >= dis_Jobs[ms_left].time);
}

static unsigned int lookUpIndex(AbstractTime i)
{
  unsigned index = ms_left;
  while ((index != ms_right)&&(i != dis_Jobs[index].time)){
    index = nextIndex(index);
  }
  return index;
}

static void delJob(AbstractTime time)
{
  unsigned i = lookUpIndex(time);
  if (i == ms_right){
    our_error("delJob: Job is lost");
  }
  if (MASTER_VERBOSE){
    printf("delJob: job state is %d time is %ld\n", dis_Jobs[i].state, dis_Jobs[i].time);
  }
  switch (dis_Jobs[i].state) {
  case DIS_JStateA: 
  case DIS_JStateG: 
  case DIS_JStateW: dis_Jobs[i].state = DIS_JStateD; break;
  case DIS_JStateS: dis_Jobs[i].state = DIS_JStateX; break;
  case DIS_JStateB: dis_Jobs[i].state = DIS_JStateD; /* Array fuer ientries verwerfen XXX */ break;
  default:          our_error("delJob: job in wrong state");
  }
}

static void gjJob(AbstractTime time)
{
  unsigned i = lookUpIndex(time);
  if (i == ms_right){
    our_error("gjJob: Job is lost");
  }
  if (MASTER_VERBOSE){
    printf("gjJob: job state is %d\n", dis_Jobs[i].state);
  }
  switch (dis_Jobs[i].state) {
  case DIS_JStateW: dis_Jobs[i].state = DIS_JStateG; break;
  case DIS_JStateS: dis_Jobs[i].state = DIS_JStateY; break;
  case DIS_JStateB: dis_Jobs[i].state = DIS_JStateG; /* Array fuer ientries verwerfen XXX */ break;
  default:          our_error("gjJob: job in wrong state");
  }
  dis_Jobs[i].aEntry.w1  = maximalWeight();
  dis_Jobs[i].aEntry.w2  = maximalWeight();
}

static char *basename(char *fname)
{
  char *rloc = strrchr(fname, '/');
  if (rloc != NULL){
    return rloc+1;
  }
  return fname;
}

static void whoIsWho(void)
{
  int i;
  HK_noOfSlaves = no_slaves+1;
  for (i = 0; i < no_slaves; i++){
    if (MASTER_RECHNET_MIT){
      DCM_SendWhoIsWho(no_slaves+1,i);
    }
    else {
      DCM_SendWhoIsWho(no_slaves,i);
    }
  }
}

void KPVGetWeightLimit(ElementT *limit);

static void sendLimit(void)
{
  ElementT limit;
  if (no_slaves > 0){
    KPVGetWeightLimit(&limit);
    DCM_SendLong (limit.w1);
    DCM_SendLong (limit.w2);
    DCM_SendULong(limit.t1);
    DCM_SendULong(limit.t2);
    DCM_SendLong (limit.xp);
    DCM_SendULong(limit.t2_);
    DCM_SendLong (limit.xp_);
  }
}

static void sendExpandMsg(unsigned index, int slave)
{
  if ((no_slaves == 0)||(HK_AbsTime < start_time)){
    return;
  }
  DCM_SendExpand(dis_Jobs[index].time, slave);
  dis_Jobs[index].slave = slave;
  dis_Jobs[index].state = DIS_JStateS;
}

static void sendJobsIfPossible(void)
{
  while (next_unassigned != ms_right){
    switch (dis_Jobs[next_unassigned].state){
    case DIS_JStateD:
    case DIS_JStateA:
    case DIS_JStateG:
      break;
    case DIS_JStateW:
      if ((no_slaves == 0)||(HK_AbsTime < start_time)){
	break;
      }
      if (!SRBslaveAvail()){
        return;
      }
      sendExpandMsg(next_unassigned, SRBgetNextSlave());
      break;
    default:
      printf("dis_Jobs[next_unassigned].state %d\n", dis_Jobs[next_unassigned].state);
      our_error("sendJobsIfPossible: unexpected state in job");
    }
    next_unassigned = nextIndex(next_unassigned);
  }
}

static void recvAEntry(ElementT *yp)
{
  yp->w1  = DCM_RecvLong();
  yp->w2  = DCM_RecvLong();
  yp->t1  = DCM_RecvULong();
  yp->t2  = DCM_RecvULong();
  yp->xp  = DCM_RecvLong();
  yp->t2_ = DCM_RecvULong();
  yp->xp_ = DCM_RecvLong();
  if (MASTER_VERBOSE){
    printf("a-entry: w1 = %ld, w2 = %ld, t1 = %lu, t2 = %lu, xp = %ld, t2_ = %lu, xp_ = %ld\n",
           yp->w1, yp->w2, yp->t1, yp->t2, yp->xp, yp->t2_, yp->xp_);
  }
}

static void recvIEntries(unsigned int length, sendEntryT *ie)
{
  while (length > 0){
    ie->w1  = DCM_RecvLong();
    ie->w2  = DCM_RecvLong();
    ie->t1  = DCM_RecvULong();
    ie->t2  = DCM_RecvULong();
    ie->xp  = DCM_RecvLong();
    ie++;
    length--;
  }
}

static void recvEntries(unsigned index)
{
  unsigned inpLength;
  recvAEntry(&(dis_Jobs[index].aEntry));
  inpLength = DCM_RecvLong();
  dis_Jobs[index].inpLength = inpLength;
  dis_Jobs[index].iEntries = our_alloc(inpLength*sizeof(sendEntryT));
  dis_Jobs[index].curIEntry = 0;
  recvIEntries(inpLength, dis_Jobs[index].iEntries);
}

static unsigned int recvAMsg(void)
{
  AbstractTime time;
  unsigned index, inpLength;

  time = DCM_RecvLong();
  if (MASTER_VERBOSE){
    printf("received time %ld\n", time);
  }
  index = lookUpIndex(time);
  if (index == ms_right){
    our_error("recvAMsg: job is lost");
  }
  if (dis_Jobs[index].state == DIS_JStateY){
    printf("discard message: already gj'd\n");
    dis_Jobs[index].state = DIS_JStateG;
  }
  else if (dis_Jobs[index].state == DIS_JStateX){
    printf("discard message: already deleted\n");
    dis_Jobs[index].state = DIS_JStateD;
  }
  else {
    if (dis_Jobs[index].state != DIS_JStateS){
      our_error("recvAMsg: job in wrong state");
    }
    recvEntries(index);
    dis_Jobs[index].state = DIS_JStateB;
  }
  SRBputFreeSlave(dis_Jobs[index].slave);
  return index;
}

static void recvASet(unsigned index)
{
  unsigned index2;

  for (;;){
    if (DCM_NextMsg() != DCM_RetMsg){
      our_error("RetMsg expected!");
    }
    index2 = recvAMsg();
    sendJobsIfPossible();
    if (index == index2){
      return;
    }
    else { /* Achtung vor Endlosschleife! */
      if (MASTER_VERBOSE){
        printf("expected time %ld\n", dis_Jobs[index].time);
      }
      /*      our_error("wrong time returned");*/
    }
  }
}

/* -------------------------------------------------------------------------------- */
/* --- Schnittstelle nach aussen -------------------------------------------------- */
/* -------------------------------------------------------------------------------- */

void DIS_SetStartTime(unsigned int start)
{
  printf("Starttime = %u\n", start);
  start_time = start;
}
void DIS_SetNoOfJobs(unsigned int jobs)
{
  printf("Jobs = %u\n", jobs);
  if (jobs == 0){
    jobs = 1;
    printf("Jobs changed to 1\n");
  }
  no_open_jobs = jobs;
  job_buf_size = jobs + 2;
  /* must be greater than no_open_jobs otherwise ms_right will not show on empty entry & not possible to
     use ms_right as not_found value in lookUpIndex */
  dis_Jobs = our_alloc(job_buf_size * sizeof(*dis_Jobs));
}
void DIS_SetNoOfSlaves(unsigned int slaves)
{
  printf("Slaves = %u\n", slaves);
  no_slaves = slaves;
}
unsigned int DIS_GetNoOfSlaves(void)
{
  return no_slaves;
}

void DIS_Start(int argc, char **argv)
/* argl: our_strdups notwendig wg. pvm! */
{
  int i,j;
  char **newargv;
  initJobs();
  if (no_slaves == 0){
    return;
  }
  IO_DruckeFlex("DIS_Start\n");
  newargv = our_alloc((argc+3)*sizeof(char*));
  newargv[0] = our_strdup(basename(argv[0]));
  j = 1;
  for (i = 1; i < argc; i++){
    if (argv[i] != NULL){
      newargv[j] = our_strdup(argv[i]);
      j++;
    }
  }
  newargv[j] = our_strdup(IO_Spezifikationsname());
  j++;
  newargv[j] = our_strdup("-slave");
  j++;
  newargv[j] = NULL;
  if (!DCM_Start(no_slaves,newargv)){
    unsigned actual_slaves = DCM_GetNoOfSlaves();
    printf("##################################################\n"
           "##################################################\n"
           "## Got only %3d slaves!  Wanted %3d.            ##\n"
           "##################################################\n"
           "##################################################\n", actual_slaves, no_slaves);
    our_error("not enough slaves");
  }
  whoIsWho();
  SRBinitSlaveRB(no_slaves);
}

void DIS_End(void)
{
  if (state == DIS_State5){
    our_error("DIS_End called twice!");
  }
  state = DIS_State5;
  if (no_slaves > 0){
    DCM_SendEnd();
  }
}

void DIS_AddActive(RegelOderGleichungsT active)
{
  if (state == DIS_State1){
    if (no_slaves > 0){
      int len;
      PTermpaarT tp = STT_TermpaarEinpacken(active->linkeSeite, active->rechteSeite, &len);
      int flags =
        ((RE_IstRegel(active))           	      ?   1 : 0) +
        ((active->r2 != NULL)            	      ?   2 : 0) +
        ((active->IstMonogleichung)      	      ?   4 : 0) +
        ((active->DarfNichtUeberlappen)  	      ?   8 : 0) +
        ((active->Grundzusammenfuehrbar) 	      ?  16 : 0) +
        ((active->ACGrundreduzibel)      	      ?  32 : 0) +
        ((active->StrengKonsistent)      	      ?  64 : 0) +
        ((RE_IstGleichung(active) &&
          TP_Gegenrichtung(active)->r2 != NULL) ? 128 : 0) ;
      DCM_StartActive();
      DCM_SendInt(flags);
      DCM_SendInt(active->generation);
      DCM_SendBytes(len,tp);
      our_dealloc(tp);
    }
    state = DIS_State2;
  }
  else {
    our_error("in wrong state for DIS_AddActive");
  }
}

void DIS_DelActive(RegelOderGleichungsT active)
{
  if ((state == DIS_State2)||(state == DIS_State3)){
    if (no_slaves > 0){
      DCM_SendULong(TP_getGeburtsTag_M(active));
    }
    if (dis_IsInJobBuffer(active)){
      delJob(TP_getGeburtsTag_M(active));
    }
    state = DIS_State3;
  }
  else {
    our_error("in wrong state for DIS_DelActive");
  }
}

void DIS_GJActive(RegelOderGleichungsT active)
{
  if ((state == DIS_State2)||(state == DIS_State3)){
    if (no_slaves > 0){
      DCM_SendULong(0);
    }
    state = DIS_State4;
  }
  else if (state != DIS_State4){
    our_error("in wrong state for DIS_GJActive");
  }
  if (dis_IsInJobBuffer(active)){
    gjJob(TP_getGeburtsTag_M(active));
  }
  if (no_slaves > 0){
    DCM_SendULong(TP_getGeburtsTag_M(active));
  }
}

void DIS_Flush(void)
{
  if ((state == DIS_State2)||(state == DIS_State3)){
    if (no_slaves > 0){
      DCM_SendULong(0);
      DCM_SendULong(0);
    }
  }
  else if (state == DIS_State4){
    if (no_slaves > 0){
      DCM_SendULong(0);
    }
  }
  else {
    our_error("in wrong state for DIS_Flush");
  }

  sendLimit();
  if (no_slaves > 0){
    DCM_SendActive();
  }
  state = DIS_State1;
}

void DIS_InsertJob(AbstractTime i)
{
  RegelOderGleichungsT active;

  if (state != DIS_State1){
    our_error("Wrong state for DIS_InsertJob");
  }

  ms_fill++;
  if (ms_fill > no_open_jobs){
    our_error("Buffer overrun in DIS_InsertJob");
  }

  if (dis_Jobs[ms_right].state != DIS_JStateI){
    our_error("error in state in job buffer");
  }
  active = RE_getActiveWithBirthday(i);
  dis_Jobs[ms_right].time = i;
  dis_Jobs[ms_right].aEntry.t1  = i;
  dis_Jobs[ms_right].aEntry.t2  = i;  /* kein IROpfer */
  dis_Jobs[ms_right].aEntry.xp  = i;  /* kein IROpfer */
  dis_Jobs[ms_right].aEntry.t2_ = 0;
  dis_Jobs[ms_right].aEntry.xp_ = 0;
  dis_Jobs[ms_right].aEntry.observer = &(active->aEntryObserver);

  if (active->DarfNichtUeberlappen){
    dis_Jobs[ms_right].state = DIS_JStateG;
    dis_Jobs[ms_right].aEntry.w1  = maximalWeight();
    dis_Jobs[ms_right].aEntry.w2  = maximalWeight();
  }
  else if (active->ACGrundreduzibel){
    dis_Jobs[ms_right].state = DIS_JStateA;
    dis_Jobs[ms_right].state = DIS_JStateW;
    dis_Jobs[ms_right].aEntry.w1  = minimalWeight();
    dis_Jobs[ms_right].aEntry.w2  = minimalWeight();
  }
  else {
    dis_Jobs[ms_right].state = DIS_JStateW;
    dis_Jobs[ms_right].aEntry.w1  = minimalWeight();
    dis_Jobs[ms_right].aEntry.w2  = minimalWeight();
  }

  ms_right = nextIndex(ms_right);
  sendJobsIfPossible();
}

extern BOOLEAN HK_modulo;

BOOLEAN DIS_RetrieveAEntry(ElementT *xp, BOOLEAN urgent)
{
  if (state != DIS_State1){
    our_error("DIS_RetrieveAEntry: in wrong state");
  }
  if (HK_modulo){
    AbstractTime time;
    if (DCM_NextMsg() != DCM_RetMsg){
      our_error("RetMsg expected!");
    }
    time = DCM_RecvLong();
    if (MASTER_VERBOSE){
      printf("RetMsg for date %ld received!\n", time);
    }
    recvEntries(ms_left);
  }
  else {
  again:
    if (!((ms_fill > 0) &&
          (urgent ||
           (HK_AbsTime < start_time) ||
           (ms_fill == no_open_jobs)   ))){
      return FALSE;
    }
    switch (dis_Jobs[ms_left].state){
    case DIS_JStateA:
    case DIS_JStateG:
    case DIS_JStateB:
    case DIS_JStateW:
      /* nothing to do */
      break;
    case DIS_JStateX:
    case DIS_JStateY:
    case DIS_JStateS:
      recvASet(ms_left); /* wait for return message */
      goto again;
    case DIS_JStateD:
      clearJob(ms_left);
      ms_left = nextIndex(ms_left);
      ms_fill--;
      goto again;
    default:
      our_error("DIS_RetrieveAEntry: job in wrong state");
    }
  }

  *xp = dis_Jobs[ms_left].aEntry;
  dis_Jobs[ms_left].state = DIS_JStateR;
  state = DIS_State6;
  return TRUE;
}

BOOLEAN DIS_RetrieveIEntry(ElementT *xp)
{
  if (state != DIS_State6){
    our_error("DIS_RetrieveIEntry: in wrong state");
  }
  if (dis_Jobs[ms_left].state != DIS_JStateR){
    our_error("DIS_RetrieveIEntry: job in wrong state");
  }
  if (dis_Jobs[ms_left].inpLength == dis_Jobs[ms_left].curIEntry){
    our_dealloc(dis_Jobs[ms_left].iEntries);
    clearJob(ms_left);

    if (!HK_modulo){
      ms_left = nextIndex(ms_left);
      ms_fill--;
    }
    state = DIS_State1;

    return FALSE;
  }
  else {
    sendEntryT *ie = &(dis_Jobs[ms_left].iEntries[dis_Jobs[ms_left].curIEntry]);

    xp->w1  = ie->w1;
    xp->w2  = ie->w2;
    xp->t1  = ie->t1;
    xp->t2  = ie->t2;
    xp->xp  = ie->xp;
    xp->t2_ = ie->t2;
    xp->xp_ = ie->xp;
    xp->observer = NULL;
    dis_Jobs[ms_left].curIEntry++;
  }
  return TRUE;
}

#else

;

#endif

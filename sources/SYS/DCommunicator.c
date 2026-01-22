#include <stdlib.h>
#include "Ausgaben.h"
#include "FehlerBehandlung.h"
#include "general.h"
#include "SpeicherVerwaltung.h"
#include "DCommunicator.h"


#if !COMP_DIST

BOOLEAN DCM_Start (int nslaves_, char**argv){ return FALSE; }
int DCM_GetNoOfSlaves(void){ return 0; }
void DCM_SendInt(int i){}
int  DCM_RecvInt(void){ return 0; }
void DCM_SendBytes(int len, byte *tp){}
byte *DCM_RecvBytes(void){ return NULL; }
void DCM_SendLong(long i){}
long DCM_RecvLong(void){ return 0; }
void DCM_SendULong(unsigned long i){}
unsigned long DCM_RecvULong(void){ return 0; }
void DCM_StartActive(void){}
void DCM_SendActive(void){}
void DCM_SendEnd(void){}
DCM_MsgT DCM_NextMsg(void) { return DCM_EndMsg; }
void DCM_SendExpand(unsigned long t,int slave){}
void DCM_SendWhoIsWho(int no, int slave){}
void DCM_StartReturn(void){}
void DCM_SendReturn(void){}
void DCM_SlaveStart(void){}

#else
#include <pvm3.h>

/* Invariante: es ist immer ein frischer outbuffer vorhanden */

static int mytid;
static int parenttid;
static int *slaves;
static int nslaves;
static int outbuf;
static int inbuf;

#define MSG_CHAN 1

static BOOLEAN newOutBuffer(BOOLEAN fatal)
{
  outbuf = pvm_initsend(PvmDataRaw);
  if (outbuf < 0){
    if (fatal){
      char msg[100];
      snprintf(msg, 100, "Error allocating new out buffer. Error code: %d\n", outbuf);
      our_error(msg);
    }
    else {
      fprintf(stderr, "Cannot get out buffer. Error code %d\n", outbuf);
      return FALSE;
    }
  }
  return TRUE;
}  

static void checkInfo (int i, int info)
{
  if (info < 0){
    char msg[100];
    snprintf(msg, 100, "Error in packaging data (%d). Error code: %d\n", i, info);
    our_error(msg);
  }
}

static void mcastOutBuffer(void)
{
  int info = pvm_mcast(slaves, nslaves, MSG_CHAN);
  if (info < 0){
    our_error("Error in multicast!\n");
  }
}

static void disconnect(void)
{
  pvm_exit();
}

BOOLEAN DCM_Start (int nslaves_, char**argv)
{
  mytid = pvm_mytid();
  if (mytid < 0){
    fprintf(stderr, "Error with PVM: %d\n",mytid);
    return FALSE;
  }
  atexit(disconnect);
  if(1){/* knallt ;-((( */
    int info = pvm_catchout (stdout);
  }

  slaves = our_alloc(nslaves_ * sizeof(int));
  nslaves = pvm_spawn(argv[0], argv+1, PvmTaskDefault, NULL, nslaves_, slaves);
  if (nslaves < 0){
    fprintf(stderr, "System error spawning slaves: %d\n", nslaves);
    nslaves = 0;
    return FALSE;
  }
  else if (nslaves < nslaves_){
    fprintf(stderr, "Cannot allocate enough slaves: wanted %d, allocated: %d\n", nslaves_, nslaves);
    return FALSE;
  }
  fprintf(stdout, "got all slaves!\n");

  if (!newOutBuffer(FALSE)) {
    return FALSE;
  }
  return TRUE;
}

int DCM_GetNoOfSlaves(void)
{
  return nslaves;
}

void DCM_SendInt(int i)
{
  checkInfo(1,pvm_pkint(&i, 1, 1));
}

int  DCM_RecvInt(void)
{
  int res;
  checkInfo(2,pvm_upkint(&res, 1, 1));
  return res;
}

void DCM_SendBytes(int len, byte *tp)
{
  checkInfo(3,pvm_pkint(&len, 1, 1));
  checkInfo(4,pvm_pkbyte(tp, len, 1));
}

byte *DCM_RecvBytes(void)
{
  int len;
  byte *res;
  checkInfo(5,pvm_upkint(&len, 1, 1));
  res = our_alloc(len);
  checkInfo(6,pvm_upkbyte(res, len, 1));
  return res;
}

void DCM_SendLong(long i)
{
  checkInfo(7,pvm_pklong(&(i), 1, 1));
}

long DCM_RecvLong(void)
{
  long l;
  checkInfo(8,pvm_upklong(&l, 1, 1));
  return l;
}

void DCM_SendULong(unsigned long i)
{
  checkInfo(7,pvm_pkulong(&(i), 1, 1));
}

unsigned long DCM_RecvULong(void)
{
  unsigned long l;
  checkInfo(8,pvm_upkulong(&l, 1, 1));
  return l;
}

void DCM_StartActive(void)
{
  DCM_SendInt(DCM_AddMsg);
}

void DCM_SendActive(void)
{
  mcastOutBuffer();
  newOutBuffer(TRUE);
}

void DCM_SendEnd(void)
{
  newOutBuffer(TRUE); /* discard buffer filled so far */
  DCM_SendInt(DCM_EndMsg);
  mcastOutBuffer();
  newOutBuffer(TRUE); /* only for invariance, not really nec. */
}

DCM_MsgT DCM_NextMsg(void) 
{
  int bufid;
  int inpMsgTp;
  bufid = pvm_recv(-1, MSG_CHAN); /* blocking ! */
  checkInfo(9,pvm_upkint(&inpMsgTp, 1, 1));
  return inpMsgTp;
}

void DCM_SendExpand(unsigned long t, int slave)
{
  if (!((0 <= slave) && (slave < nslaves))){
    our_error("DCM_SendExpand: slave out of range");
  }
  else {
    int info;
    DCM_SendInt(DCM_ExpMsg);
    DCM_SendULong(t);
    info = pvm_send(slaves[slave], MSG_CHAN);
    if (info < 0){
      our_error("Error in sending message!\n");
    }
  }
  newOutBuffer(TRUE);
}

void DCM_SendWhoIsWho(int no, int slave)
{
  if (!((0 <= slave) && (slave < nslaves))){
    our_error("DCM_SendWhoIsWho: slave out of range");
  }
  else {
    int info;
    DCM_SendInt(DCM_WiWMsg);
    DCM_SendInt(no);
    DCM_SendInt(slave);
    info = pvm_send(slaves[slave], MSG_CHAN);
    if (info < 0){
      our_error("Error in sending message!\n");
    }
  }
  newOutBuffer(TRUE);
}

void DCM_StartReturn(void)
{
  DCM_SendInt(DCM_RetMsg);
}

void DCM_SendReturn(void)
{
  int info = pvm_send(parenttid, MSG_CHAN);
  if (info < 0){
    our_error("Error in sending message to parent!\n");
  }
  newOutBuffer(TRUE);
}

void DCM_SlaveStart(void)
{
  mytid = pvm_mytid();
  parenttid = pvm_parent();
  atexit(disconnect);
  if (parenttid == PvmNoParent){
    our_error("Called as slave, but not via PVM!");
  }
  printf("I am the slave with number %x. My parent is %x. ", mytid, parenttid);
  {
    char hostname[1000];
    int res = gethostname(hostname,1000);
    if (res == 0){
      printf("I am running on %s.\n", hostname);
    }
    else {
      printf("I can not determine where I am running.\n");
    }
  }
  newOutBuffer(TRUE);
}

#endif

/* ------------------------------------------------*/
/* -------VADDER / SOHN Communication--------------*/
/* ------------------------------------------------*/
#include "Communication.h"
#include "Parameter.h"

#define COM_USE_COM 1 /* 0, 1, 2 */

#define COM_PRINT_THINGS (!COMP_COMP)

BOOLEAN COM_Vadder = FALSE;
BOOLEAN COM_forked = FALSE;
char*   COM_Prefix = "";


#if COM_USE_COM == 0
/* only stubs, no communication */

void       COM_init(void)        {}
BOOLEAN    COM_fork(void)        {return FALSE;}
void       COM_free(void)        {}
COM_StateT COM_getState(void)    {return COM_state_unset;}
void       COM_markSuccess(void) {}
void       COM_kill(void)        {}
void       COM_wait(void)        {}
void       COM_wakeOther(void)   {}
void       COM_sleep(void)       {}
AbstractTime COM_GlobalTime(void){return minimalAbstractTime();}
void COM_stopComputing(void)     {}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#elif COM_USE_COM == 1
/* use SysV Semaphores */

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#if (defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)) || defined(__APPLE__)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
  int val;                  /* value for SETVAL */
  struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
  unsigned short *array;    /* array for GETALL, SETALL */
  /* Linux specific part: */
  struct seminfo *__buf;    /* buffer for IPC_INFO */
};
#endif

#include "Ausgaben.h"
#include "Hauptkomponenten.h"

#define NO_OF_TRIES 3

static BOOLEAN com_communicationPossible = FALSE;

static pid_t com_son_pid;

typedef struct com_syncData {
  short state;             /* written by both -> needs lock */
  AbstractTime VadderTime; /* only written by Vadder */
  AbstractTime SohnyTime;  /* only written by Sohny */
  BOOLEAN VadderRechnetNoch;
  BOOLEAN SohnyRechnetNoch;
} *com_syncDataT;

static com_syncDataT com_sharedState; 

static int com_semid;
static BOOLEAN com_sem_allocated = FALSE;

static struct sembuf com_sembuf_up;
static struct sembuf com_sembuf_down;

/* wrapper to mmap, stolen from Stevens, UNPv2 */
static void *my_shm(size_t nbytes)
{
  void	*shared;

#if defined(MAP_ANON)
  shared = mmap(NULL, nbytes, PROT_READ | PROT_WRITE,
                MAP_ANON | MAP_SHARED, -1, 0);

#elif defined(HAVE_DEV_ZERO) || defined (SUNOS5)
  int fd;

  if ( (fd = open("/dev/zero", O_RDWR)) == -1)
    return(MAP_FAILED);
  shared = mmap(NULL, nbytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

# else
# error cannot determine what type of anonymous shared memory to use
# endif
  return shared;		/* MAP_FAILED on error */
}
/* end my_shm */

void COM_init(void) 
{
  if (!PA_ForkErlaubt()){
    return;
  }

  com_semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0600);
  if (com_semid == -1){
    our_warning("cannot allocate semaphores");
    return;
  }
  com_sem_allocated = TRUE;
  atexit(COM_free);
  {
    unsigned short argarray[3] = {1,0,0};
    union semun arg;
    int res;
    arg.array = argarray;
    res = semctl(com_semid, 0, SETALL, arg);
    if (res == -1){
      our_warning("cannot initialize semaphores");
      return;
    }
  }
  com_sharedState = my_shm(sizeof(*com_sharedState));
  if (com_sharedState == (com_syncDataT)-1){
    our_warning("no shared data\n");
    return;
  }

  com_sharedState->state = COM_state_unset;
  com_sharedState->VadderTime = minimalAbstractTime();
  com_sharedState->SohnyTime = minimalAbstractTime();
  com_sharedState->VadderRechnetNoch = TRUE;
  com_sharedState->SohnyRechnetNoch = TRUE;

  com_sembuf_up.sem_op  = 1;
  com_sembuf_up.sem_flg = 0;
  com_sembuf_up.sem_num = 0;
  com_sembuf_down.sem_op  = -1;
  com_sembuf_down.sem_flg = 0;
  com_sembuf_down.sem_num = 0;
  /* Uff, geschafft! */
  com_communicationPossible = TRUE;
}

BOOLEAN COM_fork(void)
{
  if (com_communicationPossible){/* Also probieren wir es... */
    pid_t pid = fork();
    if (pid == -1){ /* fork hat nicht geklappt */
      our_warning("fork failed!");
    }
    else {
      COM_forked = TRUE;
      if (pid == 0){/* Sohn */
        COM_Vadder = FALSE;
        COM_Prefix = "1> ";
        com_son_pid = getpid();
      }
      else { /* Vadder */
        COM_Vadder = TRUE;
        COM_Prefix = "0> ";
        com_son_pid = pid;
        COM_sleep();
      }
    }
  }
  return COM_forked;
}

void COM_free(void) 
{
  if (com_sem_allocated && !(COM_forked && !COM_Vadder)){
    int res;
    res = semctl(com_semid, 0, IPC_RMID);
    if (res == -1){
      our_warning("cannot deallocate semaphores");
    }
    com_sem_allocated = FALSE;
  }
}

/* we use 3 semaphores, identified by num:
 * num == 0: Lock to critical region
 * num == 1: sleeping barrier for Vadder
 * num == 2: sleeping barrier for Sohny
 *
 * Therefore, initialization as {1,0,0}.
 *
 * Lock superfluous because of sleeping.
 */

static BOOLEAN com_sema_up(unsigned num)
{
  int res;
  com_sembuf_up.sem_num = num;
  res = semop(com_semid, &com_sembuf_up, 1);
  if (res != 0){
    return FALSE;
  }
  return TRUE;
}

static BOOLEAN com_sema_down(unsigned num)
{
  int res;
  com_sembuf_down.sem_num = num;
  res = semop(com_semid, &com_sembuf_down, 1);
  if (res != 0){
    return FALSE;
  }
  return TRUE;
}

static void com_setState(COM_StateT state) 
{
  int i = 0;

  for (i = 0; i < NO_OF_TRIES; i++){
    if (COM_PRINT_THINGS){
      printf("%sTry to get lock(%d)\n", COM_Prefix, i);
    }
    if (com_sema_down(0)) break;
    /* es kann z.B. ein signal einschlagen, waehrend wir warten ;-(( */
  } 

  if (i == NO_OF_TRIES){
    our_warning("problems with acquiring lock");
    return;
  }

  if (COM_PRINT_THINGS){
    printf("%sgot lock(%d) --> %d\n", COM_Prefix, i, state);
  }

  if (com_sharedState->state == COM_state_unset) {
    com_sharedState->state = state;
  } 
  else {
    our_warning("illegal state change");
  }

  for (i=0; i < NO_OF_TRIES; i++){
    if (COM_PRINT_THINGS){
      printf("%sTry to release lock(%d)\n", COM_Prefix, i);
    }
    if (com_sema_up(0)) break;
  } 

  if (i == NO_OF_TRIES){
    our_warning("problems with releasing lock");
  }
}

COM_StateT COM_getState(void)
{
  return COM_forked ? com_sharedState->state : COM_state_unset;
}

void COM_markSuccess(void) 
{ 
  if (COM_forked) {
    if (COM_Vadder) {
      com_setState(COM_state_father_ready);
      if (COM_PRINT_THINGS){
        fputs("Vadder hat gewonnen\n", stdout);
      }
    } else {
      com_setState(COM_state_son_ready);
      if (COM_PRINT_THINGS){
        fputs("Sohny hat gewonnen\n", stdout);
      }
    }
  }
}

void COM_kill(void)
{
  if (COM_forked && COM_Vadder){
    kill(com_son_pid, SIGTERM);
    if (COM_PRINT_THINGS){
      fputs("Sohn toeten\n", stdout);
    }
  }
}

void COM_wait(void)
{
  if (COM_forked && COM_Vadder){
    int status;
    pid_t pid = wait(&status);
    if (COM_PRINT_THINGS){
      fputs("Sohn eingesammelt\n", stdout);
    }
  }
}

extern unsigned int HK_SelectZaehler;

void COM_wakeOther(void)
{
  if (COM_forked){
    if (COM_Vadder){
      com_sharedState->VadderTime = HK_SelectZaehler/*HK_AbsTime*/;
      com_sema_up(2);
    }
    else {
      com_sharedState->SohnyTime = HK_SelectZaehler/*HK_AbsTime*/;
      com_sema_up(1);
    }
  }
}

void COM_sleep(void)
{
  if (COM_forked){
    if (COM_Vadder){
      if (com_sharedState->SohnyRechnetNoch){
        com_sema_down(1); 
      }
    }
    else {
      if (com_sharedState->VadderRechnetNoch){
        com_sema_down(2); 
      }
    }
  }
}

void COM_stopComputing(void)
{
  if (COM_forked){
    if (COM_Vadder){
      com_sharedState->VadderRechnetNoch = FALSE;
    }
    else {
      com_sharedState->SohnyRechnetNoch = FALSE;
    }
  }
}

AbstractTime COM_GlobalTime(void)
{
  if (!COM_forked){
    return HK_SelectZaehler/*HK_AbsTime*/;
  }
  if (COM_Vadder){
    return HK_SelectZaehler/*HK_AbsTime*/ + com_sharedState->SohnyTime;
  }
  else {
    return HK_SelectZaehler/*HK_AbsTime*/ + com_sharedState->VadderTime;
  }
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#else 
/* use fctl record locks */

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Ausgaben.h"

#define NO_OF_TRIES 3

static BOOLEAN com_communicationPossible = FALSE;

static pid_t com_son_pid;

static int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
  struct flock	lock;

  lock.l_type = type;		/* F_RDLCK, F_WRLCK, F_UNLCK */
  lock.l_start = offset;	/* byte offset, relative to l_whence */
  lock.l_whence = whence;	/* SEEK_SET, SEEK_CUR, SEEK_END */
  lock.l_len = len;		/* #bytes (0 means to EOF) */

  return( fcntl(fd, cmd, &lock) );	/* -1 upon error */
}

#define	writew_lock(fd, offset, whence, len) lock_reg(fd, F_SETLKW, F_WRLCK, offset, whence, len)
#define	un_lock(fd, offset, whence, len)     lock_reg(fd, F_SETLK, F_UNLCK, offset, whence, len)

typedef struct com_syncData {
  short state;
  int fd;
} *com_syncDataT;

static com_syncDataT com_sharedState; 


void COM_init(void) 
{
  char pathname[] = "/tmp/wmXXXXXX";
  int fd;

  if (!PA_ForkErlaubt()){
    return;
  }

  fd = mkstemp(pathname);
  if (fd == -1){
    our_warning("cannot create lockfile\n");
    return;
  }

  if (lseek(fd, sizeof(*com_sharedState) - 1, SEEK_SET) == (off_t)-1){
    our_warning("cannot enlarge (lseek) lockfile\n");
    return;
  }

  if (write(fd, "", 1) == (ssize_t)-1){
    our_warning("cannot enlarge (write) lockfile\n");
    return;
  }

  com_sharedState = mmap(NULL, sizeof(*com_sharedState),PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0);
  if (com_sharedState == (com_syncDataT)-1){
    our_warning("no shared data\n");
    return;
  }

  com_sharedState->state = COM_state_unset;
  com_sharedState->fd    = fd;

  if (unlink(pathname) == -1){
    our_warning("cannot unlink lockfile\n");
    return;
  }

  if (writew_lock(com_sharedState->fd, 0, SEEK_SET, 0) == -1){
    our_warning("lock test failed\n");
    return;
  }

  if (un_lock(com_sharedState->fd, 0, SEEK_SET, 0) == -1){
    our_warning("unlock test failed\n");
    return;
  }

  /* Uff, geschafft! */
  com_communicationPossible = TRUE;
}

BOOLEAN COM_fork(void)
{
  if (com_communicationPossible){/* Also probieren wir es... */
    pid_t pid = fork();
    if (pid == -1){ /* fork hat nicht geklappt */
      our_warning("fork failed!");
    }
    else {
      COM_forked = TRUE;
      if (pid == 0){/* Sohn */
        COM_Vadder = FALSE;
        COM_Prefix = "1> ";
        com_son_pid = getpid();
        nice(15);
      }
      else { /* Vadder */
        COM_Vadder = TRUE;
        COM_Prefix = "0> ";
        com_son_pid = pid;
      }
    }
  }
  return COM_forked;
}

void COM_free(void) 
{
  /* nix tun */
}

static void com_setState(COM_StateT state) 
{
  int i, res = 0;

  for (i = 0; i < NO_OF_TRIES; i++){
    if (COM_PRINT_THINGS){
      printf("%sTry to get lock(%d)\n", COM_Prefix, i);
    }
    res = writew_lock(com_sharedState->fd, 0, SEEK_SET, 0);
    /* es kann z.B. ein signal einschlagen, waehrend wir warten ;-(( */
    if (res != -1) break;
  } 

  if (res == -1){
    our_warning("problems with acquiring lock");
    return;
  }

  if (COM_PRINT_THINGS){
    printf("%sgot lock(%d) --> %d\n", COM_Prefix, i, state);
  }

  if (com_sharedState->state == COM_state_unset) {
    com_sharedState->state = state;
  } 
  else {
    our_warning("illegal state change");
  }

  for(i=0; i < NO_OF_TRIES; i++){
    if (COM_PRINT_THINGS){
      printf("%sTry to release lock(%d)\n", COM_Prefix, i);
    }
    res = un_lock(com_sharedState->fd, 0, SEEK_SET, 0);
    if (res != -1) break;
  } 

  if (res == -1){
    our_warning("problems with releasing lock");
  }
}

COM_StateT COM_getState(void)
{
  return COM_forked ? com_sharedState->state : COM_state_unset;
}

void COM_markSuccess(void) 
{ 
  if (COM_forked) {
    if (COM_Vadder) {
      com_setState(COM_state_father_ready);
      if (COM_PRINT_THINGS){
        fputs("Vadder hat gewonnen\n", stdout);
      }
    } else {
      com_setState(COM_state_son_ready);
      if (COM_PRINT_THINGS){
        fputs("Sohny hat gewonnen\n", stdout);
      }
    }
  }
}

void COM_kill(void)
{
  if (COM_forked && COM_Vadder){
    kill(com_son_pid, SIGTERM);
    if (COM_PRINT_THINGS){
      fputs("Sohn toeten\n", stdout);
    }
  }
}

void COM_wait(void)
{
  if (COM_forked && COM_Vadder){
    int status;
    pid_t pid = wait(&status);
    if (COM_PRINT_THINGS){
      fputs("Sohn eingesammelt\n", stdout);
    }
  }
}

void COM_wakeOther(void)
{
  /* not supported */
}

void COM_sleep(void)
{
  /* not supported */
}

AbstractTime COM_GlobalTime(void)
{
  return minimalAbstractTime();  /* not supported */
}

void COM_stopComputing(void)
{
  /* not supported */
}

#endif

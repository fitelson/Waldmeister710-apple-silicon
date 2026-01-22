#include "FehlerBehandlung.h"
#include "SpeicherVerwaltung.h"
#include "VariablenDiffMMengen.h"

#define VMS_TEST 0 /* 0, 1 */
#if VMS_TEST
#include <assert.h>
#else
#define assert(x) /* nix */
#endif

struct VarDMSetS {
  int count;
  int ptr;
};

/* we are abusing the first and second entry: */

#define vms_last(/*VarDMSetT*/ s) (s[0].count)
#define vms_ptr(/*VarDMSetT*/ s)  (s[0].ptr)
#define vms_poss(/*VarDMSetT*/ s) (s[1].count)
#define vms_negs(/*VarDMSetT*/ s) (s[1].ptr)

#if VMS_TEST
static BOOLEAN vms_check(VarDMSetT s)
{
  /* sigh */
  return TRUE;
}

static BOOLEAN vms_isEmpty(VarDMSetT s)
{
  unsigned int i;
  if ((vms_poss(s) != 0) || (vms_negs(s) != 0) || (vms_ptr(s) != 0)){
    return FALSE;
  }
  for (i = 2; i <= vms_last(s)+1; i++){
    if ((s[i].count != 0) || (s[i].ptr != 0)){
      return FALSE;
    }
  }
  return TRUE;
}
#endif

static void vms_insertList(VarDMSetT s, int x)
{
  if (s[x].ptr == 0){ /* not yet inserted => insert it then */
    if (vms_ptr(s) == 0){ /* list is empty */
      s[x].ptr = x;
    }
    else {
      s[x].ptr = vms_ptr(s);
    }
    vms_ptr(s) = x;
  }
}

static void vms_clearList(VarDMSetT s)
{
  if (vms_ptr(s) != 0){ /* list is not empty */
    int ptr, ptr2;
    ptr2 = vms_ptr(s);
    do {
      ptr  = ptr2;
      ptr2 = s[ptr].ptr;
      s[ptr].count = 0;
      s[ptr].ptr   = 0;
    } while (ptr != ptr2);
    vms_ptr(s) = 0;
  }
}

VarDMSetT VMS_alloc(int lastId)
{
  assert(1 <= lastId);
  {
    int i;
    VarDMSetT res = our_alloc((lastId + 2) * sizeof(*res));
    vms_last(res) = lastId;
    vms_ptr(res)  = 0;
    vms_poss(res) = 0;
    vms_negs(res) = 0;
    for (i = 2; i <= lastId+1; i++){
      res[i].count = 0;
      res[i].ptr   = 0;
    }
    assert(vms_isEmpty(res));
    return res;
  }
}

VarDMSetT VMS_enlarge(VarDMSetT s, int lastId)
{
  assert(lastId < vms_last(s));
  {
    int i = vms_last(s) + 2;
    VarDMSetT res = our_realloc(s, (lastId + 2) * sizeof(*res), "VMS_enlarge");
    for (; i <= lastId+1; i++){
      res[i].count = 0;
      res[i].ptr   = 0;
    }
    vms_last(res) = lastId;
    assert(vms_check(res));
    return res;
  }
}

void VMS_dealloc(VarDMSetT s)
{
  our_dealloc(s);
}

void VMS_clear(VarDMSetT s)
{
  assert(vms_check(s));  
  vms_clearList(s);
  vms_poss(s) = 0;
  vms_negs(s) = 0;
  assert(vms_isEmpty(s));
}

void VMS_inc(VarDMSetT s, int x)
{
  assert((1 <= x) && (x <= vms_last(s)));
  x++;
  s[x].count++;
  if (s[x].count == 0){
    vms_negs(s)--;
  }
  else if (s[x].count == 1){
    vms_poss(s)++;
    vms_insertList(s,x);
  }
  assert(vms_check(s));
}

void VMS_dec(VarDMSetT s, int x)
{
  assert((1 <= x) && (x <= vms_last(s)));
  x++;
  s[x].count--;
  if (s[x].count == 0){
    vms_poss(s)--;
  }
  else if (s[x].count == -1){
    vms_negs(s)++;
    vms_insertList(s,x);
  }
  assert(vms_check(s));
}

VergleichsT VMS_compare(VarDMSetT s)
{
  assert(vms_check(s));
  if ((vms_poss(s) > 0) && (vms_negs(s) > 0)) return Unvergleichbar;
  if (vms_poss(s) > 0) return Groesser;
  if (vms_negs(s) > 0) return Kleiner;
  return Gleich;
}

BOOLEAN VMW_noNeg(VarDMSetT s)
{
  assert(vms_check(s));
  return vms_negs(s) == 0; 
}

BOOLEAN VMW_noPos(VarDMSetT s)
{
  assert(vms_check(s));
  return vms_poss(s) == 0; 
}

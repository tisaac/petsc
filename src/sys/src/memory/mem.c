
#ifndef lint
static char vcid[] = "$Id: mem.c,v 1.7 1997/03/26 17:21:03 balay Exp balay $";
#endif

#include "petsc.h"           /*I "petsc.h" I*/

#if !defined (PARCH_t3d)
#include <sys/types.h>
#include <sys/time.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <sys/resource.h>
#if defined(__cplusplus)
extern "C" {
#endif
extern int getrusage(int,struct rusage*);
extern int getpagesize();
#if defined(__cplusplus)
}
#endif
#endif
#if defined(PARCH_solaris)
#include <sys/procfs.h>
#include <fcntl.h>
#endif

#undef __FUNC__  
#define __FUNC__ "PetscGetResidentSetSize" /* ADIC Ignore */
/*@
   PetscGetResidentSetSize - Returns the maximum resident set size (memory used)
      for the program.

   Output Parameters:
     mem - memory usage in bytes

     Options Database:
.     -trmalloc_log

   Notes: The memory usage reported here includes all arrays in Fortran, so gives 
   a more complete picture of memory usage then PetscTrSpace() if you are using
   Fortran with hardwired arrays.

.seealso: PetscTrSpace()

@*/
int PetscGetResidentSetSize(PLogDouble *foo)
{
#if !defined(PARCH_solaris) && !defined(PARCH_hpux) && !defined(PARCH_t3d)
  static struct rusage temp;
  getrusage(RUSAGE_SELF,&temp);
#if defined(PARCH_rs6000)
  /* RS6000 always reports sizes in k instead of pages */
  *foo = 1024.0 * ((double) temp.ru_maxrss);
#else
  *foo = ( (double) getpagesize())*( (double) temp.ru_maxrss );
#endif
#elif defined(PARCH_solaris)
  int             fd;
  char            proc[1024];
  prpsinfo_t      prusage;
  sprintf(proc,"/proc/%d", getpid());
  if ((fd = open(proc,O_RDONLY)) == -1) SETERRQ(1,1,"Unable to access system files");
  if (ioctl(fd, PIOCPSINFO,&prusage) == -1) SETERRQ(1,1,"Unable to access system files"); 
  *foo = (double) prusage.pr_byrssize;
  close(fd);
#else
  *foo = 0.0;
#endif
  return 0;
}

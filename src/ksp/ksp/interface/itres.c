
#include "src/ksp/ksp/kspimpl.h"   /*I "petscksp.h" I*/

#undef __FUNCT__  
#define __FUNCT__ "KSPInitialResidual"
/*@C
   KSPInitialResidual - Computes the residual.

   Collective on KSP

   Input Parameters:
+  vsoln    - solution to use in computing residual
.  vt1, vt2 - temporary work vectors
.  vres     - calculated residual
-  vb       - right-hand-side vector

   Notes:
   This routine assumes that an iterative method, designed for
$     A x = b
   will be used with a preconditioner, C, such that the actual problem is either
$     AC u = f (right preconditioning) or
$     CA x = Cf (left preconditioning).

   Level: developer

.keywords: KSP, residual

.seealso:  KSPMonitor()
@*/
PetscErrorCode KSPInitialResidual(KSP ksp,Vec vsoln,Vec vt1,Vec vt2,Vec vres,Vec vb)
{
  PetscScalar   mone = -1.0;
  MatStructure  pflag;
  Mat           Amat,Pmat;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  PetscValidHeaderSpecific(vsoln,VEC_COOKIE,2);
  PetscValidHeaderSpecific(vres,VEC_COOKIE,5);
  PetscValidHeaderSpecific(vb,VEC_COOKIE,6);
  PCGetOperators(ksp->pc,&Amat,&Pmat,&pflag);
  if (!ksp->guess_zero) {
    /* skip right scaling since current guess already has it */
    ierr = KSP_MatMult(ksp,Amat,vsoln,vt1);CHKERRQ(ierr);
    ierr = VecCopy(vb,vt2);CHKERRQ(ierr);
    ierr = VecAXPY(&mone,vt1,vt2);CHKERRQ(ierr);
    ierr = (ksp->pc_side == PC_RIGHT)?(VecCopy(vt2,vres)):(KSP_PCApply(ksp,vt2,vres));CHKERRQ(ierr);
    ierr = PCDiagonalScaleLeft(ksp->pc,vres,vres);CHKERRQ(ierr);
  } else {
    if (ksp->pc_side == PC_RIGHT) {
      ierr = PCDiagonalScaleLeft(ksp->pc,vb,vres);CHKERRQ(ierr);
    } else if (ksp->pc_side == PC_LEFT) {
      ierr = KSP_PCApply(ksp,vb,vres);CHKERRQ(ierr);
      ierr = PCDiagonalScaleLeft(ksp->pc,vres,vres);CHKERRQ(ierr);
    } else if (ksp->pc_side == PC_SYMMETRIC) {
      ierr = PCApplySymmetricLeft(ksp->pc, vb, vres);CHKERRQ(ierr);
    } else {
      SETERRQ1(PETSC_ERR_SUP, "Invalid preconditioning side %d", (int)ksp->pc_side);
  }

  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "KSPUnwindPreconditioner"
/*@
   KSPUnwindPreconditioner - Unwinds the preconditioning in the solution. That is,
     takes solution to the preconditioned problem and gets the solution to the 
     original problem from it.

   Collective on KSP

   Input Parameters:
+  ksp  - iterative context
.  vsoln - solution vector 
-  vt1   - temporary work vector

   Output Parameter:
.  vsoln - contains solution on output  

   Notes:
   If preconditioning either symmetrically or on the right, this routine solves 
   for the correction to the unpreconditioned problem.  If preconditioning on 
   the left, nothing is done.

   Level: advanced

.keywords: KSP, unwind, preconditioner

.seealso: KSPSetPreconditionerSide()
@*/
PetscErrorCode KSPUnwindPreconditioner(KSP ksp,Vec vsoln,Vec vt1)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_COOKIE,1);
  PetscValidHeaderSpecific(vsoln,VEC_COOKIE,2);
  if (ksp->pc_side == PC_RIGHT) {
    ierr = KSP_PCApply(ksp,vsoln,vt1);CHKERRQ(ierr);
    ierr = PCDiagonalScaleRight(ksp->pc,vt1,vsoln);CHKERRQ(ierr);
  } else if (ksp->pc_side == PC_SYMMETRIC) {
    ierr = PCApplySymmetricRight(ksp->pc,vsoln,vt1);CHKERRQ(ierr);
    ierr = VecCopy(vt1,vsoln);CHKERRQ(ierr);
  } else {
    ierr = PCDiagonalScaleRight(ksp->pc,vsoln,vsoln);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* Defines the basic SNES object */
#include <private/snesimpl.h>

static PetscErrorCode    BuildNGmresSoln(PetscScalar*,Vec,SNES,PetscInt,PetscInt);

/* Private structure for the Anderson mixing method aka nonlinear Krylov */
typedef struct {
  PetscScalar *hh_origin;
  Vec       *v, *w, *q;
  PetscReal *f2;    /* 2-norms of function (residual) at each stage */
  PetscInt   msize; /* maximum size of space */
  PetscInt   csize; /* current size of space */
  PetscScalar beta; /* relaxation parameter */
  PetscScalar *nrs;            /* temp that holds the coefficients of the Krylov vectors that form the minimum residual solution */

} SNES_NGMRES;

#define HH(a,b)  (ngmres->hh_origin + (a)*(ngmres->msize)+(b))

#undef __FUNCT__
#define __FUNCT__ "SNESReset_NGMRES"
PetscErrorCode SNESReset_NGMRES(SNES snes)
{
  SNES_NGMRES   *ngmres = (SNES_NGMRES*) snes->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecDestroyVecs(ngmres->msize, &ngmres->v);CHKERRQ(ierr);
  ierr = VecDestroyVecs(ngmres->msize, &ngmres->w);CHKERRQ(ierr);
  ierr = VecDestroyVecs(ngmres->msize, &ngmres->q);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESDestroy_NGMRES"
PetscErrorCode SNESDestroy_NGMRES(SNES snes)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = SNESReset_NGMRES(snes);CHKERRQ(ierr);
  if (snes->work) {ierr = VecDestroyVecs(snes->nwork, &snes->work);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESSetUp_NGMRES"
PetscErrorCode SNESSetUp_NGMRES(SNES snes)
{
  SNES_NGMRES   *ngmres = (SNES_NGMRES *) snes->data;
  PetscInt msize,hh;
  PetscErrorCode ierr;

  PetscFunctionBegin;
#if 0
  if (snes->pc_side != PC_LEFT) {SETERRQ(((PetscObject) snes)->comm, PETSC_ERR_SUP, "Only left preconditioning allowed for SNESNGMRES");}
#endif
  ngmres->beta  = 1.0;
  msize         = ngmres->msize;  /* restart size */
  hh            = msize * msize;
  ierr = PetscMalloc2(hh,PetscScalar,&ngmres->hh_origin,msize,PetscScalar,&ngmres->nrs);CHKERRQ(ierr);

  ierr = PetscMemzero(ngmres->hh_origin,hh*sizeof(PetscScalar));CHKERRQ(ierr);
  ierr = PetscMemzero(ngmres->nrs,msize*sizeof(PetscScalar));CHKERRQ(ierr);

  //  ierr = PetscLogObjectMemory(ksp,(hh+msize)*sizeof(PetscScalar));CHKERRQ(ierr);

  ierr = PetscLogObjectMemory(snes,(hh+msize)*sizeof(PetscScalar));CHKERRQ(ierr);

  //ierr = SNESGetVecs(snes,ngmres->msize,&ngmres->v,ngmres->msize*2,&ngmres->w);CHKERRQ(ierr);
  //ierr = VecDuplicate(snes->vec_sol, ngmres->w);CHKERRQ(ierr);
  //ierr = VecDuplicateVecs(ngmres->w[0], ngmres->msize, &ngmres->q);CHKERRQ(ierr);

  ierr = VecDuplicateVecs(snes->vec_sol, ngmres->msize, &ngmres->v);CHKERRQ(ierr);
  ierr = VecDuplicateVecs(snes->vec_sol, ngmres->msize*2, &ngmres->w);CHKERRQ(ierr);
  ierr = VecDuplicateVecs(snes->vec_sol, ngmres->msize, &ngmres->q);CHKERRQ(ierr);
  ierr = SNESDefaultGetWork(snes, 3);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESSetFromOptions_NGMRES"
PetscErrorCode SNESSetFromOptions_NGMRES(SNES snes)
{
  SNES_NGMRES   *ngmres = (SNES_NGMRES *) snes->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscOptionsHead("SNES NGMRES options");CHKERRQ(ierr);
    ierr = PetscOptionsInt("-snes_ngmres_restart", "Number of directions", "SNES", ngmres->msize, &ngmres->msize, PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESView_NGMRES"
PetscErrorCode SNESView_NGMRES(SNES snes, PetscViewer viewer)
{
  SNES_NGMRES   *ngmres = (SNES_NGMRES *) snes->data;
  PetscBool      iascii;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscTypeCompare((PetscObject) viewer, PETSCVIEWERASCII, &iascii);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerASCIIPrintf(viewer, "  Size of space %d\n", ngmres->msize);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESSolve_NGMRES"

PetscErrorCode SNESSolve_NGMRES(SNES snes)
{
  SNES           pc;
  SNES_NGMRES   *ngmres = (SNES_NGMRES *) snes->data;
  //Vec            X,F,R, Fold, Xold,temp,*dX = ngmres->v,*dF = ngmres->w;
  Vec            X,F,R,Fold, Xold,temp,*dX = ngmres->w,*dF = ngmres->w+ngmres->msize;
  PetscScalar    *nrs=ngmres->nrs;
  PetscReal      gnorm, xnorm, pnorm;
  PetscInt       i, j, k,ivec, l, flag, it;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  snes->reason  = SNES_CONVERGED_ITERATING;
  X             = snes->vec_sol;
  F             = snes->vec_func;
  Fold          = snes->work[0];
  Xold          = snes->work[1];
  R             = snes->work[2];
  ierr = VecDuplicate(X,&Xold);CHKERRQ(ierr);
  ierr = VecDuplicate(X,&temp);CHKERRQ(ierr);
  ierr = VecDuplicate(X,&Fold);CHKERRQ(ierr);

  ierr = SNESGetPC(snes, &pc);CHKERRQ(ierr);
  ierr = PetscObjectTakeAccess(snes);CHKERRQ(ierr);
  snes->iter = 0;
  snes->norm = 0.;
  ierr = PetscObjectGrantAccess(snes);CHKERRQ(ierr);
  ierr = SNESComputeFunction(snes, X, R);CHKERRQ(ierr);               /* r = F(x) */
#if 0
  ierr = SNESSolve(snes->pc, R, F);CHKERRQ(ierr);                  /* p = P(r) */
#else
  ierr = VecCopy(R, F);CHKERRQ(ierr);                              /* p = r    */
#endif
  if (snes->domainerror) {
    snes->reason = SNES_DIVERGED_FUNCTION_DOMAIN;
    PetscFunctionReturn(0);
  }
  ierr = VecNorm(F, NORM_2, &gnorm);CHKERRQ(ierr);                    /* fnorm = ||r||  */
  if (PetscIsInfOrNanReal(gnorm)) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_FP, "Infinite or not-a-number generated in norm");

  ierr = PetscObjectTakeAccess(snes);CHKERRQ(ierr);
  snes->norm = gnorm;
  ierr = PetscObjectGrantAccess(snes);CHKERRQ(ierr);
  SNESLogConvHistory(snes, gnorm, 0);
  ierr = SNESMonitor(snes, 0, gnorm);CHKERRQ(ierr);
  ierr = (*snes->ops->converged)(snes,0,0.0,0.0,gnorm,&snes->reason,snes->cnvP);CHKERRQ(ierr);
  if (snes->reason) PetscFunctionReturn(0);

  /* for k=0 */

  k=0;
  ierr= VecCopy(X,Xold); CHKERRQ(ierr); /* Xbar_0= X_0 */
  ierr= VecCopy(F,Fold); CHKERRQ(ierr); /* Fbar_0 = f_0= B(b-Ax_0) */
  ierr= VecWAXPY(X, ngmres->beta, Fold, Xold); CHKERRQ(ierr);       /*X_1 = X_bar + beta * Fbar */

  /* to calculate f_1 */
  ierr = SNESComputeFunction(snes, X, R);CHKERRQ(ierr);               /* r = F(x) */
#if 0
  ierr = SNESSolve(snes->pc, temp, F);CHKERRQ(ierr);                  /* p = P(r) */
#else
  ierr = VecCopy(R, F);CHKERRQ(ierr);                              /* p = r    */
#endif
 
  /* calculate dX and dF for k=0 */
  ierr= VecWAXPY(dX[k],-1.0, Xold, X); CHKERRQ(ierr); /* dX= X_1 - X_0 */
  ierr= VecWAXPY(dF[k],-1.0, Fold, F); CHKERRQ(ierr); /* dF= f_1 - f_0 */

  ierr= VecCopy(X,Xold); CHKERRQ(ierr); /* Xbar_0= X_0 */
  ierr= VecCopy(F,Fold); CHKERRQ(ierr); /* Fbar_0 = f_0= B(b-Ax_0) */

  flag=0;

  for (k=1; k<snes->max_its; k += 1) {  /* begin the iteration */     
    l=ngmres->msize;
  
    if(k<l) { 
      l=k;
      ivec=l;
    }
    else{
      ivec=l-1;
    }
    it=l-1;
    ierr = BuildNGmresSoln(nrs,Fold,snes,it,flag);CHKERRQ(ierr);
    
    /* to obtain the solution at k+1 step */
    ierr= VecCopy(Xold,X); CHKERRQ(ierr); /* X=Xold+Fold-(dX + dF) *nrd */
    ierr= VecAXPY(X,1.0,Fold); CHKERRQ(ierr); /* X= Xold+Fold */
    for(i=0;i<l;i++){      /*X= Xold+Fold- (dX+dF*beta) *innerb */
      ierr= VecAXPY(X,-nrs[i], dX[i]);CHKERRQ(ierr);
      ierr= VecAXPY(X,-nrs[i]*ngmres->beta, dF[i]);CHKERRQ(ierr);
    }

    /* to calculate f_k+1 */
    ierr = SNESComputeFunction(snes, X, R);CHKERRQ(ierr);               /* r = F(x) */

#if 0
    ierr = SNESSolve(snes->pc, R, F);CHKERRQ(ierr);                  /* p = P(r) */
#else
    ierr = VecCopy(R, F);CHKERRQ(ierr);                              /* p = r    */
#endif

    /* check the convegence */

    ierr = VecNorm(R, NORM_2, &gnorm);CHKERRQ(ierr);                    /* fnorm = ||r||  */
    ierr = VecNorm(dX[l-1], NORM_2, &pnorm);CHKERRQ(ierr);                    /* fnorm = ||r||*/
    ierr = VecNorm(X, NORM_2, &xnorm);CHKERRQ(ierr);                    /* fnorm = ||r||  */
    SNESLogConvHistory(snes, gnorm, k);
    ierr = SNESMonitor(snes, k, gnorm);CHKERRQ(ierr);
    snes->iter =k;
    ierr = (*snes->ops->converged)(snes,snes->iter,xnorm,pnorm,gnorm,&snes->reason,snes->cnvP);CHKERRQ(ierr);
    if (snes->reason) PetscFunctionReturn(0);

   

    /* calculate dX and dF for k=0 */
      if( k>ivec) {/* we need to replace the old vectors */
 	flag=1;
	for(i=0;i<it;i++){
	  ierr= VecCopy(dX[i+1],dX[i]); CHKERRQ(ierr); /* X=Xold+Fold-(dX + dF) *nrd */
	  ierr= VecCopy(dF[i+1],dF[i]); CHKERRQ(ierr); /* X=Xold+Fold-(dX + dF) *nrd */
	  for(j=0;j<l;j++)
	    *HH(j,i)=*HH(j,i+1);
	}
      }
      
      ierr= VecWAXPY(dX[ivec],-1.0, Xold, X); CHKERRQ(ierr); /* dX= X_1 - X_0 */
      ierr= VecWAXPY(dF[ivec],-1.0, Fold, F); CHKERRQ(ierr); /* dF= f_1 - f_0 */
      ierr= VecCopy(X,Xold); CHKERRQ(ierr);
      ierr= VecCopy(F,Fold); CHKERRQ(ierr);
  
  }
  snes->reason = SNES_DIVERGED_MAX_IT;
  PetscFunctionReturn(0);
}

/*MC
  SNESNGMRES - The Nonlinear Generalized Minimum Residual (NGMRES) method of Oosterlee and Washio.

   Level: beginner

   Notes: Supports only left preconditioning

   "Krylov Subspace Acceleration of Nonlinear Multigrid with Application to Recirculating Flows", C. W. Oosterlee and T. Washio,
   SIAM Journal on Scientific Computing, 21(5), 2000.

.seealso: SNESCreate(), SNES, SNESSetType(), SNESType (for list of available types)
M*/
EXTERN_C_BEGIN
#undef __FUNCT__
#define __FUNCT__ "SNESCreate_NGMRES"
PetscErrorCode SNESCreate_NGMRES(SNES snes)
{
  SNES_NGMRES   *ngmres;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  snes->ops->destroy        = SNESDestroy_NGMRES;
  snes->ops->setup          = SNESSetUp_NGMRES;
  snes->ops->setfromoptions = SNESSetFromOptions_NGMRES;
  snes->ops->view           = SNESView_NGMRES;
  snes->ops->solve          = SNESSolve_NGMRES;
  snes->ops->reset          = SNESReset_NGMRES;

  ierr = PetscNewLog(snes, SNES_NGMRES, &ngmres);CHKERRQ(ierr);
  snes->data = (void*) ngmres;
  ngmres->msize = 30;
  ngmres->csize = 0;

  ierr = SNESGetPC(snes, &snes->pc);CHKERRQ(ierr);
#if 0
  if (ksp->pc_side != PC_LEFT) {ierr = PetscInfo(ksp,"WARNING! Setting PC_SIDE for NGMRES to left!\n");CHKERRQ(ierr);}
  snes->pc_side = PC_LEFT;
#endif
  PetscFunctionReturn(0);
}
EXTERN_C_END


#undef __FUNCT__  
#define __FUNCT__ "BuildNGmresSoln"
static PetscErrorCode BuildNGmresSoln(PetscScalar* nrs, Vec Fold, SNES snes,PetscInt it, PetscInt flag)
{
  PetscScalar    tt,temps;
  PetscErrorCode ierr;
  PetscInt       i,ii,j,l;
  SNES_NGMRES      *ngmres = (SNES_NGMRES *)(snes->data);
  //Vec *dF=ngmres->w+ngmres->msize, *Q=ngmres->w+ngmres->msize*2,temp;
  Vec *dF=ngmres->w+ngmres->msize,*Q=ngmres->q,temp;
  PetscReal      gam,areal;
  PetscScalar    a,b,c,s;

  PetscFunctionBegin;
  ierr = VecDuplicate(Fold,&temp);CHKERRQ(ierr);
  l=it+1;
    
  /* Solve for solution vector that minimizes the residual */

  if(flag==1) { // we need to replace the old vector and need to modify the QR factors, use Givens rotation
      for(i=0;i<it;i++){
	/* calculate the Givens rotation */
	a=*HH(i,i);
	b=*HH(i+1,i);
        gam=1.0/PetscRealPart(PetscSqrtScalar(PetscConj(a)*a + PetscConj(b)*b));
        c= a*gam;
        s= b*gam;
     
#if defined(PETSC_USE_COMPLEX)
	/* update the Q factor */
        ierr= VecCopy(Q[i],temp); CHKERRQ(ierr); 
	ierr = VecAXPBY(temp,s,PetscConj(c),Q[i+1]);CHKERRQ(ierr); /*temp= c*Q[i]+s*Q[i+1] */
        ierr = VecAXPBY(Q[i+1],-s,c,Q[i]);CHKERRQ(ierr); /* Q[i+1]= -s*Q[i] + c*Q[i+1] */
        ierr= VecCopy(temp,Q[i]); CHKERRQ(ierr);   /* Q[i]= c*Q[i] + s*Q[i+1] */
        /* update the R factor */
        for(j=0;j<l;j++){
          a= *HH(i,j);
          b=*HH(i+1,j);
	  temps=PetscConj(c)* a+s* b;           
          *HH(i+1,j)=-s*a+c*b;
          *HH(i,j)=temps;
        } 
#else
	/* update the Q factor */
        ierr= VecCopy(Q[i],temp); CHKERRQ(ierr); 
	ierr = VecAXPBY(temp,s,c,Q[i+1]);CHKERRQ(ierr); /*temp= c*Q[i]+s*Q[i+1] */
        ierr = VecAXPBY(Q[i+1],-s,c,Q[i]);CHKERRQ(ierr); /* Q[i+1]= -s*Q[i] + c*Q[i+1] */
        ierr= VecCopy(temp,Q[i]); CHKERRQ(ierr);   /* Q[i]= c*Q[i] + s*Q[i+1] */
        /* update the R factor */
        for(j=0;j<l;j++){
          a= *HH(i,j);
          b=*HH(i+1,j);
	  temps=c* a+s* b;           
          *HH(i+1,j)=-s*a+c*b;
          *HH(i,j)=temps;
        }
#endif 
      }
    }

    // add a new vector, use modified Gram-Schmidt 
    ierr= VecCopy(dF[it],temp); CHKERRQ(ierr);
    for(i=0;i<it;i++){
      ierr=VecDot(temp,Q[i],HH(i,it));CHKERRQ(ierr); /* h(i,l-1)= dF[l-1]'*Q[i] */
      ierr = VecAXPBY(temp,-*HH(i,it),1.0,Q[i]);CHKERRQ(ierr); /* temp= temp- h(i,l-1)*Q[i] */ 
    }
    ierr=VecCopy(temp,Q[it]);CHKERRQ(ierr); 
    ierr=VecNormalize(Q[it],&areal);CHKERRQ(ierr);
    *HH(it,it) = areal;
    


    /* modify the RHS with Q'*Fold*/
  
  for(i=0;i<l;i++) 
          ierr=VecDot(Fold,Q[i],ngmres->nrs+i);CHKERRQ(ierr); /* nrs= Fold'*Q[i] */
      
    /* start backsubstitution to solve the least square problem */      

     if (*HH(it,it) != 0.0) {
      nrs[it] =  nrs[it]/ *HH(it,it);
    } else {
    snes->reason = SNES_DIVERGED_MAX_IT;
    ierr = PetscInfo2(snes,"Likely your matrix or preconditioner is singular. HH(it,it) is identically zero; it = %D nrs(it) = %G",it,10);
    PetscFunctionReturn(0);
  }
  for (ii=1; ii<=it; ii++) {
    i   = it - ii;
    tt  = nrs[i];
    for (j=i+1; j<=it; j++) tt  = tt - *HH(i,j) * nrs[j];
    if (*HH(i,i) == 0.0) {
      snes->reason = SNES_DIVERGED_MAX_IT;
      ierr = PetscInfo1(snes,"Likely your matrix or preconditioner is singular. HH(k,k) is identically zero; i = %D",i);
      PetscFunctionReturn(0);
    } 
    nrs[i]   = tt / *HH(i,i);
  }

  
  PetscFunctionReturn(0);
}
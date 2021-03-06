#if !defined(__PETSC_P4EST_PACKAGE_H)
#define      __PETSC_P4EST_PACKAGE_H
#include <petscsys.h>
#include <p4est_base.h>

#if defined(PETSC_HAVE_SETJMP_H) && defined(PETSC_USE_ERRORCHECKING)
#include <setjmp.h>
PETSC_INTERN jmp_buf PetscScJumpBuf;

#define PetscStackCallP4est(func,args) do {                                                                                                           \
  if (setjmp(PetscScJumpBuf)) {                                                                                                                       \
    return PetscError(PETSC_COMM_SELF,__LINE__,PETSC_FUNCTION_NAME,__FILE__,PETSC_ERR_LIB,PETSC_ERROR_REPEAT,"Error in p4est/libsc call %s()",#func); \
  }                                                                                                                                                   \
  else {                                                                                                                                              \
    PetscStackPush(#func);                                                                                                                            \
    func args;                                                                                                                                        \
    PetscStackPop;                                                                                                                                    \
  }                                                                                                                                                   \
} while (0)
#define PetscStackCallP4estReturn(ret,func,args) do {                                                                                                 \
  if (setjmp(PetscScJumpBuf)) {                                                                                                                       \
    return PetscError(PETSC_COMM_SELF,__LINE__,PETSC_FUNCTION_NAME,__FILE__,PETSC_ERR_LIB,PETSC_ERROR_REPEAT,"Error in p4est/libsc call %s()",#func); \
  }                                                                                                                                                   \
  else {                                                                                                                                              \
    PetscStackPush(#func);                                                                                                                            \
    ret = func args;                                                                                                                                  \
    PetscStackPop;                                                                                                                                    \
  }                                                                                                                                                   \
} while (0)
#else
#define PetscStackCallP4est(func,args) do { \
  PetscStackPush(#func);                    \
  func args;                                \
  PetscStackPop;                            \
} while (0)
#define PetscStackCallP4estReturn(ret,func,args) do { \
  PetscStackPush(#func);                              \
  ret = func args;                                    \
  PetscStackPop;                                      \
} while (0)
#endif

PETSC_EXTERN PetscErrorCode PetscP4estInitialize();

#undef __FUNCT__
#define __FUNCT__ "P4estLocidxCast"
PETSC_STATIC_INLINE PetscErrorCode P4estLocidxCast(PetscInt a,p4est_locidx_t *b)
{
  PetscFunctionBegin;
  *b =  (p4est_locidx_t)(a);
#if defined(PETSC_USE_64BIT_INDICES)
  if ((a) > P4EST_LOCIDX_MAX) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Index to large for p4est_locidx_t");
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "P4estTopidxCast"
PETSC_STATIC_INLINE PetscErrorCode P4estTopidxCast(PetscInt a,p4est_topidx_t *b)
{
  PetscFunctionBegin;
  *b =  (p4est_topidx_t)(a);
#if defined(PETSC_USE_64BIT_INDICES)
  if ((a) > P4EST_TOPIDX_MAX) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Index to large for p4est_topidx_t");
#endif
  PetscFunctionReturn(0);
}

#endif

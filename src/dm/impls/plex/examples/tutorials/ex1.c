static char help[] = "Define a simple field over the mesh\n\n";

#include <petscdmplex.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc, char **argv)
{
  DM             dm;
  PetscSection   section;
  PetscInt       dim = 2, numFields, numBC, i;
  PetscInt       numComp[3];
  PetscInt       numDof[12];
  PetscInt       bcField[1];
  IS             bcPointIS[1];
  PetscBool      interpolate = PETSC_TRUE;
  PetscErrorCode ierr;

  ierr = PetscInitialize(&argc, &argv, NULL, help);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL, "-dim", &dim, NULL);CHKERRQ(ierr);
  /* Create a mesh */
  ierr = DMPlexCreateBoxMesh(PETSC_COMM_WORLD, dim, interpolate, &dm);CHKERRQ(ierr);
  /* Create a scalar field u, a vector field v, and a surface vector field w */
  numFields  = 3;
  numComp[0] = 1;
  numComp[1] = dim;
  numComp[2] = dim-1;
  for (i = 0; i < numFields*(dim+1); ++i) numDof[i] = 0;
  /* Let u be defined on vertices */
  numDof[0*(dim+1)+0]     = 1;
  /* Let v be defined on cells */
  numDof[1*(dim+1)+dim]   = dim;
  /* Let v be defined on faces */
  numDof[2*(dim+1)+dim-1] = dim-1;
  /* Setup boundary conditions */
  numBC = 1;
  /* Prescribe a Dirichlet condition on u on the boundary
       Label "marker" is made by the mesh creation routine */
  bcField[0] = 0;
  ierr = DMPlexGetStratumIS(dm, "marker", 1, &bcPointIS[0]);CHKERRQ(ierr);
  /* Create a PetscSection with this data layout */
  ierr = DMPlexCreateSection(dm, dim, numFields, numComp, numDof, numBC, bcField, bcPointIS, &section);CHKERRQ(ierr);
  /* Name the Field variables */
  ierr = PetscSectionSetFieldName(section, 0, "u");CHKERRQ(ierr);
  ierr = PetscSectionSetFieldName(section, 1, "v");CHKERRQ(ierr);
  ierr = PetscSectionSetFieldName(section, 2, "w");CHKERRQ(ierr);
  ierr = PetscSectionView(section, PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  /* Tell the DM to use this data layout */
  ierr = DMSetDefaultSection(dm, section);CHKERRQ(ierr);
  /* Cleanup */
  ierr = PetscSectionDestroy(&section);CHKERRQ(ierr);
  ierr = DMDestroy(&dm);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
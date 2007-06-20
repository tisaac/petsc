#!/usr/bin/env python
from __future__ import generators
import user
import config.base
import os
import PETSc.package

class Configure(PETSc.package.Package):
  def __init__(self, framework):
    PETSc.package.Package.__init__(self, framework)
    self.download  = ['ftp://ftp.mcs.anl.gov/pub/mpi/mpe/mpe2.tar.gz']
    self.functions = ['MPE_Log_event']
    self.includes  = ['mpe.h']
    self.liblist   = [['libmpe.a']]
    return

  def setupDependencies(self, framework):
    PETSc.package.Package.setupDependencies(self, framework)
    self.mpi        = framework.require('config.packages.MPI',self)
    self.deps       = [self.mpi]
    return
          
  def Install(self):

    args = ['--prefix='+self.installDir]
    
    self.framework.pushLanguage('C')
    args.append('CFLAGS="'+self.framework.getCompilerFlags()+'"')
    args.append('CC="'+self.framework.getCompiler()+'"')
    self.framework.popLanguage()

    args.append('--disable-f77')
    if self.mpi.include:
      args.append('MPI_INC="'+self.headers.toString(self.mpi.include)+'"')
    if self.mpi.lib:
      libdir = os.path.dirname(self.mpi.lib[0])
      libs = []
      for l in self.mpi.lib:
        ll = os.path.basename(l)
        libs.append('-l'+ll[3:-2])
      libs = ' '.join(libs)
      args.append('MPI_LIBS="'+'-L'+libdir+' '+libs+'"')

    args = ' '.join(args)
    
    fd = file(os.path.join(self.packageDir,'mpe'), 'w')
    fd.write(args)
    fd.close()

    if self.installNeeded('mpe'):
      try:
        self.logPrintBox('Configuring mpe; this may take several minutes')
        output  = config.base.Configure.executeShellCommand('cd '+self.packageDir+';./configure '+args, timeout=2000, log = self.framework.log)[0]
      except RuntimeError, e:
        raise RuntimeError('Error running configure on MPE: '+str(e))
      # Build MPE
      try:
        self.logPrintBox('Compiling mpe; this may take several minutes')
        output  = config.base.Configure.executeShellCommand('cd '+self.packageDir+';make clean; make; make install', timeout=2500, log = self.framework.log)[0]
      except RuntimeError, e:
        raise RuntimeError('Error running make on MPE: '+str(e))
      self.checkInstall(output,'mpe')
    return self.installDir
  
if __name__ == '__main__':
  import config.framework
  import sys
  framework = config.framework.Framework(sys.argv[1:])
  framework.setup()
  framework.addChild(Configure(framework))
  framework.configure()
  framework.dumpSubstitutions()

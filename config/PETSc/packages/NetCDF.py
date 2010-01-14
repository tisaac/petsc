import config.base

class Configure(config.base.Configure):
  def __init__(self, framework):
    config.base.Configure.__init__(self, framework)
    self.headerPrefix = ''
    self.substPrefix  = ''
    return

  def configureLibrary(self):
    '''Find a NetCDF installation and check if it can work with PETSc'''
    return

  def configure(self):
    self.executeTest(self.configureLibrary)
    return
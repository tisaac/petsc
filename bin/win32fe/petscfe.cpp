/* $Id: fe.cpp,v 1.1 2001/04/17 15:21:14 buschelm Exp buschelm $ */
#include <iostream>
#include "toolfe.h"

using namespace std;

int main(int argc,char *argv[]) {
  PETScFE::tool *Tool;
  if (argc>1) {
    PETScFE::tool::Create(Tool,(string)(argv[1]));
  } else {
    PETScFE::tool::Create(Tool,"--help");
  }

  Tool->GetArgs(argc,argv);
  Tool->Parse();
  Tool->Execute();
    
  Tool->Destroy();
  return(0);
}

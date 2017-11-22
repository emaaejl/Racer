#include "flowSensitivePA.h"

void FSPAVisitor::initPA(SymTab<SymBase> *symbTab)
{
  _symbTab=symbTab;
  assert(_symbTab);
}  

void FSPAVisitor::analyze(clang::Decl *D)
{
  clang::AnalysisDeclContext ac(/* AnalysisDeclContextManager */ nullptr, D);
  clang::CFG *cfg;
  D->dump();
  errs()<<"obtain CFG\n";
  if (!(cfg = ac.getCFG()))
    return;
  
  errs()<<" Printing CFGs"<<"\n";
  ac.dumpCFG(false);
}

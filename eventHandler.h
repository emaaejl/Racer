#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang;
using namespace clang::ast_matchers;

StatementMatcher LoopMatcher =
  forStmt(hasLoopInit(declStmt(hasSingleDecl(varDecl(
    hasInitializer(integerLiteral(equals(0)))))))).bind("forLoop");

StatementMatcher CallExprMatcher=callExpr(callee(functionDecl(hasName("inc1")))).bind("specificcallexpr");
StatementMatcher CallExprMatcher1=callExpr(callee(functionDecl(hasName("add2")))).bind("specificcallexpr1");

class LoopPrinter : public MatchFinder::MatchCallback {
public :

  std::string getArgumentName(const clang::Expr * exp)
   {
     // Expr is a DeclRefExpr
     if(const clang::DeclRefExpr *ref = clang::dyn_cast<clang::DeclRefExpr>(exp))  
       if(const clang::ValueDecl *vDecl=dyn_cast<clang::ValueDecl>(ref->getDecl()))
	return vDecl->getNameAsString();

     // Expr is a DeclRefExpr preceded by some unary operator
     if(const clang::UnaryOperator *uop=dyn_cast<clang::UnaryOperator>(exp))
       if(const clang::Expr *subexp=uop->getSubExpr()->IgnoreImplicit())
	 return getArgumentName(subexp);
     return "";
   }

  virtual void run(const MatchFinder::MatchResult &Result) {
    const CallExpr *call;
    //if (const ForStmt *FS = Result.Nodes.getNodeAs<clang::ForStmt>("forLoop"))
    //  FS->dump();
    if ( (call= Result.Nodes.getNodeAs<clang::CallExpr>("specificcallexpr"))||((call= Result.Nodes.getNodeAs<clang::CallExpr>("specificcallexpr1"))))
     {
       llvm::outs()<< "Parameters: ";
       for(unsigned i=0;i<call->getNumArgs();i++){
	 const clang::Expr* exp=call->getArg(i)->IgnoreImplicit();
	 llvm::outs()<<getArgumentName(exp)<<" ";
       }
       llvm::outs()<<"\n";
       call->dump();
     } 
    else llvm::outs()<<"Call Expr not found";
  }
};

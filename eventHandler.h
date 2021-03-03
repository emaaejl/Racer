#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "callGraphAnalysis.h"
#include <fstream>

using namespace clang;
using namespace clang::ast_matchers;


StatementMatcher BindThread=callExpr(callee(functionDecl(hasName("bbiTcbb_eventMediator_bindThread"))),hasAncestor(functionDecl().bind("ThreadCaller"))).bind("bThread");
StatementMatcher BindActivator=callExpr(callee(functionDecl(hasName("bbiTcbb_eventMediator_bindActivator"))),hasAncestor(functionDecl().bind("ActivatorCaller"))).bind("bActivator");
StatementMatcher JoinConfig=callExpr(callee(functionDecl(hasName("bbiTcbb_eventMediator_joinConfig"))),hasAncestor(functionDecl().bind("JoinCaller"))).bind("bJoinConfig");
StatementMatcher Trigger=callExpr(callee(functionDecl(hasName("bbiTcbb_eventMediator_trigger"))),hasAncestor(functionDecl().bind("TrigCaller"))).bind("bTrigger");


class EventRecorder : public MatchFinder::MatchCallback {
  std::ofstream & EventFile;
  CGReachability cgR;
 public :
 EventRecorder(std::ofstream & o, CallGraph *cg,std::vector<std::string> startFuncsEvents):EventFile(o),cgR(cg,startFuncsEvents){}

  const clang::Expr* IgnoreOtherCastParenExpr(const clang::Expr *E)
  {

    if (const ImplicitCastExpr *P = dyn_cast<ImplicitCastExpr>(E)) 
      return  IgnoreOtherCastParenExpr(P->getSubExpr()->IgnoreImplicit()->IgnoreParenCasts());
    else if (const CStyleCastExpr *P = dyn_cast<CStyleCastExpr>(E)) 
      return  IgnoreOtherCastParenExpr(P->getSubExpr()->IgnoreImplicit()->IgnoreParenCasts());
    else if (const ParenExpr *P = dyn_cast<ParenExpr>(E)) 
      return  IgnoreOtherCastParenExpr(P->IgnoreParenCasts()->IgnoreImplicit());  
    else return  E->IgnoreParenCasts()->IgnoreImplicit();  
  } 


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
     
     if(const clang::StringLiteral *strExpr=clang::dyn_cast<clang::StringLiteral>(exp)){
       std::string str=strExpr->getString().str();
       return str.substr(17);
     }
     //exp->dump();
     //llvm::errs()<<"Graph: blabla\n";
     return "item_not_found";
   }

  void recordCallArguments(const CallExpr *call)
  {
    for(unsigned i=1;i<call->getNumArgs();i++){
      const clang::Expr* exp=call->getArg(i)->IgnoreImplicit()->IgnoreParenCasts();
      const clang::Expr* E=IgnoreOtherCastParenExpr(exp);
      EventFile<<"\""<<getArgumentName(E)<<"\"";
      if(i==call->getNumArgs()-1) EventFile<<")."; else EventFile<<",";
    }
    EventFile<<std::endl;
  }

  void run(const MatchFinder::MatchResult &Result) override {
    if (const CallExpr *call= Result.Nodes.getNodeAs<clang::CallExpr>("bThread")) 
      //if(const FunctionDecl *caller= Result.Nodes.getNodeAs<clang::FunctionDecl>("ThreadCaller"))
	{
	  // llvm::errs()<<"threadEvent\n";
	  EventFile<<"threadEvent(";
	  recordCallArguments(call);
     }
    if(const CallExpr *call= Result.Nodes.getNodeAs<clang::CallExpr>("bActivator"))
      //if(const FunctionDecl *caller= Result.Nodes.getNodeAs<clang::FunctionDecl>("ActivatorCaller"))
	  {
	    //llvm::errs()<<"ActivatorEvent\n";
	    EventFile<<"activatorEvent(";
	    recordCallArguments(call);
      }  
    if (const CallExpr *call= Result.Nodes.getNodeAs<clang::CallExpr>("bJoinConfig")) 
      // if(const FunctionDecl *caller= Result.Nodes.getNodeAs<clang::FunctionDecl>("JoinCaller"))
	{
	  EventFile<<"joinConfigEvent(";
	  recordCallArguments(call);
	}
    if (const CallExpr *call= Result.Nodes.getNodeAs<clang::CallExpr>("bTrigger"))
	if(const FunctionDecl *caller= Result.Nodes.getNodeAs<clang::FunctionDecl>("TrigCaller")) {

	  std::string callerName=caller->getNameInfo().getAsString();
	  std::string from=cgR.reachableFrom(callerName);
	  std::string sName;
	  if(from=="not_found") sName=callerName; 
	  else sName=from; 
	  EventFile<<"triggerEvent(\""<<sName<<"\",";
	  recordCallArguments(call);
     }
    
  }
};

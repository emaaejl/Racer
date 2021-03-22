/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/


//== CallGraph.cpp - AST-based Call graph  ----------------------*- C++ -*--==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the AST-based CallGraph.
//
//===----------------------------------------------------------------------===//


#include "libExt/CallGraphCtu.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/StmtVisitor.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/GraphWriter.h"

using namespace clang;


#define DEBUG_TYPE "CallGraph"

STATISTIC(NumObjCCallEdges, "Number of Objective-C method call edges");
STATISTIC(NumBlockCallEdges, "Number of block call edges");

namespace {
/// A helper class, which walks the AST and locates all the call sites in the
/// given function body.
class CGBuilder : public StmtVisitor<CGBuilder> {
  CallGraphCtu *G;
  CallGraphNode *CallerNode;
  SteengaardPAVisitor *PA;
public:
  CGBuilder(CallGraphCtu *g, CallGraphNode *N, SteengaardPAVisitor *pa)
    : G(g), CallerNode(N), PA(pa) {}

  void VisitStmt(Stmt *S) { VisitChildren(S); }

  std::set<Decl *> getDeclFromCall(CallExpr *CE) {
    std::set<Decl *> DSet;
    if (FunctionDecl *CalleeDecl = CE->getDirectCallee())
      DSet.insert(CalleeDecl);
    if(PA){
      std::set<FunctionDecl *> FDSet=PA->getCallsFromFuncPtr(CE);
      for(std::set<FunctionDecl *>::iterator it=FDSet.begin();it!=FDSet.end();it++)  
	DSet.insert(*it);
    }  
    else
      llvm::errs()<<"PA not found\n";
    
    // Simple detection of a call through a block.
    Expr *CEE = CE->getCallee()->IgnoreParenImpCasts();
    if (BlockExpr *Block = dyn_cast<BlockExpr>(CEE)) {
      NumBlockCallEdges++;
      DSet.insert(Block->getBlockDecl());
    }

    return DSet;
  }

  void addCalledDecl(Decl *D) {
    /*
      llvm::errs()<<"Caller Node: ";
      CallerNode->print(llvm::errs());
    */  
    if (G->includeInGraph(D) || !D->hasBody()) {
      CallGraphNode *CalleeNode = G->getOrInsertNode(D);
      /*
      llvm::errs()<<"Callee Node: ";
      CalleeNode->print(llvm::errs());
      */
      CallerNode->addCallee(CalleeNode);
      G->removeRootChild(CalleeNode);
      G->addDeclToExport(CalleeNode);
    }
    //llvm::errs()<<"\n";
  }

  void VisitCallExpr(CallExpr *CE) {
    std::set<Decl *> FDSet=getDeclFromCall(CE);
    for(std::set<Decl *>::iterator it=FDSet.begin();it!=FDSet.end();it++)  
      addCalledDecl(*it);
    VisitChildren(CE);
  }

  // Adds may-call edges for the ObjC message sends.
  void VisitObjCMessageExpr(ObjCMessageExpr *ME) {
    if (ObjCInterfaceDecl *IDecl = ME->getReceiverInterface()) {
      Selector Sel = ME->getSelector();
      
      // Find the callee definition within the same translation unit.
      Decl *D = nullptr;
      if (ME->isInstanceMessage())
        D = IDecl->lookupPrivateMethod(Sel);
      else
        D = IDecl->lookupPrivateClassMethod(Sel);
      if (D) {
        addCalledDecl(D);
        NumObjCCallEdges++;
      }
    }
  }

  void VisitChildren(Stmt *S) {
    for (Stmt *SubStmt : S->children())
      if (SubStmt)
        this->Visit(SubStmt);
  }
};

} // end anonymous namespace

void CallGraphCtu::addNodesForBlocks(DeclContext *D) {
  if (BlockDecl *BD = dyn_cast<BlockDecl>(D))
    addNodeForDecl(BD, true);

  for (auto *I : D->decls())
    if (auto *DC = dyn_cast<DeclContext>(I))
      addNodesForBlocks(DC);
}

CallGraphCtu::CallGraphCtu() {
  Root = getOrInsertNode(nullptr);
  std::string str=uniqueNameResolve(nullptr);
  FunctionNameMap[str]=Root;
  FunctionMap[nullptr]=Root;
}

CallGraphCtu::~CallGraphCtu() {}

bool CallGraphCtu::includeInGraph(const Decl *D) {
  assert(D);
  if (!D->hasBody())
    return false;

  if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(D)) {
    // We skip function template definitions, as their semantics is
    // only determined when they are instantiated.
    if (FD->isDependentContext())
      return false;

    IdentifierInfo *II = FD->getIdentifier();
    if (II && II->getName().startswith("__inline"))
      return false;
  }

  return true;
}

void CallGraphCtu::addNodeForDecl(Decl* D, bool IsGlobal) {
  assert(D);

  // Allocate a new node, mark it as root, and process it's calls.
  CallGraphNode *Node = getOrInsertNode(D);
  addDeclToExport(Node);
  // Process all the calls by this function as well.
  CGBuilder builder(this, Node,getPA());
  if (Stmt *Body = D->getBody())
    builder.Visit(Body);
}

std::string CallGraphCtu::uniqueNameResolve(Decl *F)
{
    std::string s="";
    if(!F)  return "none";
    FunctionDecl *FD=F->getAsFunction();
    if(!FD)
        return "none";
    s=FD->getNameInfo().getAsString();
    for(unsigned i=0;i<FD->getNumParams();++i)
    {
        std::string arg=FD->parameters()[i]->getType().getAsString();
        s=s+arg;
      }
   return s;
}

std::string CallGraphCtu::uniqueNameResolve(const Decl *F) const
{
    std::string s="";
    const FunctionDecl *FD=F->getAsFunction();
    if(!FD)
        return "none";
    s=FD->getNameInfo().getAsString();
    for(unsigned i=0;i<FD->getNumParams();++i)
    {
        std::string arg=FD->parameters()[i]->getType().getAsString();
        s=s+arg;
      }
   return s;
}


CallGraphNode *CallGraphCtu::getNode(const Decl *F) const {
  const std::string str=uniqueNameResolve(F);
    //std::string str="";
  FunctionNameMapTy::const_iterator I = FunctionNameMap.find(str);
  if (I == FunctionNameMap.end()) return nullptr;
  return I->second;
}


CallGraphNode *CallGraphCtu::getOrInsertNode(Decl *F) {
  if (F && !isa<ObjCMethodDecl>(F))
    F = F->getCanonicalDecl();
  std::string str=uniqueNameResolve(F);
  CallGraphNode *Node = FunctionNameMap[str];
  if (Node)
        return Node;
  Node=new CallGraphNode(F);
  // Make Root node a parent of all functions to make sure all are reachable.
  if (F){
      FunctionNameMap[str]=Node;
      FunctionMap[F]=Node;
      Root->addCallee(Node);
    }
  return Node;
}


void CallGraphCtu::finishGraphConstruction(){
    for(NodesCtuType::iterator it=NodesWithCtuDecl.begin();it!=NodesWithCtuDecl.end();it++)
     {
       CallGraphNode *parent,*child;
       Decl *childDecl;
       childDecl=it->first;
       parent=it->second;
       if(FunctionDecl *func=dyn_cast<FunctionDecl>(childDecl))
	 {
	   std::string Name=func->getNameInfo().getAsString();
	   std::pair <DeclNameType::iterator,DeclNameType::iterator> range;
	   range = ExportedDecl.equal_range(Name);
	   for (DeclNameType::iterator it1=range.first; it1!=range.second; ++it1)
	     {
	       child=it1->second; 
	       FunctionDecl *fdecl=dyn_cast<FunctionDecl>(child->getDecl());
	       if(equalFuncDecls(fdecl,func)){
		 parent->addCallee(child);
		 removeRootChild(child);
	       }
	     } 
	 }
     }
  }  


void CallGraphCtu::print(raw_ostream &OS) const {
  OS << " --- Call graph Dump --- \n";

  // We are going to print the graph in reverse post order, partially, to make
  // sure the output is deterministic.
  llvm::ReversePostOrderTraversal<const clang::CallGraphCtu*> RPOT(this);
  for (llvm::ReversePostOrderTraversal<const clang::CallGraphCtu*>::rpo_iterator
         I = RPOT.begin(), E = RPOT.end(); I != E; ++I) {
    const CallGraphNode *N = *I;

    OS << "  Function: ";
    if (N == Root)
      OS << "< root >";
    else
      N->print(OS);

    OS << " calls: ";
    for (CallGraphNode::const_iterator CI = N->begin(),
                                       CE = N->end(); CI != CE; ++CI) {
      assert(*CI != Root && "No one can call the root node.");
      (*CI)->print(OS);
      OS << " ";
    }
    OS << '\n';
  }
  OS.flush();
}

LLVM_DUMP_METHOD void CallGraphCtu::dump() const {
  print(llvm::errs());
}

void CallGraphCtu::viewGraph() const {
  llvm::ViewGraph(this, "CallGraphCtu");
}

void CallGraphNode::print(raw_ostream &os) const {
  if (const NamedDecl *ND = dyn_cast_or_null<NamedDecl>(FD))
      return ND->printName(os);
  os << "< >";
}

LLVM_DUMP_METHOD void CallGraphNode::dump() const {
  print(llvm::errs());
}

namespace llvm {

template <>
struct DOTGraphTraits<const CallGraphCtu*> : public DefaultDOTGraphTraits {

  DOTGraphTraits (bool isSimple=false) : DefaultDOTGraphTraits(isSimple) {}

  static std::string getNodeLabel(const CallGraphNode *Node,
                                  const CallGraphCtu *CG) {
    if (CG->getRoot() == Node) {
      return "< root >";
    }
      if(!Node) return "LnotFound";
    if (const NamedDecl *ND = dyn_cast_or_null<NamedDecl>(Node->getDecl()))
      return ND->getNameAsString();
    else
      return "< >";
  }

};
}

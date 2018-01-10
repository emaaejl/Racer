/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#ifndef LLVM_CLANG_CALLGRAPHANALYSIS_H
#define LLVM_CLANG_CALLGRAPHANALYSIS_H

#include "clang/AST/AST.h"
#include "clang/AST/Decl.h"
#include "libExt/CallGraphCtu.h"
#include <queue>
#include <set>
#include <map>

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;


class CGReachability{
private:
  CallGraph *cg;
  std::vector<std::string> startFuncs;
  std::multimap<std::string, std::pair<std::string, bool> > parentOf;
public:
  explicit CGReachability(CallGraph *g,std::vector<std::string> funcs) : startFuncs(funcs)
  {
    cg=g;
  }
 
  bool isnameOfNode(CallGraphNode *currNode,std::string ef)
  {
    if(!currNode) return false;
    clang::Decl *d=currNode->getDecl();
    if(!d) return false;
    std::string st;
    if (dyn_cast<FunctionDecl>(d)){
      st=d->getAsFunction()->getNameInfo().getAsString();
    }	
    else st="";
    if(st==ef) {return true;}
    else return false;
  }  
  std::string nameOfNode(CallGraphNode *currNode)
    {
      clang::Decl *d=currNode->getDecl();
      if(!d) return "Root";
      return d->getAsFunction()->getNameInfo().getAsString();
    }  

  CallGraphNode *getCallGraphNode(CallGraphNode *cgNode, std::string name, bool edgeL)
  {
    std::set<CallGraphNode *> visited;
    std::queue<CallGraphNode *> cgQ;
    cgQ.push(cgNode);
    while(!cgQ.empty())
      {
	CallGraphNode *currNode=cgQ.front();
	cgQ.pop();
	visited.insert(currNode);     
	if(isnameOfNode(currNode,name)) return currNode;
	for(auto it=currNode->begin();it!=currNode->end();it++)
	  {
	    if(visited.find(*it)==visited.end())
	      cgQ.push(*it);
	    parentOf.insert(std::make_pair(nameOfNode(*it), std::make_pair(nameOfNode(currNode),edgeL)));
	  }  
      }
    return NULL;
  }


    // At present it assumes that 
  bool hasReachablePath(std::string from, std::string to)
  {
    std::pair <std::multimap<std::string, std::pair<std::string, bool> >::iterator, std::multimap<std::string,std::pair<std::string, bool> >::iterator > range;
    std::vector<std::string> cgV;
    cgV.push_back(to);
    std::string temp=to;
    do{
      range = parentOf.equal_range(temp);
      std::multimap<std::string, std::pair<std::string, bool> >::iterator irange = range.first;
      for(; irange != range.second; ++irange)
	{
	  if(irange->second.second)
	    {
	      temp=irange->second.first;
	      cgV.push_back(temp);
	      break;
	    }
	}
      if(irange == range.second) return false;
    }while(temp!=from);
    return true;
  }    

  std::string reachableFrom(std::string to)
  {
    if(!cg) return "not_found";
    std::vector<std::string>::iterator it=startFuncs.begin();

    while(it!=startFuncs.end())
      {  
	parentOf.clear();
	CallGraphNode *startNode;
	llvm::errs()<<"CG 1\n";
	startNode=getCallGraphNode(cg->getRoot(),*it,0);
	if(!startNode) {it++; continue;}
	llvm::errs()<<"CG 2\n";
	if(getCallGraphNode(startNode, to, 1)){
	  llvm::errs()<<"CG 3\n";
	  if(hasReachablePath(*it, to))
	    return *it;
	}
	it++;
      }
    return "not_found";
  }  

};

#endif

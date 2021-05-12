/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "libExt/CompilerInstanceCtu.h"
#include "libExt/CallGraphCtu.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Sema/Sema.h"
#include "steengaardPAVisitor.h"
#include "symTabBuilder.h"
#include "raceAnalysis.h"
#include "headerDepAnalysis.h"
#include "flowSensitivePA.h"
#include "CGFrontendAction.h"
#include "commandOptions.h"
#include "eventHandler.h"
#include <memory>
#include <ctime>

#include "TimerWrapper.h"

using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

#define WITH_TIME_SAMPLING

RaceFinder *racer=new RaceFinder();
int debugLabel=0;
#ifdef WITH_TIME_SAMPLING
TimerWrapper CTU_action("CallGraph CTU Invocation", 3000);
#endif
class PointerAnalysis : public ASTConsumer {
private:
  SteengaardPAVisitor *visitorPA; 
  SymTabBuilderVisitor *visitorSymTab;
public:
    // override the constructor in order to pass CI
  explicit PointerAnalysis(CompilerInstance *CI,std::string file)
    : visitorPA(new SteengaardPAVisitor(CI,debugLabel,file)),visitorSymTab(new SymTabBuilderVisitor(CI,debugLabel))
  {
  }

  // override this to call our ExampleVisitor on the entire source file
  virtual void HandleTranslationUnit(ASTContext &Context) {
    
    visitorSymTab->TraverseDecl(Context.getTranslationUnitDecl());
    visitorSymTab->dumpSymTab();
    errs()<<"building initPA\n";
    visitorPA->initPA(visitorSymTab->getSymTab());
    errs()<<"Traverse Decl\n";
    visitorPA->TraverseDecl(Context.getTranslationUnitDecl());
    errs()<<"Store Global Pointer\n";
    visitorPA->storeGlobalPointers();
    errs()<<"show PA\n";
    visitorPA->showPAInfo();
    errs()<<"Show globals\n";
    visitorPA->getGvHandler()->showGlobals();
    }   
};

class FSPointerAnalysis : public ASTConsumer {
private:
  FSPAVisitor *visitorPA; 
  SymTabBuilderVisitor *visitorSymTab;
public:
    // override the constructor in order to pass CI
  explicit FSPointerAnalysis(CompilerInstance *CI,std::string file)
    : visitorPA(new FSPAVisitor(CI,debugLabel,file)),visitorSymTab(new SymTabBuilderVisitor(CI,debugLabel))
  {
  }

  virtual bool HandleTopLevelDecl(clang::DeclGroupRef DG) {
    errs()<<"first line\n";
    if (!DG.isSingleDecl()) {
      return true;
    }
    
    clang::Decl *D = DG.getSingleDecl();
     errs()<<"second line\n";
    const clang::FunctionDecl *FD = clang::dyn_cast<clang::FunctionDecl>(D);
    // Skip other functions
    if (!FD) {
      return true;
    }
    
    errs()<<"third checkpoint\n";
    visitorPA->analyze(D);
    
    // recursively visit each AST node in Decl "D"
    //        visitor->TraverseDecl(D); 
    
    return true;
  }
    
};



class RaceDetector : public ASTConsumer {
private:
  SteengaardPAVisitor *visitorPA; 
  SymTabBuilderVisitor *visitorSymTab;
public:
    // override the constructor in order to pass CI
  explicit RaceDetector(CompilerInstance *CI, std::string file)
    : visitorPA(new SteengaardPAVisitor(CI,debugLabel,file)),visitorSymTab(new SymTabBuilderVisitor(CI, debugLabel))
      {
      }

    // override this to call our ExampleVisitor on the entire source file
  virtual void HandleTranslationUnit(ASTContext &Context) {
    visitorSymTab->TraverseDecl(Context.getTranslationUnitDecl());
    visitorSymTab->dumpSymTab();
    
    visitorPA->initPA(visitorSymTab->getSymTab());
    
    visitorPA->TraverseDecl(Context.getTranslationUnitDecl());
    visitorPA->storeGlobalPointers();
    visitorPA->showPAInfo();   
    //visitorPA->showVarReadWriteLoc(); 

    racer->createNewTUAnalysis(visitorPA->getGvHandler());
    }   
};

class PAFrontendAction : public ASTFrontendAction {
 public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)   {
    std::cout<<"Pointer Analysis of "<<file.str()<<"\n";
    return make_unique<PointerAnalysis>(&CI,file.str()); // pass CI pointer to ASTConsumer
   }
};

class PAFlowSensitiveFrontendAction : public ASTFrontendAction {
 public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)   {
    std::cout<<"Pointer Analysis (Flow Sensitive) of "<<file.str()<<"\n";
    return make_unique<FSPointerAnalysis>(&CI,file.str()); // pass CI pointer to ASTConsumer
   }
};


class RacerFrontendAction : public ASTFrontendAction {
 public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)   {
     if(debugLabel>1) llvm::outs()<<"Analyzing File: "<<file.str()<<"\n";
     return make_unique<RaceDetector>(&CI,file.str()); // pass CI pointer to ASTConsumer
   }
};

class SymbTabAction : public ASTFrontendAction {
 public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)    {
      return make_unique<SymTabBuilder>(&CI,debugLabel);
     }
};


class CGFrontendFactory : public clang::tooling::FrontendActionFactory {

    CallGraph &_cg;
    std::vector<std::unique_ptr<clang::CompilerInstanceCtu>> *astL;
public:
  CGFrontendFactory (CallGraph &cg, std::vector<std::unique_ptr<clang::CompilerInstanceCtu>> &ast)
    : _cg (cg) { astL=&ast;}

    virtual std::unique_ptr<clang::FrontendAction> create () {
      CGFrontendAction* ptr = new CGFrontendAction(_cg);
      std::unique_ptr<CGFrontendAction> u_ptr(ptr);
      return u_ptr;
    }

    bool runInvocation(
    std::shared_ptr<CompilerInvocation> Invocation, FileManager *Files,
    std::shared_ptr<PCHContainerOperations> PCHContainerOps,
    DiagnosticConsumer *DiagConsumer) {
      // Create a compiler instance to handle the actual work.
      std::unique_ptr<clang::CompilerInstanceCtu> Compiler=make_unique<clang::CompilerInstanceCtu>   (std::move(PCHContainerOps));
      Compiler->setInvocation(std::move(Invocation));
      Compiler->setFileManager(Files);

      // The FrontendAction can have lifetime requirements for Compiler or its
      // members, and we need to ensure it's deleted earlier than Compiler. So we
      // pass it to an std::unique_ptr declared after the Compiler variable.
      
      std::unique_ptr<FrontendAction> ScopedToolAction(create());
      // Create the compiler's actual diagnostics engine.
      Compiler->createDiagnostics(DiagConsumer, /*ShouldOwnClient=*/false);
      if (!Compiler->hasDiagnostics())
	return false;
      Compiler->createSourceManager(*Files);
      
      Compiler->setFrontendAction(ScopedToolAction.get());
#ifdef WITH_TIME_SAMPLING
      CTU_action.startTimers();
#endif
      const bool Success = Compiler->ExecuteActionCtu(*ScopedToolAction);
#ifdef WITH_TIME_SAMPLING
      CTU_action.stopTimers();
#endif
      astL->push_back(std::move(Compiler));
      Files->clearStatCache();
      return Success;
    }
};
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WITH_TIME_SAMPLING
/*
 * https://stackoverflow.com/questions/1558402/memory-usage-of-current-process-in-c
 * Measures the current (and peak) resident and virtual memories
 * usage of your linux C process, in kB
 */
void getMemory(
    int* currRealMem, int* peakRealMem,
    int* currVirtMem, int* peakVirtMem) {

    // stores each word in status file
    char buffer[1024] = "";

    // linux file contains this-process info
    FILE* file = fopen("/proc/self/status", "r");

    // read the entire file
    while (fscanf(file, " %1023s", buffer) == 1) {

        if (strcmp(buffer, "VmRSS:") == 0) {
            fscanf(file, " %d", currRealMem);
        }
        if (strcmp(buffer, "VmHWM:") == 0) {
            fscanf(file, " %d", peakRealMem);
        }
        if (strcmp(buffer, "VmSize:") == 0) {
            fscanf(file, " %d", currVirtMem);
        }
        if (strcmp(buffer, "VmPeak:") == 0) {
            fscanf(file, " %d", peakVirtMem);
        }
    }
    fclose(file);
}
#endif
int main(int argc, const char **argv) {
    // parse the command-line args passed to your code 
    CommonOptionsParser op(argc, argv, RacerOptCat);        
    
    // create a new Clang Tool instance (a LibTooling environment)
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());	
    // run the Clang Tool, creating a new FrontendAction (explained below)
    int result;
    if(DebugLevel==O1) debugLabel=1;
    else if(DebugLevel==O2) debugLabel=2;
    else if(DebugLevel==O3) debugLabel=3;
  /*  if(Event.c_str()){
      std::ofstream EventArg(Event.c_str(),std::ios::app);
      if (EventArg.good())
	{
	  
	  std::clock_t c_start = std::clock();

	  ClangTool ToolCG(op.getCompilations(), op.getSourcePathList());
	  CallGraph cg;
	  std::vector<std::unique_ptr<clang::CompilerInstanceCtu> > vectCI;
	  CGFrontendFactory cgFact(cg,vectCI);
	  ToolCG.run(&cgFact);
	  cg.finishGraphConstruction();
     
	  std::clock_t c_end = std::clock();

	  llvm::outs()<<"***Call Graph construction completes in: "<<(c_end-c_start)/CLOCKS_PER_SEC<<" ms\n";

	  ToolCG.getFiles().PrintStats();

	  c_start = std::clock();
	  EventRecorder Printer(EventArg,&cg,StartFuncsForEvents);
	  MatchFinder Finder;
	  
	  Finder.addMatcher(Trigger, &Printer);
	  Tool.run(newFrontendActionFactory(&Finder).get());

	  c_end = std::clock();

	  llvm::outs()<<"***Search for events completed in: "<<(c_end-c_start)/CLOCKS_PER_SEC<<" ms\n";
	}  
    }*/
    if(PAFlow) result = Tool.run(newFrontendActionFactory<PAFlowSensitiveFrontendAction>().get());
    if(Symb) {
      result = Tool.run(newFrontendActionFactory<SymbTabAction>().get());
    }
    if(PA)
      {
      result = Tool.run(newFrontendActionFactory<PAFrontendAction>().get());
      }
    if(HA) {
       RepoGraph repo(debugLabel);
       IncAnalFrontendActionFactory act (repo);
       Tool.run(&act);
       std::vector<std::string> hFiles=repo.getHeaderFiles();
       repo.printHeaderList();
       //ClangTool Tool1(op.getCompilations(), hFiles);
       //result = Tool1.run(newFrontendActionFactory<PAFrontendAction>().get());
    }
    if(CG){
      CallGraph cg;
      std::vector<std::unique_ptr<clang::CompilerInstanceCtu> > vectCI;
      CGFrontendFactory cgFact(cg,vectCI);
#ifdef WITH_TIME_SAMPLING
      TimerWrapper cg_timer = TimerWrapper("Call Graph Generation", 8);
      int currRealMem, currVirtMem;
      int peakRealMem, peakVirtMem;
      cg_timer.startTimers();
#endif
      result=Tool.run(&cgFact);
      cg.finishGraphConstruction();
#ifdef WITH_TIME_SAMPLING
      cg_timer.stopTimers();
      cg_timer.printInfo(std::cout);
      CTU_action.printInfo(std::cout);
      getMemory(&currRealMem, &peakRealMem, &currVirtMem, &peakVirtMem);
      std::cout << "Peak RAM (kb): " << peakRealMem << " Peak Virtual Memory (kb) " << peakVirtMem << "\n";
#endif
      if(debugLabel > 1)
        cg.viewGraph();
      for(auto it=vectCI.begin();it!=vectCI.end();it++)
      {
        FrontendAction *fact=(*it)->getFrontendAction();
        if(CGFrontendAction *cgFrontend=static_cast<CGFrontendAction *>(fact)) 
          cgFrontend->EndFrontendAction();
        it->get()->EndCompilerActionOnSourceFile();
      }
    }
    if(RA){
      if(op.getSourcePathList().size()!=2) 
      {
	errs()<<"Command Format Error, exactly two sources are required to run the command. See, e.g. racer --help\n";
	return 0;	  
      }

      // header Analysis
      /*      std::string Task1=op.getSourcePathList()[0];
      std::string Task2=op.getSourcePathList()[1];
      RepoGraph repo1(debugLabel),repo2(debugLabel);
      IncAnalFrontendActionFactory act1 (repo1);
      IncAnalFrontendActionFactory act2 (repo2);
      std::vector<std::string> Vect1,Vect2;
      Vect1.push_back(Task1);
      Vect2.push_back(Task2);
      ClangTool ToolHa1(op.getCompilations(),Vect1);
      ClangTool ToolHa2(op.getCompilations(),Vect2);
      ToolHa1.run(&act1);
      std::vector<std::string> HFileList1=repo1.getHeaderFiles();
      result=ToolHa2.run(&act2);
      std::vector<std::string> HFileList2=repo2.getHeaderFiles();
      std::cout<<"Task: "<<Task1<<"\n";
      repo1.printHeaderList();
      std::cout<<"Task: "<<Task2<<"\n";
      repo2.printHeaderList();
      */
      // Call Graph Construction
      
      ClangTool ToolCG(op.getCompilations(), op.getSourcePathList());
      CallGraph cg;
      std::vector<std::unique_ptr<clang::CompilerInstanceCtu> > vectCI;
      CGFrontendFactory cgFact(cg,vectCI);
      ToolCG.run(&cgFact);
      cg.finishGraphConstruction();
      llvm::errs()<<"CG construction finished\n";
      if(debugLabel>2)
	  cg.viewGraph();

      // Race Detection
      /*  std::vector< std::string > sop=op.getSourcePathList();
      for(std::vector< std::string >::iterator i=sop.begin();i!=sop.end();i++)
	llvm::outs()<<"OP Source Paths: "<<*i<<"\n";
      */

      /*llvm::ArrayRef< std::string > spaths=Tool.getSourcePaths(); 
      for(llvm::ArrayRef< std::string >::iterator it=spaths.begin();it!=spaths.end();it++)
	llvm::outs()<<"Source Paths: "<<*it<<"\n";
      */
      result = Tool.run(newFrontendActionFactory<RacerFrontendAction>().get());
      llvm::errs()<<"Race tool run finished\n";
      racer->setCallGraph(&cg);
      std::ofstream Method1(FUNC1.c_str()), Method2(FUNC2.c_str());
      if (Method1.good() && Method2.good()) 
	racer->setTaskStartPoint(FUNC1.c_str(),FUNC2.c_str());
      racer->extractPossibleRaces();
      
      
      // Finish compiler instances (BUG: Causes infinite loop somewhere)
      /*for(auto it=vectCI.begin();it!=vectCI.end();it++)
      {
        FrontendAction *fact=(*it)->getFrontendAction();
        if(CGFrontendAction *cgFrontend=static_cast<CGFrontendAction *>(fact)) 
          cgFrontend->EndFrontendAction();
        it->get()->EndCompilerActionOnSourceFile();
      }*/
      
    }

   return result;
}

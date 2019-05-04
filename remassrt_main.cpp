/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Sema/Sema.h"
#include "clang/Basic/LangOptions.h"
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Analysis/CFG.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Analysis/Analyses/Dominators.h"
#include "clang/Analysis/AnalysisDeclContext.h"

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"

#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/Host.h"
#include "clang/AST/Stmt.h"
#include "llvm/ADT/StringRef.h"


#include <memory>
#include <ctime>
#include <sstream>
#include <string>
#include "analyse_cfg.h"
#include <algorithm>
#include <chrono>

using namespace std;
using namespace std::chrono;

using namespace llvm;
using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;

#include "commandOptions.h"



class  PrintStmtextended:public PrinterHelper
{
public:
    ASTContext *context;
    PrintStmtextended(ASTContext &c):context(&c){}
    bool handledStmt (Stmt *E, raw_ostream &OS)
    {
        E->dumpPretty(*context);
        return true;
    }
};

void SplitFilename (const string& str, string &path, string &filename)
{
    size_t found;
    found=str.find_last_of("/\\");
    path=str.substr(0,found);
    filename=str.substr(found+1);
}

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
    MyASTVisitor(Rewriter &R, llvm::raw_fd_ostream &assF) : TheRewriter(R) {first_FDecl=false;isInTU=false;
    assertFile=&assF;
        
    }
    
    bool isAssert(Stmt *s,string &strExpr)
    {
     if(!isa<ConditionalOperator>(s)) return false;
     // the macro expansion of assert is a conditional expression
     ConditionalOperator *COp=static_cast<ConditionalOperator *>(s);
     Expr *Cond=COp->getCond()->IgnoreImplicit();
     Expr *TE=COp->getTrueExpr();
     Expr *FE=COp->getFalseExpr();
     if(!Cond && !TE && !FE) return false;
        
     CallExpr *CExpr=dyn_cast<CallExpr>(Cond);
     if(!CExpr) return false;
     if(!(CExpr->getDirectCallee()->getNameInfo().getAsString()=="__builtin_expect")) return false;
     /*
     Expr *argExp=CExpr->getArg(0)->IgnoreImplicit();
     if(!argExp) return false;
     UnaryOperator *UOp=dyn_cast<UnaryOperator>(argExp);
     if(!UOp) return false;
     Expr *exp=UOp->getSubExpr()->IgnoreImplicit();
     if(!exp) return false;
      */
     
    CallExpr *CExpr1=dyn_cast<CallExpr>(TE);
    if(!TE) return false;
    if(!(CExpr1->getDirectCallee()->getNameInfo().getAsString()=="__assert_rtn")) return false;
        
    Expr *argExp1=CExpr1->getArg(CExpr1->getNumArgs()-1)->IgnoreImplicit();
    if(!argExp1) return false;
    clang::StringLiteral *strLt=dyn_cast<clang::StringLiteral>(argExp1);
    if(!strLt) return false;
    llvm::errs()<<"Assert Expr: "<<strLt->getString().str();
    strExpr=strLt->getString().str();
    unsigned lineNo=TheRewriter.getSourceMgr().getExpansionLineNumber(s->getBeginLoc());
        //s->getBeginLoc().printToString(TheRewriter.getSourceMgr())
    *assertFile<<"ASSERT("<<fname<<","<<lineNo<<","<<strExpr<<").\n";
    return true;
        //return false;
    }
    
    bool VisitStmt(Stmt *s) {
        string str;
        if(isAssert(s,str))
        {   const char *data="dyn_assert_var = 1";
            StringRef strRef=StringRef(data);
            
            SourceLocation startLoc = TheRewriter.getSourceMgr().getFileLoc(s->getBeginLoc());
            SourceLocation endLoc = s->getEndLoc();
           
            if( endLoc.isMacroID() ){
            endLoc=TheRewriter.getSourceMgr().getImmediateExpansionRange(endLoc).getEnd();
            endLoc.dump(TheRewriter.getSourceMgr());
            }
            SourceRange expandedLoc( startLoc, endLoc );
           // s->getSourceRange().dump(TheRewriter.getSourceMgr());
            TheRewriter.ReplaceText(expandedLoc, strRef);
        }
        
        // Only care about If statements.
       /*   if (isa<IfStmt>(s)) {
            IfStmt *IfStatement = cast<IfStmt>(s);
            Stmt *Then = IfStatement->getThen();
            
            TheRewriter.InsertText(Then->getBeginLoc(), "// the 'if' part\n", true,
                                   true);
            
            Stmt *Else = IfStatement->getElse();
            if (Else)
                TheRewriter.InsertText(Else->getBeginLoc(), "// the 'else' part\n",
                                       true, true);
        }  */
        
        return true;
    }
    
    bool VisitFunctionDecl(FunctionDecl *f) {
        // Only function definitions (with bodies), not declarations.
         SourceLocation sl=f->getSourceRange().getBegin();
        if(TheRewriter.getSourceMgr().isInMainFile (sl) && !first_FDecl)
        {
            first_FDecl=true;
           
            const char *data="unsigned dyn_assert_var = 0;\n";
            StringRef strRef=StringRef(data);
            TheRewriter.InsertTextBefore(sl,strRef);
        }
        if(TheRewriter.getSourceMgr().isInMainFile (sl))
        {
            isInTU=true;
            fname=f->getName().str();
        }
        return true;
    }
    
private:
    Rewriter &TheRewriter;
    bool first_FDecl;
    bool isInTU;
    std::string fname;
    llvm::raw_fd_ostream *assertFile;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
    MyASTConsumer(Rewriter &R, llvm::raw_fd_ostream &assF) : Visitor(R,assF) {}
    
    // Override the method that gets called for each parsed top-level
    // declaration.
    virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
        for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
            // Traverse the declaration using our AST visitor.
            Visitor.TraverseDecl(*b);
        return true;
    }
    
private:
    MyASTVisitor Visitor;
};

int main(int argc,  const char **argv) {
    
    if (argc != 2) {
        llvm::errs() << "Usage: remassert <filename>\n";
        return 1;
    }
    
    // CompilerInstance will hold the instance of the Clang compiler for us,
    // managing the various objects needed to run the compiler.
    CompilerInstance TheCompInst;
    TheCompInst.createDiagnostics();
    
    LangOptions &lo = TheCompInst.getLangOpts();
    lo.CPlusPlus = 1;
    
    // Initialize target info with the default triple for our platform.
    auto TO = std::make_shared<clang::TargetOptions>();
    TO->Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *TI =
    TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
    TheCompInst.setTarget(TI);
    
    TheCompInst.createFileManager();
    FileManager &FileMgr = TheCompInst.getFileManager();
    TheCompInst.createSourceManager(FileMgr);
    SourceManager &SourceMgr = TheCompInst.getSourceManager();
    TheCompInst.createPreprocessor(TU_Complete);
    TheCompInst.createASTContext();
    // handle macros
    TheCompInst.getPreprocessor().getBuiltinInfo().initializeBuiltins(TheCompInst.getPreprocessor().getIdentifierTable(),
                                                     TheCompInst.getPreprocessor().getLangOpts());
    
    // A Rewriter helps us manage the code rewriting task.
    Rewriter TheRewriter;
    TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());
    
    // Set the main file handled by the source manager to the input file.
    const FileEntry *FileIn = FileMgr.getFile(argv[1]);
    SourceMgr.setMainFileID(
                            SourceMgr.createFileID(FileIn, SourceLocation(), SrcMgr::C_User));
    TheCompInst.getDiagnosticClient().BeginSourceFile(
                                                      TheCompInst.getLangOpts(), &TheCompInst.getPreprocessor());
    // std::string fpath = realpath (FileIn->getName().str().c_str(), NULL);
    string path, fname,fpname,assert_fname;
    SplitFilename(FileIn->getName().str().c_str(),path,fname);
    fpname=path+"/noassert_"+fname;
    assert_fname=path+"/assert.txt";
    
    //llvm::errs()<<"Path: "<<path<<"\n";
    //llvm::errs()<<"File: "<<new_fname<<"\n";
    std::error_code error_code;
    llvm::raw_fd_ostream outFile(fpname, error_code, llvm::sys::fs::F_None);
    llvm::raw_fd_ostream assertFile(assert_fname, error_code, llvm::sys::fs::F_None);
    
    // Create an AST consumer instance which is going to get called by
    // ParseAST.
    MyASTConsumer TheConsumer(TheRewriter,assertFile);
    
    // Parse the file to AST, registering our consumer as the AST consumer.
    ParseAST(TheCompInst.getPreprocessor(), &TheConsumer,
             TheCompInst.getASTContext());
    
    
    // At this point the rewriter's buffer should be full with the rewritten
    // file contents.
    const RewriteBuffer *RewriteBuf = TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
    llvm::outs() << string(RewriteBuf->begin(), RewriteBuf->end());
    if(RewriteBuf)
        TheRewriter.getEditBuffer(SourceMgr.getMainFileID()).write(outFile);
    else
        llvm::errs()<<"\nNo Assert found, original retains!!!\n";
    outFile.close();
    assertFile.close();
    
    return 0;
    
}

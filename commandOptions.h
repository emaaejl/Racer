#ifndef LLVM_CLANG_COMMANDOPTIONS_H
#define LLVM_CLANG_COMMANDOPTIONS_H

#include "clang/Tooling/CommonOptionsParser.h"

static llvm::cl::OptionCategory RacerOptCat("Static Analysis Options");
static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\nMore help text...");

static llvm::cl::opt<std::string> Event("e", llvm::cl::desc("Stores activity event in filename"), llvm::cl::value_desc("filename"),llvm::cl::cat(RacerOptCat));
static llvm::cl::list<std::string> StartFuncsForEvents("ef", llvm::cl::desc("possible name of functions in which events may occur"),llvm::cl::multi_val(llvm::cl::ZeroOrMore),llvm::cl::cat(RacerOptCat));

//static cl::opt<bool> Test("test",cl::desc("Test different clang options"), cl::cat(RacerOptCat));
static llvm::cl::opt<bool> Symb("sym",llvm::cl::desc("Build and dump the Symbol Table"), llvm::cl::cat(RacerOptCat));
static llvm::cl::opt<bool> PA("pa",llvm::cl::desc("Show pointer analysis info"), cl::cat(RacerOptCat));
static cl::opt<bool> PAFlow("pafs",cl::desc("Show flow sensitive pointer analysis info"), cl::cat(RacerOptCat));
static cl::opt<bool> HA("ha",cl::desc("Show header analysis info"), cl::cat(RacerOptCat));
static cl::opt<bool> CG("cg",cl::desc("Call Graph Info"), cl::cat(RacerOptCat));
static cl::opt<std::string> FUNC1("m1",cl::desc("Initial method from which execution starts"), cl::value_desc("function name"),cl::cat(RacerOptCat));
static cl::opt<std::string> FUNC2("m2",cl::desc("Initial method from which execution starts"), cl::value_desc("function name"),cl::cat(RacerOptCat));

static cl::opt<bool> RA("ra",cl::desc("Data race analysis"), cl::cat(RacerOptCat));

enum DLevel {
  O, O1, O2, O3
};
cl::opt<DLevel> DebugLevel("dl", cl::desc("Choose Debug level:"),
  cl::values(
	     clEnumValN(O, "none", "No debugging"),
	     clEnumVal(O1, "Minimal debug info"),
	     clEnumVal(O2, "Expected debug info"),
	     clEnumVal(O3, "Extended debug info")), cl::cat(RacerOptCat));

#endif

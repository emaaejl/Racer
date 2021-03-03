/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#ifndef STEENSGAARDPA_H_
#define STEENSGAARDPA_H_

#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include "symtab.h"

class CSteensgaardPAType;
class CSteensgaardPARefType;
class CSteensgaardPALambaType;
class CSteensgaardPAForwType;
class CSteensgaardPABotType;
//class CSteensgaardAnalysisBuilder;
//class CSymTabBase;
//class CGenericProgram;

/** \class CSteensgaardPA
   Performing the pointer analysis. The implementation follows in a high degree
   the code in the Steensgard paper "Points-to Analysis in Almost Linear Time".
   Some inspiration has also been taken from the WPO Steensgard implementetion.
   The analysis assumes that all identifiers to be analyzed are represented by
   unique numbers.
   We use a t_vector for associating each variable with a type.
   The type can be a ref tuple, a lambda expression, a forward reference
   or bot. We use a pending vector for keeping track of which variables
   that waits to be unified with a given variable if the variable gets
   a non-bot type.
*/
class CSteensgaardPA
{
public:

   // For creating the analysis data structures. The vars data structure
   // is the set of variables, where each variable is identified by an
   // long integer. The funcs data structure is a representation of each
   // function as an long integer (idenitifying the function name) and two
   // sets of long integers (identifying its formal in and out argument
   // parameters), where the formal parameters should already be part of
   // the vars set.
  CSteensgaardPA();
  //   CSteensgaardPA(std::auto_ptr<CSteensgaardAnalysisBuilder> builder, CGenericProgram *ast, const CSymTabBase *symtab);
   // If we want to add an extra set of x := &y gotten from abstract annotations.
  //  CSteensgaardPA(std::auto_ptr<CSteensgaardAnalysisBuilder> builder, CGenericProgram *ast,
  //              const CSymTabBase *symtab, const std::set<std::pair<long int,std::set<long int> > > *def_addresses_pairs);
   virtual ~CSteensgaardPA();

   // -------------------------------------------------------
   // Functions for processing statements. Corresponds to the
   // functions given in the Steensgard paper.
   // -------------------------------------------------------

   // Updates pointer analysis with e1 := e2 stmt
   void ProcessAssignStmt(long int e1, long int e2);

   // Updates pointer analysis with e1 := &e2 stmt
   void ProcessAssignAddrStmt(long int e1, long int e2);

   // Updates pointer analysis with e1 := *e2 stmt
   void ProcessAssignFromIndStmt(long int e1, long int e2);

   // Updates pointer analysis with *x := y stmt
   void ProcessAssignToIndStmt(long int x, long int y);

   // Updates pointer analysis with x := alloc(...) stmt
   void ProcessAssignAllocStmt(long int x);

   // Updates pointer analysis with ret1 .. retm = f(arg1, arg2,
   // ... argn) (i.e., a function call). The a_args and the a_rets are the
   // actual in/out parameters.
   void ProcessAssignCallStmt(const std::vector<unsigned long>& a_rets, long int f, const std::vector<unsigned long>& a_args);

   void ProcessAssignFunPtrAddrStmt(long int e1, long int e2);
   // To create a new reference variable. Returns the long int to identify the variable.
   long int CreateNewVar(void);

   /// Prints all points-to-sets using the identifier names as found in \a symbtab
   void PrintAsPointsToSets(const SymTab<SymBase> *symbTab, std::ostream &o=std::cout) const;
   // can we make it virtual or something else?
   void PrintAsPointsToSets(std::ostream &o=std::cout) const;
   // For debug printouts
   void PrintInternals(std::ostream &o=std::cout) const;

   /// \return A set of variables pointed to by \a id or an empty set
   /// if \a id is not a pointer. The \a id as well as the returned
   /// variables are represented by their globally unique keys.
   const std::set<unsigned long> &GetPointsToVarsSet(unsigned long id) const;

   std::set<unsigned long> getPtsToFuncsWithPartialPA(unsigned long var);
   // To get the variables pointed to in a set
   void GetPointsToVars(unsigned long int var, std::set<unsigned long> * vars_pointed_to) const;
   // Returns true if there are some variables pointed to by id.
   bool HasPointsToVars(unsigned long id) const;

   void GetPointedAtVar(unsigned long id, std::set<unsigned long> * vars_pointed_at) const;
   /// \return A set of functions pointed to by \a id or an empty set
   /// if \a id is not a pointer. The \a id as well as the returned
   /// functions are represented by their globally unique keys.
   const std::set<unsigned long> &GetPointsToFuncsSet(unsigned long id) const;
   // To get the functions pointed to in a set
   void GetPointsToFuncs(unsigned long int var, std::set<unsigned long> * funcs_pointed_to) const;
   // Returns true if there are some funcs pointed to by id.
   bool HasPointsToFuncs(unsigned long id) const;  

   /// \return A set of labels pointed to (not including function
   /// labels) by \a id or an empty set if \a id is not a pointer. The
   /// \a id as well as the returned labels are represented by their
   /// globally unique keys.
   const std::set<unsigned long> &GetPointsToLabelsSet(unsigned long id) const;
   // To get the labels pointed to in a set
   void GetPointsToLabels(unsigned long int var, std::set<unsigned long> * labels_pointed_to) const;
   // Returns true if there are some labels pointed to by id.
   bool HasPointsToLabels(unsigned long id) const;

   /// To get the symbol table used when creating the point-to sets (needed
   /// by CSteensgaardAnalysisBuilder to get the type of a certain key).
   //const CSymTabBase * GetSymbolTable() const { return _symtab; }

   // Creates the final mapping used by the interface
   void BuildVarToFuncsPointToSets();
   void BuildVarToVarsAndVarToLabelsPointToSets();

   void initPASolver(std::set<unsigned long> &vars, std::set<FuncSignature *> &funcs);
private:

   // Fucntion which does the actual pointer analysis
   //   void PerformPA(std::auto_ptr <CSteensgaardAnalysisBuilder> builder, CGenericProgram *ast, const CSymTabBase *symtab);

   // Prints a mapping from variable to a set of entities using the real names eventually
   // found in symbtab
    void PrintMapToSet(const std::map<unsigned long, std::set<unsigned long> > &x, std::ostream &o, const SymTab<SymBase> *symbTab=NULL) const;
    // void PrintMapToSet(const std::map<unsigned long, std::set<unsigned long> > &x, std::ostream &o) const;
   // To get the type of the argument variable. Also short cuts
   // any forward reference chain longer than 1.
   CSteensgaardPAType * gettype(long int e);

   // To set the type of the argument variable. Will delete the
   // previous type associated with the variable.
   void settype(long int e, CSteensgaardPAType * t);

   // To follow all forward references until we find the variable index
   // going to a ref(x,y), lambda(args)(rets) or bot type. Will also
   // shortcut all forward reference chains longer than 1.
   long int deref(long int e);

   // Conditional join between two variables point-to sets. If the
   // second one is bot the first one will be inserted into its pending
   // set.
   void cjoin(long int e1, long int e2);

   // Unconditional join between two variables point-to sets.
   void join(long int e1, long int e2);

   void joinWithoutUnification(long int e1, long int e2);


   // Unconditional unify of two types.
   void unify(CSteensgaardPAType * t1, CSteensgaardPAType * t2);

   // Conditional unify of two types.
   void cunify(CSteensgaardPAType * t1, CSteensgaardPAType * t2);

   // -------------------------------------------------------
   // Internal data structures
   // -------------------------------------------------------

   // A pointer to the symbol table used in the pointer analysis
   //  const CSymTabBase * _symtab;

   // To each variable we associate a type holding what the variable
   // can be pointing to.
   std::vector< CSteensgaardPAType * > _type_vector;

   // To each variable we have a set of variables pending for this
   // variable to be updated. Both the index variable and the set of
   // pending variables are represented by ints (global keys).
   std::vector< std::set<unsigned long> > _pending_vector;

   std::set<unsigned long> _vars;
   std::set<unsigned long> _temp_vars;
   std::set<unsigned long> _funcs;
   std::set<unsigned long> _labels;
   std::set<unsigned long> _vars_and_funcs;
   std::set<unsigned long> _vars_labels_and_temp_vars;
   std::set<unsigned long> _vars_labels_temp_vars_and_funcs;
   std::map< unsigned long, std::set< unsigned long > > _var_to_vars;
   std::map< unsigned long, std::set< unsigned long > > _var_to_labels;
   std::map< unsigned long, std::set< unsigned long > > _var_to_funcs;
};

std::ostream & operator << (std::ostream &o, CSteensgaardPA &r);

// =======================================================
// CSteensgaardPA -
// A class repreesenting a Steensgard pointer analysis type.
// The type could be either a reference, a lambda expression,
// or a forward reference. BOT values are represented as the
// variable gkey is BOT, i.e. we have no BOT type class.
// =======================================================
// Parent class
class CSteensgaardPAType
{
public:
   // To delete the type. Since it has virtual undefined
   // functions it cannot be created (only its sublasses).
   virtual ~CSteensgaardPAType(void) {}

   // To get the type. Overridden by subclasses.
   virtual bool IsRef() {return false;}
   virtual bool IsLambda() {return false;}
   virtual bool IsForw() {return false;}
   virtual bool IsBot() {return false;}

   virtual CSteensgaardPAType * Copy() = 0;

   void Print(void) {Print(&std::cout);};
   virtual void Print(std::ostream * o) = 0;

private:
   void print_var(std::ostream *o, long int v);
};

std::ostream & operator << (std::ostream &o, CSteensgaardPAType &t);

// -------------------------------------------------------
// CSteensgaardPARefType -
// So called ref type. Is a tuple with two variables. Inherents
// from parent. We represent bottom vars and funcs by assigning the
// long integer BOT to the argument values.
// -------------------------------------------------------
class CSteensgaardPARefType : public CSteensgaardPAType
{
public:
   CSteensgaardPARefType(long int var, long int func);

   bool IsRef() {return true;};

   long int Var() {return _var;};
   long int Func() {return _func;};
   CSteensgaardPAType * Copy();
   void Print(std::ostream * o);

private:
   long int _var;
   long int _func;

}; // end CSteensgaardPARefType

// -------------------------------------------------------
// CSteensgaardPALambaType -
// A function with a number of arguments and a number of return arguments
// -------------------------------------------------------
class CSteensgaardPALambdaType : public CSteensgaardPAType
{
public:
   // To create the type.
   CSteensgaardPALambdaType(const std::vector<unsigned long>& args, const std::vector<unsigned long>& rets);

   bool IsLambda() {return true;}
   const std::vector<unsigned long>& Args() {return _args;};
   const std::vector<unsigned long>& Rets() {return _rets;};

   // For copying the type
   CSteensgaardPAType * Copy();

   // To print the type
   void Print(std::ostream * o);

protected:
   std::vector<unsigned long> _args;
   std::vector<unsigned long> _rets;

}; // end CSteensgaardPALambdaType


// -------------------------------------------------------
// CSteensgaardPAForwType -
// Holds a forward reference to another variable.
// -------------------------------------------------------
// Forward reference to another CSteensgaardPAType
class CSteensgaardPAForwType : public CSteensgaardPAType
{
public:
   CSteensgaardPAForwType(long int forward_var);
   bool IsForw() {return true;}
   long int ForwVar() {return _forward_var;}
   void SetForwVar(long int new_forward_var) {_forward_var = new_forward_var;}
   CSteensgaardPAType * Copy();
   void Print(std::ostream * o);

protected:
   long int _forward_var;
};

// -------------------------------------------------------
// CSteensgaardPABotType -
// Holds a bottom type
// -------------------------------------------------------
class CSteensgaardPABotType : public CSteensgaardPAType
{
public:
   bool IsBot() {return true;}
   CSteensgaardPAType * Copy();
   void Print(std::ostream * o);
};

#endif

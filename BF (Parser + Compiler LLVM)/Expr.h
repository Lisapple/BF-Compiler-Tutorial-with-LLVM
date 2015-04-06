#ifndef EXPR_H
#define EXPR_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include <vector>

using namespace llvm;

class Expr // Abstract class
{
public:
  virtual void CodeGen(Module *M, IRBuilder<> &B) = 0;
  virtual void DebugDescription(int level) = 0;
};

// @TODO: Create abstract 'Generator' class

class ShiftExpr : public Expr
{
protected:
  int _step;
public:
  ShiftExpr(int step) : _step(step) { }
  void CodeGen(Module *M, IRBuilder<> &B);
  void DebugDescription(int level);
};

class IncrementExpr : public Expr
{
protected:
  int _increment;
public:
  IncrementExpr(int increment) : _increment(increment) { }
  void CodeGen(Module *M, IRBuilder<> &B);
  void DebugDescription(int level);
};

class InputExpr : public Expr
{
public:
  InputExpr() { }
  void CodeGen(Module *M, IRBuilder<> &B);
  void DebugDescription(int level);
};

class OutputExpr : public Expr
{
public:
  OutputExpr() { }
  void CodeGen(Module *M, IRBuilder<> &B);
  void DebugDescription(int level);
};

class LoopExpr : public Expr
{
protected:
  std::vector<Expr *> _exprs;
public:
  LoopExpr(std::vector<Expr *> &exprs) : _exprs(exprs) { }
  void CodeGen(Module *M, IRBuilder<> &B);
  void DebugDescription(int level);
};

#endif // EXPR_H
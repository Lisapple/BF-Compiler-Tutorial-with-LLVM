#ifndef EXPR_H
#define EXPR_H

#include <vector>

class Expr
{
public:
  virtual void CodeGen() = 0;
  virtual void DebugDescription(int level) = 0;
};

class ShiftExpr : public Expr
{
protected:
  int _step;
public:
  ShiftExpr(int step) : _step(step) { }
  void CodeGen();
  void DebugDescription(int level);
};

class IncrementExpr : public Expr
{
protected:
  int _increment;
public:
  IncrementExpr(int increment) : _increment(increment) { }
  void CodeGen();
  void DebugDescription(int level);
};

class InputExpr : public Expr
{
public:
  InputExpr() { }
  void CodeGen();
  void DebugDescription(int level);
};

class OutputExpr : public Expr
{
public:
  OutputExpr() { }
  void CodeGen();
  void DebugDescription(int level);
};

class LoopExpr : public Expr
{
protected:
  std::vector<Expr *> _exprs;
public:
  LoopExpr(std::vector<Expr *> &exprs) : _exprs(exprs) { }
  void CodeGen();
  void DebugDescription(int level);
};

#endif // EXPR_H
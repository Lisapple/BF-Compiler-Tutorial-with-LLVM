#include "DebugDescription.h"

void ShiftExpr::DebugDescription(int level)
{
  std::cout.width(level);
  std::cout << "ShiftExpr (" << _step << ")" << std::endl;
}


void IncrementExpr::DebugDescription(int level)
{
  std::cout.width(level);
  std::cout << "IncrementExpr (" << _increment << ")" << std::endl;
}


void InputExpr::DebugDescription(int level)
{
  std::cout.width(level);
  std::cout << "InputExpr" << std::endl;
}


void OutputExpr::DebugDescription(int level)
{
  std::cout.width(level);
  std::cout << "OutputExpr" << std::endl;
}


void LoopExpr::DebugDescription(int level)
{
  std::cout << "LoopExpr: [" << std::endl;
  for (std::vector<Expr *>::iterator it = _exprs.begin(); it != _exprs.end(); ++it) {
    std::cout << std::string(level * 2, ' ');
    (*it)->DebugDescription(level+1);
  }
  std::cout << std::string(level, ' ') << "]" << std::endl;
}
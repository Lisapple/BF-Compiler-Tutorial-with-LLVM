#include "Parser.h"

bool Parser::isSkipable(char c)
{
  return (c != '<' && c != '>' &&
          c != '+' && c != '-' &&
          c != '.' && c != ',' &&
          c != '[' && c != ']');
}

char Parser::getToken()
{
  char c = 0;
  while ( (c = data[index++]) && isSkipable(c) ) { }
  return c;
}

void Parser::parse(std::vector<Expr *> &exprs)
{
  char c = 0;
  while ( (c = getToken()) ) {
    Expr *expr = NULL;
    switch ( c ) {
      case '<': {
        expr = new ShiftExpr(-1); // Shift to the left
      }
        break;
      case '>': {
        expr = new ShiftExpr(1); // Shift to the right
      }
        break;
      case '+': {
        expr = new IncrementExpr(1); // Increment (add 1)
      }
        break;
      case '-': {
        expr = new IncrementExpr(-1); // Decrement (substract 1)
      }
        break;
      case '.': {
        expr = new OutputExpr(); // Output value
      }
        break;
      case ',': {
        expr = new InputExpr(); // Output value
      }
        break;
      case '[': {
        std::vector<Expr *> loopExprs;
        parse(loopExprs); // Enter into a new function
        expr = new LoopExpr(loopExprs);
      }
        break;
      case ']': {
        return ; // Exit the function
      }
      default: break; // Ignored character
    }
    if (expr) {
      exprs.push_back(expr);
    }
  }
}

void Parser::DebugDescription(int level)
{
  for (std::vector<Expr *>::iterator it = _exprs.begin(); it != _exprs.end(); ++it) {
    std::cout << std::string(level * 2, ' ');
    (*it)->DebugDescription(level+1);
  }
}
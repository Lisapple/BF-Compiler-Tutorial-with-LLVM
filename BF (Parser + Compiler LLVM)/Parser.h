#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <iostream>
#include "Expr.h"

class Parser
{
protected:
  std::string data;
  int index;
  std::vector<Expr *> _exprs;
  
  static bool isSkipable(char c);
  char getToken();
  void parse(std::vector<Expr *> &exprs);
public:
  Parser(std::string s) : data(s), index(0)
  { parse(_exprs); }
  
  void CodeGen(Module *M, IRBuilder<> &B);
  void DebugDescription(int level);
};

#endif // PARSER_H
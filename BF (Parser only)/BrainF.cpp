#include "BrainF.h"

using namespace std;

int main(int argc, char *argv[])
{
  string s = ">+<[>++[-]<[-]]>+<++";
  Parser parser(s);
  parser.DebugDescription(0);
}
BrainF**k with LLVM
===================

Introduction
------------

Let's write a compiler using LLVM. Since LLVM (Low Level Virtual Memory) manages the generation and the execution of native binary code, we only need to focus on the part that read our program source code and makes all LLVM API calls.

[BrainF**k](http://esolangs.org/wiki/brainfuck) is a good introduction to compiler programmation because it use much of basic tools of LLVM API, needed to create more complex compiler.

BrainF**k
---------

BrainF**k language is very easy: the internal structure is composed of an array of cells and a cursor. The cursor allows to access (read or write) a cell and each cell contains a number (basically from 0, by default, to 255, to be output as ASCII).

You've got 8 operations:

<pre>
'<' and '>' to move the cursor to access another cell (move to left and right)
'+' and '-' to change the value of the current cell, where the cursor stands (to increment and decrement)
',' and '.' to input (ask the user a value to set) and to output (print the value) the current cell
'[' and ']' to do conditional looping depending of the current cell value (enter the loop the current celle value > 0 and exit if it equals to 0)
</pre>

For example, this dummy program:
<pre>
>><+++<++>--[->+<]>.
</pre>
will gives the output: 1

The state of cells and cursor at the end of the execution is:
<pre>
cells : [2][0][1]
cursor:        ^
</pre>

Detailled operations:
<pre>
>>  : set cursor to 0 + 2 = 2
<   : set cursor to 2 - 1 = 1
+++ : add 3 to current cell value ([1] = 3)
<   : set cursor to 1 - 1 = 0
++  : add 2 to current cell value ([0] = 2)
>   : set cursor to 0 + 1 = 1
--  : subtract 2 to current cell value ([1] = 3 - 2 = 1)
[   : enter the loop since the current cell value > 0 ([1] = 1)
-   : subtract 1 to current cell value ([1] = 1 - 1 = 0)
>   : set cursor to 1 + 1 = 2
+   : add 2 to current cell value ([2] = 0 + 1 = 1)
<   : set cursor to 1 - 1 = 1
]   : exit the loop since the current cell value is 0
>   : set cursor to 1 + 1 = 2
.   : print the current cell value ([2] = 1)
</pre>

Compiler & LLVM
---------------

The definition of a compiler is a program that transform a format of file to another. For example, XSD allows to transform XML to HTML, Dart compiler to transform Dart script to Javascript, etc. In our case, we want our compiler to transform BrainF**k files to native code, for x86, ARM or else.

For flexibility reasons, we compile (transform) our BF file to an intermediate language (much lower level) and LLVM compiles this to assembler, which once linked, gives native binary. Hopefully, most works are done by LLVM like assembler generation, we only need to compile our input file to this intermediate language, called *Intermediate Representation*. This kind of compiler is called a *front-end* for LLVM (generate IR form input file). The compiler which generate assembler from IR is called a *back-end*.

Intermediate language allows low-level optimisations and is totally _platform independent_, so this representation program will be exactly the same even if you want to generate binary code to x86, ARM or Sparc architecture. Basically, intermediate language allows memory operation (allocation, reading and writing data from/to memory), arithmetic operation (add, mul, xor, etc.), basic data structure (c-like struct and array) and integration to external c-function (printf, scanf, etc.).

<pre>
                 |--   Front-end   --|                                          |--          Back-end         --|
[Source code] -> [Parser] --- LLVM API --> [IR (Intermediate Representation)] -> [Compilation] -> [Native binary] -> [Execution] -> [Output]
                              |--                         LLVM (Low Level Virtual Machine)                    --|
</pre>

For example, *clang* (C/C++ compiler based on LLVM) will follow this schema:

<pre>
                       |-                     clang                      -|
[C/C++ code source] -> [Parser] -> [IR] -> [Compilation] -> [Native binary] -> [Execution] -> [Output]
|--   hello.c   --|    |--            $ clang hello.c -o hello          --|    |--    $ ./hello    --|
</pre>

As LLVM is a "Virtual Machine", it also can execute code without to generate native binary, following this schema:

<pre>
                 |--   Front-end   --|                                         
[Source code] -> [Parser] --- LLVM API --> [IR (Intermediate Representation)] -> [Execution] -> [Output]
                              |--              LLVM (Low Level Virtual Machine)          --|
</pre>

The performance are the same that a native binary, it's just a convenient way to execute native code without to export a binary. You can choose, once the IR generate from your compiler, to execute native code from the Virtual Machine or to export native binary.

Installation
------------

Simply, to install the lasted version of LLVM with git:
<pre>
$ git clone http://llvm.org/git/llvm.git
$ cd llvm/
$ ./configure
$ sudo make install
</pre>

More informations on [installation wiki](http://llvm.org/docs/GettingStarted.html#git-mirror).

BF Parser
---------

First, we need to get _tokens_ from our BF program. We will ignore (skip) all characters that are not BF commands (note that we use char, not string):
<pre>
bool Parser::isSkipable(char c)
{
  return (c != '<' && c != '>' &&
          c != '+' && c != '-' &&
          c != '.' && c != ',' &&
          c != '[' && c != ']');
}
</pre>

The method `getToken` returns the next valid character (token).

The `while` loop will stop when `c` will be equal to zero, and since the last character of `_data` (at index `index`) is zero (null-character terminated string), the method will stop when it reaches the end of the string.

Since we want to skip ignored character, the `while` will loop until a non-ignored character is found (or the end of the string).
<pre>
char Parser::getToken()
{
  char c = 0;
  while ( (c = _data[_index++]) && isSkipable(c) ) { }
  return c;
}
</pre>

Note: the `while ( (var = value) ) { }` pattern is common, it's a shorter way to do:
<pre>
while (1) {
  var = value;
  if (var == 0) {
    break;
  }
}
</pre>
Note the extra parenthesis `(var = value)`, this not evaluates the assignment (always true) but the value of the variable assigned, `var` in this case; if `var` is greater than zero, the `while` loop continues to loop. A warning (with `-Wall`) shows up if the extra parenthesis are missing.

Now, we can create our parser class:
<pre>
class Parser
{
protected:
  string _data;
  int _index;
  
  static bool isSkipable(char c);
  char getToken();
public:
  Parser(string s) : _data(s), _index(0) { }
};
</pre>
The default constructor will take a string, the program, store it into `_data` and will parse it.

Note: Be careful to hide (under `protected:` or `private:`) all method and variables that shouldn't be accessed outside the class.
The protected `getToken()` method is a good example, we don't want to access (and modify `_index`) outside the parser class.

For debugging propose, we will expose protected and private variables and methods, don't forget to hide them after tests.

Usage:
<pre>
class Parser
{
protected:
  string _data;
  int _index;
  
  static bool isSkipable(char c);
--  char getToken();
public:
  Parser(string s) : _data(s), _index(0) { }
  
++  char getToken();
};

string s = ">>+++ <<3 + >++ < 123 abc .>.>.";
Parser parser(s);
char c = 0;
while ( (c = parser.getToken()) ) {
  cout << c << " ";
}
</pre>

This writes out: `> > + + + < < + > + + < . > . > .` and ignored characters are ignored indeed.

Put back the `getToken()` method into protected section.

Let's now write the `parse()` method.
We will use the pattern from the last example but we will create a case for each item:
<pre>
class Parser
{
protected:
  string _data;
  int _index;
  
  static bool isSkipable(char c);
  char getToken();
++  void parse();
public:
  Parser(string s) : _data(s), _index(0)
  { parse(); }
};

++ void Parser::parse()
++ {
++    char c = 0;
++    while ( (c = getToken()) ) {
++      switch (c) {
++        case '<':
++          cout << "go to left" << endl; break;
++        case '>':
++          cout << "go to right" << endl; break;
++        case '+':
++          cout << "add one" << endl; break;
++        case '-':
++          cout << "substract one" << endl; break;
++        case '.':
++          cout << "print" << endl; break;
++        case ',':
++          cout << "ask for value" << endl; break;
++        case '[':
++          cout << "start loop" << endl; break;
++        case ']':
++          cout << "end loop" << endl; break;
++        default: break; // Ignored character
++      }
++    }
++ }
</pre>

Now our parser knows how to handle each character, we use it to create a AST (Abstract Structure Tree) where we will create an object for each case.

Let's begin with the "move to" operation and we will call it the "shift" expression. We can create a single shift expression for right and left, the first add one to cursor (+1), the latter remove one (-1) and we need to check that the cursor can't be less than zero. 

The class looks like this:
<pre>
class ShiftExpr
{
protected:
  int _step;
public:
  ShiftExpr(int step) : _step(step) { }
};
</pre>

We will inherit this class from an abstract (virtual) `Expr` class which contains a public `CodeGen()` method to generate the LLVM code, we will see the implementation later.

<pre>
class Expr
{
public:
  virtual void CodeGen() = 0;
};
</pre>

and we update our `ShiftExpr` to inherite from `Expr` publicly to allow polymorphism:

<pre>
class ShiftExpr : public Expr
{
protected:
  int _step;
public:
  ShiftExpr(int step) : _step(step) { }
  void CodeGen();
};

void ShiftExpr::CodeGen()
{
  // We will implement this later
}

</pre>

Now, we will use this class into the parse method. This create an shift expr which could generate LLVM bitcode.
Expressions will be stored into a vector (some sort of array).
<pre>
#include &lt;vector&gt;
</pre>

<pre>
class Parser
{
protected:
  string data;
  int index;
++  vector&lt;Expr *&gt; exprs;
  
  static bool isSkipable(char c);
  char getToken();
  void parse();
public:
  Parser(string s) : data(s), index(0)
  { parse(); }
};

void Parser::parse()
{
  char c = 0;
  while ( (c = getToken()) ) {
++    Expr *expr = NULL;
    switch (c) {
      case '<': {
++        expr = new ShiftExpr(-1); // Shift to the left
++      }
          break;
      case '>': {
++        expr = new ShiftExpr(1); // Shift to the right
++      }
          break;
      case '+':
          cout << "add one" << endl; break;
      case '-':
          cout << "substract one" << endl; break;
      case '.':
          cout << "print" << endl; break;
      case ',':
          cout << "ask for value" << endl; break;
      case '[':
          cout << "start loop" << endl; break;
      case ']':
          cout << "end loop" << endl; break;
      default: break; // Ignored character
    }
++    if (expr) {
++      exprs.push_back(expr);
++    }
  }
}
</pre>

And we will create three more expressions: `IncrementExpr`, `InputExpr` and `OutputExpr` like that:

<pre>
class IncrementExpr : public Expr
{
protected:
  int _increment;
public:
  IncrementExpr(int increment) : _increment(increment) { }
  void CodeGen();
};

void IncrementExpr::CodeGen()
{
  // We will implement this later
}
</pre>

<pre>
class InputExpr : public Expr
{
public:
  InputExpr() { }
  void CodeGen();
};

void InputExpr::CodeGen()
{
  // We will implement this later
}
</pre>

<pre>
class OutputExpr : public Expr
{
public:
  OutputExpr() { }
  void CodeGen();
};

void OutputExpr::CodeGen()
{
  // We will implement this later
}
</pre>

And, again, update the `parse()` method:

<pre>
void Parser::parse()
{
  char c = 0;
  while ( (c = getToken()) ) {
    Expr *expr = NULL;
    switch (c) {
      case '<': {
        expr = new ShiftExpr(-1); // Shift to the left
      }
          break;
      case '>': {
        expr = new ShiftExpr(1); // Shift to the right
      }
          break;
      case '+': {
++        expr = new IncrementExpr(1); // Increment (add 1)
++      }
        break;
      case '-': {
++        expr = new IncrementExpr(-1); // Decrement (substract 1)
++      }
        break;
      case '.': {
++        expr = new OutputExpr(); // Output value
++      }
        break;
      case ',': {
++        expr = new InputExpr(); // Output value
++      }
        break;
      case '[':
          cout << "start loop" << endl; break;
      case ']':
          cout << "end loop" << endl; break;
      default: break; // Ignored character
    }
++    if (expr) {
++      exprs.push_back(expr);
++    }
  }
}
</pre>

Now, a little bit more complicated: the loop. A loop starts with `[` and ends `]`. The program enters the loop only if the value at the current cursor is greater than zero and the program ends the loop only if the value at the current cursor is equal to zero. We will see later how to create a loop with LLVM IR language but for now, we only need to create a new class `LoopExpr` which contains all expressions inside the loop.

As a loop can contain other loops, we will use recursive calls of `parse()` function, so we need to modify this method to allow reccursivity. In place for calling the function with no arguments, we will use the vector which holds parser expressions as argument.

The new version of the prototype is simply:
<pre>
void Parser::parse(vector<Expr *> &exprs);
</pre>
and the implementation doesn't change.

Also, we need to change the calling into the default constructor:
<pre>
Parser(string s) : data(s), index(0)
{ parse(exprs); }
</pre>
Note: Don't confound the initialisation of instance variables, after the `:` and the calling of a method inside the method body. These cases aren't correct:
<pre>
Parser(string s) : data(s), index(0), parse(exprs) // error
{ }
</pre>

<pre>
Parser(string s)
{
  data(s); index(0); // error
  parse(exprs);
}
</pre>

If you are not confident with this writing, you could also write this:
<pre>
Parser(string s)
{
  data = s; index = 0;
  parse(exprs);
}
</pre>

Now, let's write our `LoopExpr` class, which be initialised with a vector of expressions:
<pre>
class LoopExpr : public Expr
{
protected:
  vector<Expr *> _exprs;
public:
  LoopExpr(vector<Expr *> &exprs) : _exprs(exprs) { }
  void CodeGen();
};

void LoopExpr::CodeGen()
{
  // We will implement this later
}
</pre>

We create the vector when the parser will find a start loop character `[`:
<pre>
void Parser::parse(vector<Expr *> &exprs)
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
++        vector<Expr *> loopExprs;
++        parse(loopExprs); // Enter into a new function
++        expr = new LoopExpr(loopExprs);
      }
          break;
      case ']': {
++        return ; // Exit the function
      }
      default: break; // Ignored character
    }
    if (expr) {
      exprs.push_back(expr);
    }
  }
}
</pre>

Each time the parser will find a `[` character, it will call the same method `parse(vector<Expr *> &exprs)` method and when it will found the associated `]`, it will quit this function.
Let's look at this simple BF example:
<pre>
++[+[-]]
</pre>
At the beginning, we initialising the parser and calling the `parse`method. Then, the current cell is incrementing by two before entering the loop. The parser finds a start loop character `[` so it calls the `parse` method again by passing a empty vector argument.

The parser will continue from the current token, after the `[` (inside the loop) and will parsing the next `+`, which increment again the current cell. Then, another `[` is found, and the parser will one time more calls the `parse` method with a new brand vector argument. A this time, no `LoopExpr` is yet created, not until a matching `]` is found. The trace looks like:
<pre>
parse this: "++[+[-]]" {
  // '+' found
  // '+' found
  // First '[' found here, call 'parse'
  parse this: "+[-]]" {
    // '+' found
    // Another '[' found here, call 'parse'
    parse this: "-]]" {
      // We are here
    }
  }
}
</pre>

Next, we decrement the current cell and a matching `]` is found, it's time to exit the current loop. At this precise time, the `parse` method exits and the parser goes on by creating an instance of `LoopExpr` by passing the vector wich contains an instance of `IncrementExpr`, created from the previous `-` character. Then, another `]` is found, create a `LoopExpr` with previous expression and exit. The trace looks like:

<pre>
parse this: "++[+[-]]" {
  // '+' found
  // '+' found
  // First '[' found here, call 'parse'
  parse this: "+[-]]" {
    // '+' found
    // Another '[' found here, call 'parse'
    parse this: "-]]" {
      // '-' found
      // A ']' found here, create a 'LoopExpr' with '-' and exit
      return
    }
    // A ']' found here, create a 'LoopExpr' with: '+' and the previous 'LoopExpr' (which contains '-')
    // and then, exit
    return
  }
  // End of program
}
</pre>

As the `parse()` method takes a reference to a vector, with can modify it inside the method and getting a up-to-date version when the method exits, needed to create an new `LoopExpr` instance (we could also return the vector when exiting the method but this latter solution required to create a copy of a vector, which can more expensive that using a reference).

We also need to add a `CodeGen()` method to Parser, it will be the entry point to generate native code from the vector of expressions:

<pre>
/** Parser.h **/
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
  
++  void CodeGen(Module *M, IRBuilder<> &B);
  void DebugDescription(int level);
};
</pre>

<pre>
/** Parser.cpp **/
++ void Parser::CodeGen(Module *M, IRBuilder<> &B)
++ {
++ }
</pre>

Refactoring
-----------

Now our whole is about 200 lines of code, it's time for refactoring!
The idea is to separate part into different files like `BrainF.cpp` with out `main` function, `Parser.cpp` for the `Parser` class, `Expr.cpp` for `*Expr` classes and `CodeGen.cpp` for all `*Expr::CodeGen()` methods implementation (as we will see, this file could be the biggest file of the program, it's important to separate from `*Expr` classes definition).
Let's create `Brain.cpp`, `Expr.cpp` and `CodeGen.cpp` but also header files like so:
<pre>
- BrainF.h
- BrainF.cpp
- Parser.h
- Parser.cpp
- Expr.h
- Expr.cpp
- CodeGen.h
- CodeGen.cpp
</pre>

Move `Parser` class definition to `Parser.h` and implementation to `Parser.cpp`. I recommend the header file to looks like
<pre>
#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include "Expr.h"

// Class declaration here

#endif // PARSER_H
</pre>

The define, like `PARSER_H` (uppercase path, `folder/my_file.h` could become `FOLDER_MY_FILE_H`), is used to avoid multiple inclusion [link].

[Code source](./BF \(Parser only\)/)

We need to update the `Makefile` to include all `*.cpp`files:
<pre>
all: build run

build: Expr.cpp CodeGen.cpp Parser.cpp BrainF.cpp
  clang++ Expr.cpp CodeGen.cpp Parser.cpp BrainF.cpp -o BrainF

run: BrainF
  ./BrainF
</pre>

LLVM API
========

Note: To follow LLVM coding rules for the API:

- indentation are created with two white-spaces (not a tab)
- each line of code should not be more than 85 columns long
- variables should be short, begins with an Uppercase letter and ends with the few first letters of the type; for example a Variable can be called "CountV" and a Type "IntTy"

Examples:
<pre>
llvm::Variable *CountV = ...;
llvm::Type *IntTy = ...;
llvm::BasicBlock *LoopBB = ...;
llvm::IRBuilder<> LoopB(LoopBB);
</pre>

Or with explicit namespace (preferred):

<pre>
namespace llvm;

Variable *CountV = ...;
Type *IntTy = ...;
BasicBlock *LoopBB = ...;
IRBuilder<> LoopB(LoopBB);
</pre>

Let's started (seriously)
-------------------------

We now update our `XXXExpr::CodeGen` method to pass the module and the IR Builder to generate IR code.

`llvm:Module` contains informations about locals and globals variables, functions, etc.

`llvm::IRBuilder<>` is a template to add instructions to the current module.

<pre>
void Parser::CodeGen(Module *M, IRBuilder<> &B)
{
  // @TODO: Initialise the native code generator (initialise variables, etc.)
  // @TODO: Recursively generate code by calling `CodeGen(M, B)`on each expression
}

void ShiftExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  // @TODO: Add |_step| to the current index
}

void IncrementExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  // @TODO: Add |_step| to the current cell
}

void InputExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  // @TODO: Call "scanf" std function
  // @TODO: Put the input value to the current cell
}

void OutputExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  // @TODO: Call "printf" std function
}

void LoopExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  // @TODO: Create a loop
  // @TODO: Enter the loop only if the current cell value is greater than zero
  // @TODO: Exit the loop if the current cell value is equal to zero
}
</pre>

Into the `Parser` method, we will first create two _global_ variables (the opposite of local variable) for the index and cells, and initialise them.

The second step is to recursively call `CodeGen()` on each `*Expr` from |_expr|, with a simple for loop.
<pre>
void Parser::CodeGen(Module *M, IRBuilder<> &B)
{
  // @TODO: Create "index" global variable
  // @TODO: Initialise "index" global variable
  // @TODO: Create "cells" global variable
  // @TODO: Initialise "cells" global variable

  // Recursively generate code
  for (std::vector<Expr *>::iterator it = _exprs.begin(); it != _exprs.end(); ++it) {
    (*it)->CodeGen(M, B);
  }
}
</pre>

For the `Shift` expression, it simple but we need to decompose each part. To change the value of a variable, the computer need 3 steps:

- Load the value at the given address (the pointer of the "index" global variable)

- Create a temporary variable to store the result of the operation ("index" + "step")

- Save the temporary variable value at the "index" address

<pre>
void ShiftExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  // @TODO: Get the value of "index" global variable
  // @TODO: Create a temporary variable that contains the "index" + |_step| result
  // @TODO: Save the "index" global variable
}
</pre>
It can look complicated but it's what your computer does every time it add two value from memory.

It should be easy for you now to understand this basic IR code (all variables start with "%"):
<pre>
%index = load %index_addr ; Load the address from (the given) "index_addr" to "index" (created)
%temp = add %index, 1    ; Create "temp" variable with the result of "index" + 1
store %temp, %index_addr ; Save (store) the value of "temp" at address "index_addr"
</pre>


The `IncrementExpr` uses the same principle but this time, we need to load "index" first to get the value from "cells" at a specific index, the "index" value (the name makes sense now!).

Once we've got "index", we need to compute the offset to get the cell value, load it, create a temporary variable to store the addition then save it back:
<pre>
void IncrementExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  // @TODO: Get the value of "index" global variable
  // @TODO: Compute the offset of "cells" array based on "index"
  // @TODO: Load the offset address from "cells"
  // @TODO: Create a temporary variable that contains the "cells[index]" + |_step| result
  // @TODO: Save the temporary variable at address "cells[index]"
}
</pre>
Note: You could ask why we use the `add` operator for decrement and not `sub`, since it's more natural. In old days for computing, `add -1` was generally much faster than "sub 1" but with modern architectures, it's different, and the best way is to let LLVM decides which platform-dependant optimisations will be made. Since IR code is platform independent, we can even not provide an hint for this. Just keep that LLVM will certainly knows better what to optimise and it's better to focus on algorithm and data structures optimisation than these silly ones, and keep the code clear for everyone.

The `InputExpr` expression, we will simply use the `scanf` C function, LLVM will link to it during compilation. We just need to pass arguments to this function and call it. The single catch is to create a global variable to store the format string, but we already know how to do it.

<pre>
void InputExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  // @TODO: Prepare arguments (format string and input char)
  // @TODO: Call "scanf" std function
  // @TODO: Get the value of "index" global variable
  // @TODO: Compute the offset of "cells" array based on "index"
  // @TODO: Save the temporary variable at address "cells[index]" (no operation, just overwrite)
}
</pre>

The `OutputExpr` expression is simply a call to `printf` C function:

<pre>
void OutputExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  // @TODO: Get the value of "index" global variable
  // @TODO: Compute the offset of "cells" array based on "index"
  // @TODO: Load the offset address from "cells"
  // @TODO: Prepare arguments (format string and the current cell value)
  // @TODO: Call "printf" std function
}
</pre>

The `Loop` expression is more complex since the comportment will depend of current cell value, like enter the loop if the current cell value is greater to zero and exit if this value if equal to zero. If we need to make choice depending to value (at runtime), we need to use _block_ and _branches_.

A block is simply instructions that we separate to other piece of code, we can _jump_ to the block, skip it or even loop by calling it at the end of itself. Since block is a basis of LLVM IR code structure, they are referred as `Basic blocks` and are so useful that every function (like the `main` one) starts with a block.

In some way, blocks are like function, we can call them (using _branches_) but we can't pass argument to it, therefore variables created before the branch is made are still accessible.

Branches are equally useful since a block must be call by a branch (except the first block of a function, called `EntryBlock`) and must finish by a branch or a `ret` (return, to exit a function) instruction. Branches can be _conditional_ and _direct_.

Basic blocks are constructed like so:

<pre>
  ; Previous instructions
  br %BlockName ; Direct branching to "BlockName"

LoopBlock:
  ; Loop instructions
  %result = icmp eq i32 %i, 0 ; CoMPare as Int (icmp with int32) if %i is EQual (eq) to 0, %i equal 1 (true), else 0 (false)
  br i1 %result, label %DoneBlock, label %LoopBlock ; If %result (boolean, of type int1) is true, branch to DoneBlock, else to LoopBlock

DoneBlock:
  ; Next instructions
</pre>

For out `Loop` expression to work correctly, we need to check _before_ entering the loop if the current cell value is greater than 0 (Signed Greater Than, or `sgt` of LLVM) and checking at the end of the loop is the value is equal to 0 to exit the loop. We can resume the structure our loop to:

<pre>
  ; Previous instructions of our program
  br %StartLoopBlock

StartLoopBlock:
  ; @TODO: Load current cell value and put it into %value
  %enterLoop = icmp sgt i32 %value, 0 ; if (%value > 0), enter the loop, else skip the loop
  br i1 %enterLoop, label %LoopBlock, label %EndLoopBlock

LoopBlock:
  ; Loop instructions
  ; @TODO: Load current cell value and put it into %value
  br %StartLoopBlock ; Restart the loop (will exit if current cell value > 0)

EndLoopBlock:
  ; Next instructions of our program
</pre>

Note: a block must existing before branching to it. It's generally more clear to add the branching instruction _before_ adding instructions to it. For example, the following branching:

<pre>
  ; Previous instructions
  br %MyBlock

MyBlock:
  %3 = add %1, %2; Dummy addition for example
  br %MyBlock2

MyBlock2:
  ; Next instructions
</pre>

will be decomposed as follow from API:

<pre>
// Create "MyBlock" block
// Add branch to "MyBlock"
// Add "add" instruction into "MyBlock"
// Create "MyBlock2" block
// Add branch to "MyBlock2"
</pre>

We update our `CodeGen` function comments:

<pre>
void LoopExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  // @TODO: Create the "StartLoop" block
  // @TODO: Create branch to "StartLoop"

  // @TODO: Get the value of "index" global variable
  // @TODO: Compute the offset of "cells" array based on "index"
  // @TODO: Compare current cell value with zero

  // @TODO: Create the "Loop" block
  // @TODO: Create the "EndLoop" block
  // @TODO: Create conditional branch to %LoopBlock or %EndLoopBlock

  // @TODO: Generate instructions from |_exprs|
  // @TODO: Restart loop
}
</pre>

To create a global variable, with need to specify the type and the initialisation value:

<pre>
Module *M = // Default module
LLVMContext &C = M->getContext(); // Get the current context (an opaque part of LLVM that contains state about the program execution)

Type *Ty = Type::getInt32Ty(C); // 32 bits integer type
const APInt Zero = APInt(32, 0); // 32 bits integer with value zero
Constant *InitK = Constant::getIntegerValue(Ty, Zero); // Create a constant with the int32 value zero
__BrainF_IndexPtr = new GlobalVariable(*M, // Use default module
                                       Ty, // 32 bits integer
                                       false, // non-constant
                                       GlobalValue::WeakAnyLinkage, // Keep one copy when linking (weak)
                                       InitK, // Initialise with constant zero
                                       "brainf.index" // Call this global variable "brainf.index"
                                       );
</pre>

And since with will use it as global variable into our program, we will declare it static:
<pre>
static GlobalVariable *__BrainF_IndexPtr = NULL;
</pre>

and be sure that we initialise it only once:
<pre>
if (!__BrainF_IndexPtr) {
  // Initialisation goes here
}
</pre>

For the cells array, it's the same way but we need the declare the exact size of it. Hundred cells seem correct for most programs:
<pre>
#define kCellsCount 100
ArrayType *ArrTy = ArrayType::get(Type::getInt32Ty(C), kCellsCount); // An array type of 100 x int32
std::vector<Constant *> constants(kCellsCount, B.getInt32(0)); // Create a vector of 100 items equal to 0
ArrayRef<Constant *> Constants = ArrayRef<Constant *>(constants); // Create an array of 100 constants equal to 0
Constant *InitPtr = ConstantArray::get(ArrTy, Constants); // Create a pointer to this global array
__BrainF_CellsPtr = new GlobalVariable(*M, // Use default module
                                       ArrTy, // an array of 100 x int32
                                       false, // non-constant
                                       GlobalValue::WeakAnyLinkage, // Keep one copy when linking (weak)
                                       InitPtr, // Initialise with the array
                                       "brainf.cells" // Call this global variable "brainf.cells"
                                       );
</pre>

And we need to also declare it:
<pre>
static GlobalVariable *__BrainF_CellsPtr = NULL;
</pre>

and be sure that we initialise it only once:
<pre>
if (!__BrainF_CellsPtr) {
  // Initialisation goes here
}
</pre>

Now that `__BrainF_IndexPtr` and `__BrainF_CellsPtr` are pointer to respectively int32 and [100 x int32], we need to do some work to get the value.
For the `__BrainF_IndexPtr`, it's simply a pointer to int32, noted `*int32`. To read it, we need the `load` instruction, the result will be the value that the pointer contains (i.e.: the value stored in memory at this address).
In IR, this gives:
<pre>
%index = load %brainf.index
</pre>

And with LLVM API, this is:
<pre>
IRBuilder<> &B = // Use current builder
Value *IdxV = B.CreateLoad(__BrainF_IndexPtr);
</pre>

The save operation is simple: put a value at the pointer but there is a catch, the value _must_ be of the same type that the base pointer type. For example, you have to save a `int32` into a `*int32`, you can't save a `int8` or `int64` into it, you have to cast the value. Without casting, an assert will be thrown when executing the bitcode.

The save operation is done with the store instruction (which returns nothing):
<pre>
store %index, %brainf.index ; OK, %index is int32 and %brainf.index is *int32
</pre>

And with LLVM API, this is:

<pre>
// Get value for "brainf.index"
Value *IdxV = B.CreateLoad(__BrainF_IndexPtr); // %1 = load %brainf.index

// Add 10 to |IdxV|
Value *NewIdxV = B.CreateAdd(IdxV, B.getInt32(10)); // %2 = add int32 %1, 10

// Save it back
B.CreateStore(IdxV, __BrainF_IndexPtr); // store %2, %brain.index
</pre>

Now, the value of `brainf.index` is 10, and the `ShiftExpr` simply becomes:

<pre>
void ShiftExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  // Get the value of "index" global variable
  Value *IdxV = B.CreateLoad(__BrainF_IndexPtr);

  // Create a temporary variable that contains the "index" + |_step| result
  Value *ResV = B.CreateAdd(IdxV, B.getInt32(_step));

  // Save the "index" global variable
  B.CreateStore(ResV, __BrainF_IndexPtr);
}
</pre>

We now need to access to cells.

It's really important to note that `__BrainF_CellsPtr` is *a pointer to an array* (since we create a global variable), for LLVM it's a `[i32 x 100]*`. To access to the *3rd element* of this array, we will use the `Get Element Pointer` method (generally noted as `GEP`). This computes the memory offset that we need to access a specific element of an array but it's _really important_ to note that this method *never access to memory*, it only do some calculation (that we can of course also do by our own). The idea is simple: from a pointer, we provide an array of integer of offsets to access to a specific element.

In our example, we've got a pointer to an array, if we need to access to the *3rd element*, we need two steps:

- Convert pointer to array to get `[i32 x 100]*` to `[i32 x 100]`: offset 0
- Get the 3rd element from this array `[i32 x 100]+3` : offset 3
Our offsets array is `[0, 3]` and the final instruction `%third_element = getelementptr %brainf.index, 0, 3`

With LLVM API, this gives:
<pre>
ArrayRef<Value *> IdxsArr((Value* []){B.getInt32(0), B.getInt32(3)}); // Create the array of offsets: [0, 3]
Value *CellPtr = B.CreateGEP(__BrainF_CellsPtr, IdxsArr); // %cell = getelementptr %brainf.index, 0, 3
</pre>
Since `getelementptr` doesn't acces to any memory (it only computes memory final address), it returns also a pointer to the 3rd element. 

We obviously need to `load` the pointer to access to the integer value:
<pre>
Value *CellV = B.CreateLoad(CellPtr);
</pre>

And to change the cell value, we just need to save the new value at `CellPtr ` using the `store` instruction:

<pre>
void IncrementExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  LLVMContext &C = M->getContext();

  // Get the value of "index" global variable
  Value *IdxV = B.CreateLoad(__BrainF_IndexPtr);

  // Compute the offset of "cells" array based on "index"
  ArrayRef<Value *> IdxsArr((Value* []){B.getInt32(0), IdxV});
  Value *CellPtr = B.CreateGEP(__BrainF_CellsPtr, IdxsArr);

  // Load the offset address from "cells"
  Value *CellV = B.CreateLoad(CellPtr);

  // Create a temporary variable that contains the "cells[index]" + |_increment| result
  Value *ResV = B.CreateAdd(CellV, B.getInt32(_increment))

  // Save the temporary variable at address "cells[index]"
  B.CreateStore(ResV, CellPtr);
}
</pre>

For the `InputExpr`, we will use `scanf` c-function. For this, we need the format string `"%d"`, to get the `scanf` standard function and to call it.

To create a string, `"%d"` in this case, for the `scanf` function, the only way is to create a global constant. LLVM API provide a convenient function for that from the IRBuilder `B`: `B.CreateGlobalString("%d", "brainf.scanf.format");`, since it a global constant, it name should be explicit, like "brainf.scanf.format".


This constant should be unique, every time we call this function, this create a new global constant ("brainf.scanf.format2", "brainf.scanf.format3", etc.), we can use a static variable to ensure we create only one global constant:
<pre>
static Value *GScanfFormat = NULL;
if (!GScanfFormat) {
  GScanfFormat = B.CreateGlobalString("%d", "brainf.scanf.format");
}
</pre>

To call an existing function, we need to get the exact type first. In our case, the `scanf` prototype is, for LLVM, as: `i32 scanf(i8*, …)`. It's a function that takes a variable number of argument (also called "vaarg"), the first argument is a _pointer to 8 bit integer_ (`i8*`) an returns a _32 bit integer_ (`i32`).
<pre>
Type* ScanfArgs[] = { Type::getInt8PtrTy(C) };
FunctionType *ScanfTy = FunctionType::get(Type::getInt32Ty(C), // Returns i32
                                          ScanfArgs, // First argument is i8* argument
                                          true // This function contains variable number of arguments
                                          );
</pre>

As a standard function, `scanf` is automatically accessible by LLVM API, we just need the name and the correct function type:
<pre>
Function *ScanfF = cast<Function>(M->getOrInsertFunction("scanf", ScanfTy)); // Find "scanf" function
</pre>

Once we have got the function and arguments, we just need to call it. With `B.CreateCall2([function], [arg 1], [arg 2])`, we can translate our C program `scanf("%d", &i)` to that using LLVM API:
<pre>
Value *IntPtr = B.CreateAlloca(Type::getInt32Ty(C)); // Allocate memory to "i"
B.CreateCall2(ScanfF, // Call "scanf" function with 2 arguments
              CastToCStr(GScanfFormat, B), // First argument: the format string
              IntPtr // Second argument: the int pointer (i32*) to save the entered value
              );
</pre>
Note: As `B.CreateAlloc([type])` returns a pointer (the memory address), we don't need extra instruction for `scanf`, it the same that C example: `int * i = (int *)malloc(sizeof(int))` but it's important to note that `CreateAlloc` will only use stack memory (as local variable) and so, will be destroy when the function exits.

Note 2: `CastToCStr(GScanfFormat, B)` is used to convert `[i8 x 3]` to `i8*` (null-terminated c-string), as the `scanf` function required.

Let's code now:
<pre>
void InputExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  LLVMContext &C = M->getContext();

  // Prepare arguments (format string and input char)
  static Value *GScanfFormat = NULL;
  if (!GScanfFormat) {
    GScanfFormat = B.CreateGlobalString("%d", "brainf.scanf.format");
  }
  Value *IntPtr = B.CreateAlloca(Type::getInt32Ty(C));

  // Call "scanf" std function
  Type* ScanfArgs[] = { Type::getInt8PtrTy(C) };
  FunctionType *ScanfTy = FunctionType::get(Type::getInt32Ty(C), // Returns i32
                                            ScanfArgs, // Passes char* (i8*) argument
                                            true // This function contains variable argument count (also called "vaarg" function)
                                            );
  Function *ScanfF = cast<Function>(M->getOrInsertFunction("scanf", ScanfTy)); // Find "scanf" function
  B.CreateCall2(ScanfF, // Call "scanf" function with 2 arguments
                CastToCStr(GScanfFormat, B), // First argument: the format string
                IntPtr // Second argument: the int pointer (i32*) to save the entered value
                );

  // Get the value of "index" global variable
  Value *IdxV = B.CreateLoad(__BrainF_IndexPtr);

  // Compute the offset of "cells" array based on "index"
  ArrayRef<Value *> IdxsArr((Value* []){B.getInt32(0), IdxV});
  Value *CellPtr = B.CreateGEP(__BrainF_CellsPtr, IdxsArr);

  // Save the temporary variable at address "cells[index]" (no operation, just overwrite)
  Value *IntV = B.CreateLoad(IntPtr); // load the int pointer (i32*) to int value (i32)
  B.CreateStore(IntV, CellPtr);
}
</pre>

As the same that `InputExpr`, the `OutputExpr` will call a C standard function, `printf` in this case. This is the same way to do it.

<pre>
void OutputExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  LLVMContext &C = M->getContext();

  // Prepare arguments (format string and the current cell value)
  static Value *GPrintfFormat = NULL;
  if (!GPrintfFormat) {
    GPrintfFormat = B.CreateGlobalString("%c", "brainf.printf.format");
  }

  // Get the value of "index" global variable
  Value *IdxV = B.CreateLoad(__BrainF_IndexPtr);

  // Compute the offset of "cells" array based on "index"
  ArrayRef<Value *> IdxsArr((Value* []){B.getInt32(0), IdxV});
  Value *CellPtr = B.CreateGEP(__BrainF_CellsPtr, IdxsArr);

  // Load the offset address from "cells"
  Value *CellV = B.CreateLoad(CellPtr);
  
  // Call "printf" std function
  Type* PrintfArgs[] = { Type::getInt8PtrTy(C) };
  FunctionType *PrintfTy = FunctionType::get(Type::getInt32Ty(C), // Returns i32
                                             PrintfArgs, // Passes char* (i8*) argument
                                             true // "vaarg" function 
                                             );
  Function *PrintfF = cast<Function>(M->getOrInsertFunction("printf", PrintfTy)); // Find "printf" function
  B.CreateCall2(PrintfF, // Call "printf" function with 2 arguments
                CastToCStr(GPrintfFormat, B), // First argument: the format string
                CellV // Second argument: the value to print
                );
}
</pre>

For `LoopExpr`, we need to know how to create and use basic blocks. From our skeleton of comment, we need to create two blocks and branch to them:

<pre>
// Create "MyBlock" block
// Add branch to "MyBlock"
// Add "add" instruction into "MyBlock"
// Create "MyBlock2" block
// Add branch to "MyBlock2"
</pre>

To create a block, we need the current function (remember that a block is always attached to a function) and the current context.

<pre>
// Get the current context
LLVMContext &C = M->getContext();
// Get the current function (if we don't have any reference already)
Function *F = B.GetInsertBlock()->getParent();
// Create the block "MyBlock" to function "F" in to the default context "C"
BasicBlock *MyBlockBB = BasicBlock::Create(C, "MyBlock", F);
// Insert the block "MyBlock" into the current function
B.SetInsertPoint(MyBlockBB);
</pre>

To add instruction to a created block, with need to create a _builder_ with the `IRBuilder` template from the block:
<pre>
IRBuilder<> MyBlockB(MyBlockBB);
Value *ResV = MyBlockB.CreateAdd(MyBlockB.getInt32(1), MyBlockB.getInt32(2)); // ResV = 1 + 2
</pre>

Now, to _branch_ to a block, we've got the _direct branch_ and _conditional branch_:

<pre>
// Branch directly to "MyBlock"
B.CreateBr(MyBlockBB);
</pre>
Note: We branch to a `BasicBlock`, not a `IRBuilder<>` and we can only branch from outside the block, in this case, we branch from the previous block with the `B` builder.

To conditionally branch, we of course need a condition (a predicate, true or false):
<pre>
// Compare Integers (ICmp): is 3 an Int Signed Greater Than (ICmpSGT) one?
Value *ThreeSGZeroCond = B.CreateICmpSGT(B.getInt32(3),
                                         B.getInt32(1));
// If the condition |ThreeSGZeroCond| is true, branch to |MyBlockBB|, else to |AnotherBlockBB|
B.CreateCondBr(ThreeSGZeroCond, MyBlockBB, AnotherBlockBB);
</pre>

For this case, the code is:
<pre>
void LoopExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  LLVMContext &C = M->getContext();

  // Create the StartLoop block
  Function *F = B.GetInsertBlock()->getParent();
  BasicBlock *StartBB = BasicBlock::Create(C, "LoopStart", F);

  // Create branch to StartLoop
  B.CreateBr(StartBB);
  
  // Get the value of "index" global variable
  Value *IdxV = B.CreateLoad(__BrainF_IndexPtr);

  // Compute the offset of "cells" array based on "index"
  ArrayRef<Value *> IdxsArr((Value* []){B.getInt32(0), IdxV});
  Value *CellPtr = B.CreateGEP(__BrainF_CellsPtr, IdxsArr);

  B.SetInsertPoint(StartBB);
  IRBuilder<> StartB(StartBB);

  // Compare current cell value with zero
  Value *SGZeroCond = StartB.CreateICmpSGT(StartB.CreateLoad(CellPtr),
                                           StartB.getInt32(0)); // is cell Signed Int Greater than Zero?

  // Create the Loop block
  BasicBlock *LoopBB = BasicBlock::Create(C, "LoopBody", F);

  // Create the EndLoop block
  BasicBlock *EndBB = BasicBlock::Create(C, "LoopEnd", F);

  // Create conditional branch to LoopBlock or EndLoopBlock
  StartB.CreateCondBr(SGZeroCond, LoopBB, EndBB);

  B.SetInsertPoint(LoopBB);
  IRBuilder<> LoopB(LoopBB);

  // Generate instructions from |_exprs|
  for (std::vector<Expr *>::iterator it = _exprs.begin(); it != _exprs.end(); ++it) {
    (*it)->CodeGen(M, LoopB);
  }
  LoopB.CreateBr(StartBB); // Restart loop (will next exit if current cell value > 0)

  B.SetInsertPoint(EndBB);
}
</pre>

We need to update our main function to call the parser method:

BrainF.h:
<pre>
#ifndef BRAIN_F_H
#define BRAIN_F_H

++ #include "llvm/ExecutionEngine/SectionMemoryManager.h"
++ #include "llvm/ExecutionEngine/ExecutionEngine.h"
++ #include "llvm/ExecutionEngine/GenericValue.h"
++ #include "llvm/ExecutionEngine/MCJIT.h"

++ #include "llvm/Support/ManagedStatic.h"
++ #include "llvm/Support/TargetSelect.h"
++ #include "llvm/Support/raw_ostream.h"

++ #include "llvm/IR/LLVMContext.h"
++ #include "llvm/IR/IRBuilder.h"
++ #include "llvm/IR/Module.h"
++ #include "llvm/IR/Value.h"

#include &lt;iostream&gt;
#include &lt;vector&gt;

#include "Parser.h"

#endif // BRAIN_F_H
</pre>

BrainF.cpp:
<pre>
#include "BrainF.h"

using namespace llvm;

int main(int argc, char *argv[])
{
++   // Not so dummy BrainF**k program
++   std::string s = ">++++++++[<+++++++++>-]<.>>+>+>++>[-]+<[>[->+<<++++>]<<]>.+++++++..+++.>>+++++++.<<<[[-]<[-]>]<+++++++++++++++.>>.+++.------.--------.>>+.>++++.";
     Parser parser(s);
  
++   // Create the context and the module
++   LLVMContext &C = getGlobalContext();
++   ErrorOr&lt;Module *&gt; ModuleOrErr = new Module("my test", C);
++   std::unique_ptr&lt;Module&gt; Owner = std::unique_ptr&lt;Module&gt;(ModuleOrErr.get());
++   Module *M = Owner.get();

++   // Create the main function: "i32 @main()"
++   Function *MainF = cast&lt;Function&gt;(M-&gt;getOrInsertFunction("main", // Called "main"
++                                                           Type::getInt32Ty(C), // Returns an int (i32)
++                                                           (Type *)0) // Takes no arguments
++                                                           )
++   // Create the entry block
++   BasicBlock *BB = BasicBlock::Create(C,
++                                       "EntryBlock", // Conventionnaly called "EntryBlock"
++                                       MainF // Add it to "main" function
++                                       );
++   IRBuilder&lt;&gt; B(BB); // Create a builder to add instructions
++   B.SetInsertPoint(BB); // Insert the block to function

++   // Generate IR code from parser
++   parser.CodeGen(M, B);

++   B.CreateRet(B.getInt32(0)); // Return 0 to the "main" function

++   // Print (dump) the module
++   M-&gt;dump();

++   // Default initialisation
++   InitializeNativeTarget();
++   InitializeNativeTargetAsmPrinter();
++   InitializeNativeTargetAsmParser();

++   // Create the execution engine
++   std::string ErrStr;
++   EngineBuilder *EB = new EngineBuilder(std::move(Owner));
++   ExecutionEngine *EE = EB-&gt;setErrorStr(&ErrStr)
++     .setMCJITMemoryManager(std::unique_ptr&lt;SectionMemoryManager&gt;(new SectionMemoryManager()))
++     .create();

++   if (!ErrStr.empty()) {
++     std::cout &lt;&lt; ErrStr &lt;&lt; "\n";
++     exit(0);
++   }

++   // Finalize the execution engine before use it
++   EE-&gt;finalizeObject();

++   // Run the program
++   std::vector&lt;GenericValue&gt; Args(0); // No args
++   EE-&gt;runFunction(MainF, // Called "main",
++                   Args); // with no arguments

++   std::cout &lt;&lt; "\n" &lt;&lt; "=== Program Output ===" &lt;&lt; "\n";
++   std::vector&lt;GenericValue&gt; Args(0); // No args
++   EE-&gt;runFunction(MainF, Args);

++   // Clean up and shutdown
++   delete EE;
++   llvm_shutdown();
  
  return 0;
}
</pre>

[Code source](./BF \(Parser + Compiler LLVM\)/)

To compile this:
<pre>
clang++ -Wall Expr.cpp CodeGen.cpp Parser.cpp BrainF.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native nativecodegen` DebugDescription.cpp -o BrainF
</pre>

Or with a Makefile format:
<pre>
CC=clang++
CFLAGS=-Wall # Display all warning
SRCS=Expr.cpp CodeGen.cpp Parser.cpp BrainF.cpp DebugDescription.cpp
TARGET= BrainF
CONFIG=`llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native nativecodegen`

all: build

build: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(CONFIG) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)
</pre>

Then run our command:
<pre>
$ make build run
</pre>

At last, we've got our "hello, world!" from our LLVM compiler!
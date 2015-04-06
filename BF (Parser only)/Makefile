all: build

build: Expr.cpp CodeGen.cpp Parser.cpp BrainF.cpp DebugDescription.cpp
	clang++ Expr.cpp CodeGen.cpp Parser.cpp DebugDescription.cpp BrainF.cpp -o BrainF

run: BrainF
	./BrainF
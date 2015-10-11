CC=clang++
CFLAGS=-O3 -Wall
SRCS=Expr.cpp CodeGen.cpp Parser.cpp BrainF.cpp DebugDescription.cpp
TARGET= BrainF
CONFIG=`llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native nativecodegen`

all: build

build: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(CONFIG) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)
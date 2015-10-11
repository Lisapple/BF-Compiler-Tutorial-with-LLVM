#include "CodeGen.h"

using namespace llvm;

static GlobalVariable *__BrainF_IndexPtr = NULL;
static GlobalVariable *__BrainF_CellsPtr = NULL;

void Parser::CodeGen(Module *M, IRBuilder<> &B)
{
  LLVMContext &C = M->getContext();
  
  if (!__BrainF_IndexPtr) {
    // Create global variable |brainf.index|
    Type *Ty = Type::getInt32Ty(C);
    const APInt Zero = APInt(32, 0); // int32 0
    Constant *InitV = Constant::getIntegerValue(Ty, Zero);
    __BrainF_IndexPtr = new GlobalVariable(*M, Ty, false /* non-constant */,
                                           GlobalValue::WeakAnyLinkage, // Keep one copy when linking (weak)
                                           InitV, "brainf.index");
  }
  
  if (!__BrainF_CellsPtr) {
#define kCellsCount 100
    // Create |brainf.cells|
    ArrayType *ArrTy = ArrayType::get(Type::getInt32Ty(C), kCellsCount);
    std::vector<Constant *> constants(kCellsCount, B.getInt32(0)); // Create a vector of kCellsCount items equal to 0
    ArrayRef<Constant *> Constants = ArrayRef<Constant *>(constants);
    Constant *InitPtr = ConstantArray::get(ArrTy, Constants);
    __BrainF_CellsPtr = new GlobalVariable(*M, ArrTy, false /* non-constant */,
                                           GlobalValue::WeakAnyLinkage, // Keep one copy when linking (weak)
                                           InitPtr, "brainf.cells");
  }
  
  // Recursively generate code
  for (std::vector<Expr *>::iterator it = _exprs.begin(); it != _exprs.end(); ++it) {
    (*it)->CodeGen(M, B);
  }
}

void ShiftExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  // Load index value
  Value *IdxV = B.CreateLoad(__BrainF_IndexPtr);
  // Add |_step| to index and save the value
  B.CreateStore(B.CreateAdd(IdxV, B.getInt32(_step)),
                __BrainF_IndexPtr);
}


void IncrementExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  Value* Idxs[] = { B.getInt32(0), B.CreateLoad(__BrainF_IndexPtr) };
  ArrayRef<Value *> IdxsArr(Idxs);
  Value *CellPtr = B.CreateGEP(__BrainF_CellsPtr, IdxsArr);
  // Load cell value
  Value *CellV = B.CreateLoad(CellPtr);
  // Add |_step| to cell value and save the value
  B.CreateStore(B.CreateAdd(CellV, B.getInt32(_increment)),
                CellPtr);
}


void InputExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  LLVMContext &C = M->getContext();
  
  // @TODO: Print "> "
  
  // Get "scanf" function
  // i32 @scanf(i8*, ...)
  Type* ScanfArgs[] = { Type::getInt8PtrTy(C) };
  FunctionType *ScanfTy = FunctionType::get(Type::getInt32Ty(C), ScanfArgs, true /* vaarg */);
  Function *ScanfF = cast<Function>(M->getOrInsertFunction("scanf", ScanfTy));
  
  // Prepare args
  static Value *GScanfFormat = NULL;
  if (!GScanfFormat) {
    GScanfFormat = B.CreateGlobalString("%d", "brainf.scanf.format");
  }
  Value *IntPtr = B.CreateAlloca(Type::getInt32Ty(C));
  
  // Call "scanf"
  Value* Args[] = { CastToCStr(GScanfFormat, B), IntPtr };
  ArrayRef<Value *> ArgsArr(Args);
  B.CreateCall(ScanfF, ArgsArr);
	
  Value *IdxV = B.CreateLoad(__BrainF_IndexPtr);
  Value *CellPtr = B.CreateGEP(B.CreatePointerCast(__BrainF_CellsPtr,
                                                   Type::getInt32Ty(C)->getPointerTo()), // Cast to int32*
                               IdxV);
  // Save the new value to current cell
  B.CreateStore(B.CreateLoad(IntPtr), CellPtr);
}


void OutputExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  LLVMContext &C = M->getContext();
  
  // Get "printf" function
  // i32 @printf(i8*, ...)
  Type* PrintfArgs[] = { Type::getInt8PtrTy(C) };
  FunctionType *PrintfTy = FunctionType::get(Type::getInt32Ty(C), PrintfArgs, true /* vaarg */);
  Function *PrintfF = cast<Function>(M->getOrInsertFunction("printf", PrintfTy));
  
  // Prepare args
  static Value *GPrintfFormat = NULL;
  if (!GPrintfFormat) {
    GPrintfFormat = B.CreateGlobalString("%c\n", "brainf.printf.format");
  }
  Value *IdxV = B.CreateLoad(__BrainF_IndexPtr);
  Value *CellPtr = B.CreateGEP(B.CreatePointerCast(__BrainF_CellsPtr,
                                                   Type::getInt32Ty(C)->getPointerTo()), // Cast to int32*
                               IdxV);
  
  // Call "printf"
  Value* Args[] = { CastToCStr(GPrintfFormat, B), B.CreateLoad(CellPtr) };
  ArrayRef<Value *> ArgsArr(Args);
  B.CreateCall(PrintfF, ArgsArr);
}


void LoopExpr::CodeGen(Module *M, IRBuilder<> &B)
{
  LLVMContext &C = M->getContext();
  
  // Create a basic block for loop
  Function *F = B.GetInsertBlock()->getParent();
  BasicBlock *StartBB = BasicBlock::Create(C, "LoopStart", F);
  B.CreateBr(StartBB);
  
  B.SetInsertPoint(StartBB);
  IRBuilder<> StartB(StartBB);
  
  // Enter the block ("LoopBody") if current cell value > 0, else skip the loop (i.e.: go to "LoopEnd")
  BasicBlock *LoopBB = BasicBlock::Create(C, "LoopBody", F);
  BasicBlock *EndBB = BasicBlock::Create(C, "LoopEnd", F);
  
  // Get the current cell adress
  Value *IdxV = B.CreateLoad(__BrainF_IndexPtr);
  Value *CellPtr = B.CreateGEP(B.CreatePointerCast(__BrainF_CellsPtr,
                                                   Type::getInt32Ty(C)->getPointerTo()), // Cast to int32*
                               IdxV);
  Value *SGZeroCond = StartB.CreateICmpSGT(StartB.CreateLoad(CellPtr),
                                           StartB.getInt32(0)); // is cell Signed Int Greater than Zero?
  StartB.CreateCondBr(SGZeroCond, LoopBB, EndBB);
  
  B.SetInsertPoint(LoopBB);
  IRBuilder<> LoopB(LoopBB);
  // Recursively generate code (into "LoopBody" block)
  for (std::vector<Expr *>::iterator it = _exprs.begin(); it != _exprs.end(); ++it) {
    (*it)->CodeGen(M, LoopB);
  }
  LoopB.CreateBr(StartBB); // Restart loop (will next exit if current cell value > 0)
  
  B.SetInsertPoint(EndBB);
}
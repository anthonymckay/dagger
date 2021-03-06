//===-- llvm/DC/DCModule.h - Module Translation -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines DCModule, the main interface that can be used to
// translate module-level constructs from an MCModule to an IR Module.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DC_DCMODULE_H
#define LLVM_DC_DCMODULE_H

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/DC/DCTranslator.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>
#include <string>

namespace llvm {
class FunctionType;
class Value;

class DCModule {
public:
  DCModule(DCTranslator &DCT, Module &M);
  virtual ~DCModule();

  Function *createExternalWrapperFunction(uint64_t Addr, Value *ExtFn);
  Function *createExternalWrapperFunction(uint64_t Addr, StringRef Name);
  Function *createExternalWrapperFunction(uint64_t Addr);

  Function *getOrCreateMainFunction(Function *EntryFn);
  Function *getOrCreateInitRegSetFunction();
  Function *getOrCreateFiniRegSetFunction();

  // Returns the regset diff function, that prints to stderr:
  //     void @__llvm_dc_print_regset_diff(i8* fn, %regset* v1, %regset* v2)
  Function *getOrCreateRegSetDiffFunction();

  std::string getFunctionName(uint64_t Addr);
  Function *getOrCreateFunction(uint64_t Addr);

  DCTranslator &getTranslator() { return DCT; }
  Module *getModule() { return &TheModule; }
  LLVMContext &getContext() { return getModule()->getContext(); }

  FunctionType *getFuncTy() { return &FuncTy; }

  /// Debug Info Support.
  /// @{
  /// Get the output stream for the emitted debug source file.
  /// When debug info is enabled, we synthesize a source file for debug info
  /// purposes, by printing assembly to it.  This lets us reference machine
  /// code in the debug info of the translated IR.
  raw_ostream *getDebugStream() {
    return DebugStream ? DebugStream.getPointer() : nullptr;
  }

  /// Get the debug info builder.
  DIBuilder *getDebugBuilder() { return DebugBuilder.get(); }

  /// Get the DIFile for the debug source file we emit for this module.
  DIFile *getDebugFile() { return DebugFile; }

  /// Get the type of translated void(%regset*) functions.
  DISubroutineType *getDebugFunctionTy() { return DebugFnTy; }

  /// Increment the current line number in the emitted debug source file,
  /// returning the previous line number.
  unsigned incrementDebugLine();
  /// @}

private:
  /// Initialize the output stream for the emitted debug source file.
  /// When debug info is enabled, we synthesize a source file for debug info
  /// purposes, by printing assembly to it.  This lets us reference machine
  /// code in the debug info of the translated IR.
  void initializeDebugInfo(StringRef OutputDirectory);

protected:
  /// Insert, at the end of basic block \p InsertAtEnd, the target-specific ABI
  /// code for initializing the register set with the dynamic environment, as
  /// if immediately before calling the 'main' function.
  virtual void insertCodeForInitRegSet(BasicBlock *InsertAtEnd, Value *RegSet,
                                       Value *StackPtr, Value *StackSize,
                                       Value *ArgC, Value *ArgV) = 0;

  /// Insert, at the end of basic block \p InsertAtEnd, the target-specific ABI
  /// code for extracting the 'main' i32 result from the regset.
  virtual Value *insertCodeForFiniRegSet(BasicBlock *InsertAtEnd,
                                         Value *RegSet) = 0;

  /// Insert, at the end of basic block \p InsertAtEnd, the target-specific
  /// context-switching code to expose the register set \p RegSet to the native
  /// external function \p ExternalFunc, call it, and return to the translated
  /// code, saving the native register state to \p RegSet.
  /// This is only well-defined when translating to the same target as the
  /// original code, but can be made to work in limited cross-translation cases.
  virtual void insertExternalWrapperAsm(BasicBlock *InsertAtEnd,
                                        Value *ExternalFunc, Value *RegSet) = 0;

private:
  DCTranslator &DCT;
  Module &TheModule;
  FunctionType &FuncTy;

  /// Debug Info State.
  /// @}
  /// The output stream for the emitted debug source file.
  Optional<raw_fd_ostream> DebugStream;

  /// The current line number in the emitted debug source file.
  unsigned DebugLine = 1;

  /// The debug info builder.
  std::unique_ptr<DIBuilder> DebugBuilder;

  /// The DIFile for the debug source file we emit for this module.
  DIFile *DebugFile = nullptr;

  /// The type of translated void(%regset*) functions.
  DISubroutineType *DebugFnTy = nullptr;
  /// @}
};

} // end namespace llvm

#endif

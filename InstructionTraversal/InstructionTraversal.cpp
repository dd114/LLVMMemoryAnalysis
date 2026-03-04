#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

// This method implements what the pass does
// void visitor(Function &F) {
//     errs() << "Hello from: "<< F.getName() << "\n";
//     errs() << "number of arguments: " << F.arg_size() << "\n";
// }

void makeLogs(Function &F) {
  errs() << "\nFunction name: "<< F.getName() << "; "<< "Number of arguments: " << F.arg_size() << "\n";

  for (auto &BB : F) {


    for (auto &Inst : BB) {
      
      // 1. Broad check: Does this instruction interact with memory at all?
      if (Inst.mayReadOrWriteMemory()) {
          
          // Print the instruction's opcode name (e.g., "load", "store", "call")
          // outs() << "Name Op is " << Inst.getName() << "\n";
          outs() << "Memory Op [" << Inst.getOpcodeName() << "]: ";
          
          // 2. Extract specific details for standard Load/Store
          if (auto *LI = dyn_cast<LoadInst>(&Inst)) {
              outs() << "Reads from: " << *LI->getPointerOperand() 
                    << " (Align: " << LI->getAlign().value() << ")";
          } 
          else if (auto *SI = dyn_cast<StoreInst>(&Inst)) {
              outs() << "Writes " << *SI->getValueOperand() 
                    << " to: " << *SI->getPointerOperand() 
                    << " (Align: " << SI->getAlign().value() << ")";
          } 
          else {
              // Handles other memory ops like 'call', 'atomicrmw', etc.
              outs() << "Complex memory interaction (e.g. Call/Atomic)";
          }
          
          outs() << "\n";
      }

    }
  }
}

// New PM implementation
struct InstructionTraversal : PassInfoMixin<InstructionTraversal> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    // visitor(F);
    makeLogs(F);

    return PreservedAnalyses::all();
  }

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};
} // namespace



//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getMyPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "InstructionTraversal", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "InstructionTraversal") {
                    FPM.addPass(InstructionTraversal());
                    return true;
                  }
                  return false;
                });
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize HelloWorld when added to the pass pipeline on the
// command line, i.e. via '-passes=hello-world'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getMyPassPluginInfo();
}
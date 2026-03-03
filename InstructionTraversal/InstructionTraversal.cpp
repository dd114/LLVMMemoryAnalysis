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
      // Changed |= runOnBasicBlock(BB);

    errs() << "===============================\n";
    errs() << BB.getName() << " has " << BB.size() << " instructions.\n";
    errs() << BB.getParent()->getName() << " is the parent function of " << BB.getName() << "\n";
    errs() << BB.getModule()->getName() << " is the parent module of " << BB.getName() << "\n";
    errs() << BB.getModule()->getSourceFileName() << " is the source file name of " << BB.getName() << "\n";
    errs() << BB.getModule()->getDataLayoutStr() << " is the data layout of " << BB.getName() << "\n";
    errs() << BB.getModule()->getTargetTriple().str() << " is the target triple of " << BB.getName() << "\n";
    errs() << BB.getNextNode() << " is the next node of " << BB.getName() << "\n";
    errs() << BB.getPrevNode() << " is the previous node of " << BB.getName() << "\n";
    errs() << BB.getModule()->getFunctionList().size() << " is the number of functions in the module of " << BB.getName() << "\n";
    // errs() << BB.

    errs() << "\n-------------------------------\n";

    for (auto &Inst : BB) {
      errs() << "Instruction: " << Inst << "\n";
      errs() << "Inst.getOpcodeName(): " << Inst.getOpcodeName() << "\n";
      errs() << "Inst.getNumOperands(): " << Inst.getNumOperands() << "\n";
      errs() << "Inst.getType(): " << *Inst.getType() << "\n";
      errs() << "Inst.getOpcode(): " << Inst.getOpcode() << "\n";
      errs() << "Inst.getNumOperands(): " << Inst.getNumOperands() << "\n";
      errs() << "Inst.getOperandList(): " << Inst.getOperandList() << "\n";
      errs() << "Inst.getOperand(0): " << Inst.getOperand(0) << "\n";
      errs() << "Inst.getOperand(0)->getName(): " << Inst.getOperand(0)->getName() << "\n";
      errs() << "Inst.getOperand(0)->getType(): " << *Inst.getOperand(0)->getType() << "\n";
      errs() << "Inst.getOperand(0)->getType()->isIntegerTy(): " << Inst.getOperand(0)->getType()->isIntegerTy() << "\n";
      errs() << "Inst.getOperand(0)->getType()->isFloatingPointTy(): " << Inst.getOperand(0)->getType()->isFloatingPointTy() << "\n";
      errs() << "Inst.getOperand(0)->getType()->isPointerTy(): " << Inst.getOperand(0)->getType()->isPointerTy() << "\n";
      errs() << "Inst.getModule()->getName(): " << Inst.getModule()->getName() << "\n";
      errs() << "Inst.getFunction()->getName(): " << Inst.getFunction()->getName() << "\n";
      errs() << "Inst.getParent()->getName(): " << Inst.getParent()->getName() << "\n";
      errs() << "Inst.getDebugLoc(): " << Inst.getDebugLoc() << "\n";
      // errs() << "Inst.getDebugLoc().getLine(): " << Inst.getDebugLoc().getLine() << "\n"; // crush
      // errs() << "Inst.getDebugLoc().getCol(): " << Inst.getDebugLoc().getCol() << "\n"; // crush
      // errs() << "Inst.getDebugLoc().getScope(): " << Inst.getDebugLoc().getScope() << "\n"; // crush

      // errs() << "Inst.getAllMetadata(): " << Inst.getAllMetadata(BB).size() << "\n";

      errs() << "-------------------------------\n";
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
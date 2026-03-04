#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

void makeLogs(Function &F) {
  const DataLayout &DL = F.getParent()->getDataLayout();

  outs() << "\n=============================\n";
  outs() << "Function: " << F.getName() << "; "<< "Number of arguments: " << F.arg_size() << "\n";
  outs() << "=============================\n";

  for (auto &BB : F) {
      for (auto &Inst : BB) {

          if (!Inst.mayReadOrWriteMemory() && !isa<AllocaInst>(&Inst) && !isa<GetElementPtrInst>(&Inst)){
                errs() << "Not memory interaction" << "\n";
                errs() << "Opcode: " << Inst.getOpcodeName() << "\n";
                errs() << "Full IR: ";
                Inst.print(errs());
                errs() << "\n---------------------------\n";
                continue;
              }

          outs() << "\n=== Memory Instruction ===\n";
          outs() << "Opcode: " << Inst.getOpcodeName() << "\n";

          outs() << "Full IR: ";
          Inst.print(outs());
          outs() << "\n";

          // ---------- LOAD ----------
          if (auto *LI = dyn_cast<LoadInst>(&Inst)) {

              Value *Ptr = LI->getPointerOperand();
              Type *Ty = LI->getType();

              outs() << "Kind: Load\n";

              outs() << "Pointer: ";
              Ptr->print(outs());
              outs() << "\n";

              outs() << "Value Type: ";
              Ty->print(outs());
              outs() << "\n";

              uint64_t Size = DL.getTypeStoreSize(Ty);
              outs() << "Access Size: " << Size << " bytes\n";

              outs() << "Alignment: "
                      << LI->getAlign().value() << "\n";

              PointerType *PT =
                  cast<PointerType>(Ptr->getType());
              outs() << "Address Space: "
                      << PT->getAddressSpace() << "\n";

              if (LI->isVolatile())
                  outs() << "Volatile: yes\n";

              if (LI->isAtomic())
                  outs() << "Atomic Ordering: "
                          << static_cast<int>(LI->getOrdering()) << "\n";
          }

          // ---------- STORE ----------
          if (auto *SI = dyn_cast<StoreInst>(&Inst)) {

              Value *Ptr = SI->getPointerOperand();
              Value *Val = SI->getValueOperand();
              Type *Ty = Val->getType();

              outs() << "Kind: Store\n";

              outs() << "Pointer: ";
              Ptr->print(outs());
              outs() << "\n";

              outs() << "Stored Type: ";
              Ty->print(outs());
              outs() << "\n";

              uint64_t Size = DL.getTypeStoreSize(Ty);
              outs() << "Access Size: " << Size << " bytes\n";

              outs() << "Alignment: "
                      << SI->getAlign().value() << "\n";

              PointerType *PT =
                  cast<PointerType>(Ptr->getType());
              outs() << "Address Space: "
                      << PT->getAddressSpace() << "\n";

              if (SI->isVolatile())
                  outs() << "Volatile: yes\n";

              if (SI->isAtomic())
                  outs() << "Atomic Ordering: "
                          << static_cast<int>(SI->getOrdering()) << "\n";
          }

          // ---------- ALLOCA ----------
          if (auto *AI = dyn_cast<AllocaInst>(&Inst)) {

              outs() << "Kind: Alloca\n";

              outs() << "Allocated Type: ";
              AI->getAllocatedType()->print(outs());
              outs() << "\n";

              outs() << "Alignment: "
                      << AI->getAlign().value() << "\n";

              outs() << "Address Space: "
                      << AI->getAddressSpace() << "\n";
          }

          // ---------- GEP ----------
          if (auto *GEP = dyn_cast<GetElementPtrInst>(&Inst)) {

              outs() << "Kind: GetElementPtr\n";

              outs() << "Base Pointer: ";
              GEP->getPointerOperand()->print(outs());
              outs() << "\n";

              outs() << "Result Type: ";
              GEP->getType()->print(outs());
              outs() << "\n";
          }

          // ---------- CALL ----------
          if (auto *CB = dyn_cast<CallBase>(&Inst)) {

              outs() << "Kind: Call\n";

              if (Function *Callee =
                  CB->getCalledFunction()) {
                  outs() << "Callee: "
                          << Callee->getName() << "\n";
              } else {
                  outs() << "Indirect Call\n";
              }

              outs() << "Memory Effects: (see call intrinsics)\n";

              if (CB->onlyReadsMemory())
                  outs() << "Only Reads Memory\n";

              if (CB->onlyWritesMemory())
                  outs() << "Only Writes Memory\n";

              if (CB->doesNotAccessMemory())
                  outs() << "No Memory Access\n";
          }

          // ---------- ATOMIC RMW ----------
          if (auto *ARMW =
              dyn_cast<AtomicRMWInst>(&Inst)) {

              outs() << "Kind: AtomicRMW\n";
              outs() << "Operation: "
                      << ARMW->getOperation() << "\n";
          }

          // ---------- CMPXCHG ----------
          if (auto *CX =
              dyn_cast<AtomicCmpXchgInst>(&Inst)) {

              outs() << "Kind: AtomicCmpXchg\n";
              outs() << "Success Ordering: "
                      << static_cast<int>(CX->getSuccessOrdering()) << "\n";
              outs() << "Failure Ordering: "
                      << static_cast<int>(CX->getFailureOrdering()) << "\n";
          }

          // ---------- METADATA ----------
          SmallVector<std::pair<unsigned, MDNode *>, 8> MDs;
          Inst.getAllMetadata(MDs);

          if (!MDs.empty()) {
              outs() << "Metadata:\n";
              for (auto &MD : MDs) {
                  outs() << "  Kind ID: "
                          << MD.first << "\n";
              }
          }

          outs() << "---------------------------\n";
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
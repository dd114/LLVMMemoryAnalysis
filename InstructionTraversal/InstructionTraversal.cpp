#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

void makeLogs(Function &F, llvm::MemorySSA &MSSA) {
  const DataLayout &DL = F.getParent()->getDataLayout();

  outs() << "\n=============================\n";
  outs() << "Function: " << F.getName()
         << "; Number of arguments: " << F.arg_size() << "\n";
  outs() << "=============================\n";

  for (auto &BB : F) {
      for (auto &Inst : BB) {

          if (!Inst.mayReadOrWriteMemory() &&
              !isa<AllocaInst>(&Inst) &&
              !isa<GetElementPtrInst>(&Inst)) {

                errs() << "Not memory interaction\n";
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

          outs() << "Parent BB: ";
          Inst.getParent()->printAsOperand(outs(), false);
          outs() << "\n";


    // ====== MEMORYSSA INFORMATION

          MemoryAccess *MA = MSSA.getMemoryAccess(&Inst);

          if (MA) {

              outs() << "\n--- MemorySSA ---\n";

              if (auto *MU = dyn_cast<MemoryUse>(MA)) {

                  outs() << "MemorySSA Node: MemoryUse\n";

                  MemoryAccess *Def = MU->getDefiningAccess();

                  outs() << "Depends on: ";
                  if (Def)
                      Def->print(outs());
                  else
                      outs() << "null";

                  outs() << "\n";

              } else if (auto *MD = dyn_cast<MemoryDef>(MA)) {

                  outs() << "MemorySSA Node: MemoryDef\n";

                  MemoryAccess *Def = MD->getDefiningAccess();

                  outs() << "Previous Def: ";
                  if (Def)
                      Def->print(outs());
                  else
                      outs() << "null";

                  outs() << "\n";

              } else if (auto *MP = dyn_cast<MemoryPhi>(MA)) {

                  outs() << "MemorySSA Node: MemoryPhi\n";

                  outs() << "Incoming values:\n";

                  for (unsigned i = 0;
                       i < MP->getNumIncomingValues(); ++i) {

                      outs() << "  From BB ";

                      MP->getIncomingBlock(i)
                          ->printAsOperand(outs(), false);

                      outs() << " : ";

                      MP->getIncomingValue(i)->print(outs());

                      outs() << "\n";
                  }
              }

              /* ---------- USERS OF MEMORY ACCESS ---------- */

              outs() << "MemorySSA Users:\n";

              for (auto *U : MA->users()) {

                  outs() << "  ";

                  if (auto *MAU = dyn_cast<MemoryUseOrDef>(U)) {
                      if (auto *I = MAU->getMemoryInst()) {

                          I->print(outs());

                      } else {

                          MAU->print(outs());

                      }
                  } else {

                      U->print(outs());

                  }

                  outs() << "\n";
              }

              /* ---------- WALK DEF CHAIN ---------- */

              outs() << "Memory Def Chain:\n";

              MemoryAccess *Walker = MA;

              int depth = 0;

              while (Walker && depth < 5) {

                  outs() << "  ";

                  Walker->print(outs());

                  outs() << "\n";

                  if (auto *UD =
                      dyn_cast<MemoryUseOrDef>(Walker))
                      Walker = UD->getDefiningAccess();
                  else
                      break;

                  depth++;
              }

              outs() << "----------------------\n";
          }
          else {
              outs() << "No MemorySSA node\n";
          }


    // ====== INSTRUCTION-SPECIFIC INFORMATION

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

              PointerType *PT = cast<PointerType>(Ptr->getType());

              outs() << "Address Space: "
                     << PT->getAddressSpace() << "\n";

              if (LI->isVolatile())
                  outs() << "Volatile: yes\n";

              if (LI->isAtomic())
                  outs() << "Atomic Ordering: "
                         << static_cast<int>(LI->getOrdering())
                         << "\n";
          }

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

              outs() << "Access Size: "
                     << Size << " bytes\n";

              outs() << "Alignment: "
                     << SI->getAlign().value() << "\n";

              PointerType *PT = cast<PointerType>(Ptr->getType());

              outs() << "Address Space: "
                     << PT->getAddressSpace() << "\n";

              if (SI->isVolatile())
                  outs() << "Volatile: yes\n";

              if (SI->isAtomic())
                  outs() << "Atomic Ordering: "
                         << static_cast<int>(SI->getOrdering())
                         << "\n";
          }

          outs() << "---------------------------\n";
      }
  }
}

// New PM implementation
struct InstructionTraversal : PassInfoMixin<InstructionTraversal> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    // visitor(F);

    auto &MSSA = FAM.getResult<MemorySSAAnalysis>(F).getMSSA();

    makeLogs(F, MSSA);

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
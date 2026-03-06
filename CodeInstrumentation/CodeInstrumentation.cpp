#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>

using namespace llvm;

namespace {

struct MemoryOpInfo {
    Instruction *Inst;

    enum OpType {
        LOAD,
        STORE,
        ALLOCA
    } Type;

    Value *Pointer;
    llvm::Type *ValueType;
    uint64_t Size;
    uint64_t Align;
};

GlobalVariable *getOrCreateFormat(Module *M,
                                  const std::string &Format,
                                  const std::string &Name) {

    if (auto *G = M->getNamedGlobal(Name))
        return G;

    LLVMContext &Ctx = M->getContext();

    Constant *Str = ConstantDataArray::getString(Ctx, Format);

    auto *GV = new GlobalVariable(
        *M,
        Str->getType(),
        true,
        GlobalValue::PrivateLinkage,
        Str,
        Name
    );

    GV->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);

    return GV;
}

Value *getStringPtr(IRBuilder<> &Builder, GlobalVariable *GV) {

    LLVMContext &Ctx = Builder.getContext();

    Value *Zero = ConstantInt::get(Type::getInt32Ty(Ctx), 0);

    return Builder.CreateInBoundsGEP(
        GV->getValueType(),
        GV,
        {Zero, Zero}
    );
}

void instrumentFunction(Function &F) {

    Module *M = F.getParent();
    LLVMContext &Ctx = M->getContext();
    const DataLayout &DL = M->getDataLayout();

    outs() << "\n=== Function: " << F.getName() << " ===\n";

    Function *Printf = M->getFunction("printf");

    if (!Printf) {

      outs() << "Function creation\n";

        FunctionType *PrintfTy =
            FunctionType::get(
                Type::getInt32Ty(Ctx),
                {PointerType::getUnqual(Ctx)},
                true
            );

        Printf = Function::Create(
            PrintfTy,
            GlobalValue::ExternalLinkage,
            "printf",
            M
        );
    }

    auto *LoadFmt = getOrCreateFormat(
        M,
        "LOAD addr=%p type=%s size=%lu align=%lu\n",
        "log_load_fmt"
    );

    auto *StoreFmt = getOrCreateFormat(
        M,
        "STORE addr=%p type=%s size=%lu align=%lu\n",
        "log_store_fmt"
    );

    auto *AllocaFmt = getOrCreateFormat(
        M,
        "ALLOCA addr=%p type=%s size=%lu align=%lu\n",
        "log_alloca_fmt"
    );

    std::vector<MemoryOpInfo> Ops;

    for (auto &BB : F) {
        for (auto &I : BB) {

            MemoryOpInfo Info;

            if (auto *LI = dyn_cast<LoadInst>(&I)) {

                Info.Inst = LI;
                Info.Type = MemoryOpInfo::LOAD;
                Info.Pointer = LI->getPointerOperand();
                Info.ValueType = LI->getType();
                Info.Size = DL.getTypeStoreSize(Info.ValueType);
                Info.Align = LI->getAlign().value();

                Ops.push_back(Info);
            }

            else if (auto *SI = dyn_cast<StoreInst>(&I)) {

                Info.Inst = SI;
                Info.Type = MemoryOpInfo::STORE;
                Info.Pointer = SI->getPointerOperand();
                Info.ValueType = SI->getValueOperand()->getType();
                Info.Size = DL.getTypeStoreSize(Info.ValueType);
                Info.Align = SI->getAlign().value();

                Ops.push_back(Info);
            }

            else if (auto *AI = dyn_cast<AllocaInst>(&I)) {

                Info.Inst = AI;
                Info.Type = MemoryOpInfo::ALLOCA;
                Info.Pointer = AI;
                Info.ValueType = AI->getAllocatedType();
                Info.Size = DL.getTypeAllocSize(Info.ValueType);
                Info.Align = AI->getAlign().value();

                Ops.push_back(Info);
            }
        }
    }

    outs() << "Memory ops found: " << Ops.size() << "\n";

    for (auto &Info : Ops) {

        Instruction *InsertPoint = Info.Inst;

        if (Info.Type == MemoryOpInfo::STORE || Info.Type == MemoryOpInfo::ALLOCA || Info.Type == MemoryOpInfo::LOAD) { // for any cases now

            if (Instruction *Next = Info.Inst->getNextNode())
                InsertPoint = Next;
        }

        IRBuilder<> Builder(InsertPoint);

        GlobalVariable *FmtGV =
            Info.Type == MemoryOpInfo::LOAD  ? LoadFmt :
            Info.Type == MemoryOpInfo::STORE ? StoreFmt :
                                               AllocaFmt;

        Value *FmtPtr = getStringPtr(Builder, FmtGV);

        Value *PtrCast =
            Builder.CreatePointerCast(
                Info.Pointer,
                PointerType::getUnqual(Ctx)
            );
       
            
        std::string TypeStr;
        llvm::raw_string_ostream RSO(TypeStr);
        Info.ValueType->print(RSO);

        Value *TypeStrVal = Builder.CreateGlobalString(TypeStr);


        Value *SizeVal =
            ConstantInt::get(
                Type::getInt64Ty(Ctx),
                Info.Size
            );

        Value *AlignVal =
            ConstantInt::get(
                Type::getInt64Ty(Ctx),
                Info.Align
            );

        Builder.CreateCall(
            Printf,
            {FmtPtr, PtrCast, TypeStrVal, SizeVal, AlignVal}
        );
    }

    verifyFunction(F, &errs());
}

struct CodeInstrumentation :
    PassInfoMixin<CodeInstrumentation> {

    PreservedAnalyses run(Function &F,
                          FunctionAnalysisManager &) {

        instrumentFunction(F);

        return PreservedAnalyses::none();
    }

    static bool isRequired() { return true; }
};

} 

PassPluginLibraryInfo getMyPassPluginInfo() {

    return {
        LLVM_PLUGIN_API_VERSION,
        "CodeInstrumentation",
        LLVM_VERSION_STRING,

        [](PassBuilder &PB) {

            PB.registerPipelineParsingCallback(

                [](StringRef Name,
                   FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {

                    if (Name == "CodeInstrumentation") {

                        FPM.addPass(CodeInstrumentation());
                        return true;
                    }

                    return false;
                });
        }
    };
}

extern "C"
LLVM_ATTRIBUTE_WEAK
PassPluginLibraryInfo llvmGetPassPluginInfo() {

    return getMyPassPluginInfo();
}
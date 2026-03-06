# LLVMMemoryAnalysis

## Build project:

```bash
export LLVM_DIR=/usr/lib/llvm-22 # <installation/dir/of/llvm/22>

mkdir build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build -S . -GNinja # -S InstructionTraversal is also possible
ninja -C build
```

## Create auxiliary folder and LLVM IR file
```bash
mkdir -p output
cd output

$LLVM_DIR/bin/clang -O0 -S -emit-llvm ../inputExamples/inputTraversal1.c -o inputTraversal1.ll
```

## Apply plugin of analysis pass to LLVM IR
```bash
$LLVM_DIR/bin/opt -load-pass-plugin ../build/lib/libInstructionTraversal.so -passes=InstructionTraversal -disable-output inputTraversal1.ll 1> memPrint1.txt 2> notMemPrint1.txt
```

## Apply plugin of transformation pass to LLVM IR
```bash
$LLVM_DIR/bin/opt -load-pass-plugin ../build/lib/libCodeInstrumentation.so -passes=CodeInstrumentation -S inputTraversal1.ll -o codeInstrumentation1.ll 1> memPrint2.txt 2> notMemPrint2.txt
```

#### Compile LLVM IR code
```bash
$LLVM_DIR/bin/lli codeInstrumentation1.ll
```

#### Interpreter LLVM IR code
```bash
$LLVM_DIR/bin/clang codeInstrumentation1.ll -o codeInstrumentation1
```

---

## Future plans
* ~~Add or rewrite through [MemorySSA](https://llvm.org/docs/MemorySSA.html) analysis~~
* Potentially add Alias analysis
* Potentially add MemoryLocation analysis
* Potentially add ScalarEvolution analysis
* ~~Add runtime address memory info printing through IRBuilder code instrumentation (in progress...)~~
* Mb rewrite potentially to fancy Rust
* ...
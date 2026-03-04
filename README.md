# LLVMMemoryAnalysis

## Build project:

```bash
export LLVM_DIR=/usr/lib/llvm-22/ # <installation/dir/of/llvm/22>

mkdir build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build -S . -GNinja # -S InstructionTraversal is also possible
ninja -C build
```

## Apply plugin to LLVM IR
```bash
mkdir -p output
cd output

$LLVM_DIR/bin/clang -O0 -S -emit-llvm ../inputExamples/inputTraversal1.c -o inputTraversal1.ll
$LLVM_DIR/bin/opt -load-pass-plugin ../build/lib/libInstructionTraversal.so -passes=InstructionTraversal -disable-output inputTraversal1.ll 1> memPrint1.txt 2> notMemPrint2.txt
```


## Future
* Add or rewrite through [MemorySSA](https://llvm.org/docs/MemorySSA.html) analysis
* Add address memory info printing (is not working correctly yet)
* Mb rewrite potentially to Rust
* ...
## Write LLVM Plugin

To create an LLVM Plugin, follow these steps:

1. **Write the Pass Code**: Below is the breakdown of an LLVM Example Pass written in C++ (check source [here](MultiplicationShifts/MultiplicationShifts.cpp)). This pass replaces multiplication operations where the second operand is a constant power of two with a corresponding left shift operation.

    - Include necessary header files. For this pass, we'll need these:
        ```cpp
        #include "llvm/Pass.h"
        #include "llvm/IR/Function.h"
        #include "llvm/IR/Instructions.h"
        #include "llvm/IR/IRBuilder.h"
        #include "llvm/Support/raw_ostream.h"
        #include "llvm/Passes/PassBuilder.h"
        #include "llvm/Passes/PassPlugin.h"
        ```
    - Create a struct. Its name will represent our pass. It inherits from `PassInfoMixin`, which is a class provided by LLVM to simplify the creation of passes.
        ```cpp
        struct MultiplicationShifts : public PassInfoMixin<MultiplicationShifts> {
        ```
        - Define the `run` method for your pass. 
            ```cpp
            PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
            ```
            - The logic of the pass should be inside the `run` function. **Note**: You can encapsulate this logic in other function(s) and call it here. The logic of this pass is:

                1. Iterate over basic blocks in a function
                2. Iterate over instructions in the basic block
                3. Check if the current instruction is a binary operator (`mul` is a binary operator)
                4. Check if the binary operator is a multiplication
                5. Check if the second operand is a constant power of 2
                6. If true, replace multiplication with left shift by the corresponding value. Replacing the instruction is done by creating a new one (or using an existing one), replace the uses of the previous instruction with the new one and erasing the obsolete one.

                ```cpp
                Modified = false;
                for (auto &BB : F) {
                    for (auto I = BB.begin(), E = BB.end(); I != E; ) {
                        Instruction *Inst = &(*I++);
                        if (auto *Mul = dyn_cast<BinaryOperator>(Inst)) {
                            if (Mul->getOpcode() == Instruction::Mul) {
                                if (auto *C = dyn_cast<ConstantInt>(Mul->getOperand(1))) {
                                    if (C->getValue().isPowerOf2()) {
                                        IRBuilder<> Builder(Mul);
                                        Value *ShiftAmount = Builder.getInt32(C->getValue().exactLogBase2());
                                        Value *NewMul = Builder.CreateShl(Mul->getOperand(0), ShiftAmount);
                                        Mul->replaceAllUsesWith(NewMul);
                                        Mul->eraseFromParent(); 
                                        Modified = true;
                                    }
                                }
                            }
                        }
                    }
                }
                ```

            - You can use `errs()` function to print useful informations for the user or to debug your pass while writing. Here, we'll use a printer pass that will get the result of `Modified` variable from the transformation pass and it will print something based on the value. 

    - Define the printer pass:
        ```cpp
        // New Printer pass
        struct MultiplicationShiftsPrinter : public PassInfoMixin<MultiplicationShiftsPrinter> {
            PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
                errs() << "*** MULTIPLICATION SHIFTS PASS EXECUTING ***\n";
                if (Modified) {
                    errs() << "Some instruction was replaced.\n";
                } else {
                    errs() << "Nothing changed.\n";
                }
                return PreservedAnalyses::all();
            }
        };
        }
        ```

    - Now, we need to register both passes.
        ```cpp
        // Register the passes as plugins
        PassPluginLibraryInfo getMultiplicationShiftsPluginInfo() {
            return {LLVM_PLUGIN_API_VERSION, "MultiplicationShifts", LLVM_VERSION_STRING,
                    [](PassBuilder &PB) {
                        PB.registerPipelineParsingCallback(
                            [](StringRef Name, FunctionPassManager &FPM,
                            ArrayRef<PassBuilder::PipelineElement>) {
                            if (Name == "multiplication-shifts") {
                                FPM.addPass(MultiplicationShifts()); // Run the transformation pass
                                FPM.addPass(MultiplicationShiftsPrinter()); // Run the printer pass with the modified status
                                return true;
                            }
                            return false;
                            });
                        PB.registerPipelineStartEPCallback([](ModulePassManager &MPM,
                                                            OptimizationLevel Level) {
                            FunctionPassManager FPM;
                            FPM.addPass(MultiplicationShifts()); // Run the transformation pass
                            FPM.addPass(MultiplicationShiftsPrinter()); // Run the printer pass with the modified status

                            MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                        });
                    }};
        }
        ```

    - Lastly, we write the entry point for the pass plugin. It provides LLVM with information about the plugin, returned by the `getMultiplicationShiftsPluginInfo` function we just discussed.
        
        ```cpp
        extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
        llvmGetPassPluginInfo() {
            return getMultiplicationShiftsPluginInfo();
        }
        ```

2. **Write CMakeLists.txt**: You can use the `CMakeLists.txt` from HelloWorld as a template for this pass as it does not need anything unique. 

## Write a test file

The [test](test.c) file should have at least one multiplication by a power of two. For example:

```c
#include <stdio.h>

int main()
{
    int i = 3;
    int z = i * 4; // int z = i << 2
    printf("%d\n", z);
    return 0;
}
```

### Build the pass

To build the pass, we run the following commands. Optionally, you can define the environment variable `LLVM_PATH` with the path to the toolchain: `LLVM_PATH=<path/to/toolchain>`. If you do not do this, alter the commands with the appropriate path. **Note**: if you are using system LLVM, there is no need for this.

```bash
# Create and go to build folder
$ mkdir build && cd build
# Configure CMake
$ cmake -DLT_LLVM_INSTALL_DIR=$LLVM_PATH ../MultiplicationShifts/
# Build the project (this line could be just 'make')
$ cmake --build . 
```

The output should be something similar to:

```
...
-- Configuring done (0.6s)
-- Generating done (0.0s)
-- Build files have been written to: <path/to>/build
[ 50%] Building CXX object CMakeFiles/MS.dir/MultiplicationShifts.cpp.o
[100%] Linking CXX shared module libMS.so
[100%] Built target MS
```

### Generate LLVM IR file

After generating the LLVM Intermediate Representation (IR) files using clang, it's important to note that while optimization levels such as `-O1`, `-O2`, `-O3`, etc., typically work for other passes, they may not be suitable for this particular pass due to the simplicity of the test file. In fact, any level of optimization, including `-O1`, can cause the `mul` instructions to vanish. Therefore, to prevent this issue, it's necessary to include `-Xclang -disable-O0-optnone` flags with clang, ensuring that optimizations are enabled even at the default optimization level.

```bash
$ $LLVM_PATH/bin/clang -S -emit-llvm -Xclang -disable-O0-optnone test.c -o test.ll
```

### Run pass with opt

Since we used `MS` as library name for our pass, a file called `libMS.so` was created in `build` folder. That is our plugin.

```bash
# Load the pass to opt and pass the correct name
$ $LLVM_PATH/bin/opt -load-pass-plugin build/libMS.so -passes=multiplication-shifts test.ll -o mod.bc
# Use llvm-dis to convert .bc -> .ll
$ $LLVM_PATH/bin/llvm-dis mod.bc -o mod.ll
```

Note that we used `llvm-dis`. It is a tool to transform bitcode LLVM to a human readable LLVM file. But we could have also used the `-S` flag with `opt`, which would generate an `.ll` file.

If everything went fine, you should see:

```
*** MULTIPLICATION SHIFTS PASS EXECUTING ***
Some instruction was replaced.
```

Let's use `llvm-diff` tool to check what changed in the file.

```bash
$ $LLVM_PATH/bin/llvm-diff mod.ll test.ll
in function main:
  in block %0 / %0:
    >   %5 = mul nsw i32 %4, 4
    >   store i32 %5, ptr %3, align 4
    <   %5 = shl i32 %4, 2
    <   store i32 %5, ptr %3, align 4
```

We can see the pass worked correctly: the line that contained the multiplication of a variable by 4 was converted into a left shift by 2. 

You can also use the `lli` tool, which is the LLVM IR interpreter. This tool allows us to directly execute programs from LLVM bitcode. It is wise to test the modified file because, even if the file is correctly transformed, something might have been broken by the pass.

### Run pass with clang

To execute the pass with `clang`, use the following command:

```bash
$ $LLVM_PATH/bin/clang -fpass-plugin=<absolute-path-to>/build/libMS.so -Xclang -disable-O0-optnone test.c -S -emit-llvm -o mod_clang.ll
```

Note that using `clang` to run the pass has an advantage: you only need to provide the C source file as input. However, you can also run the pass with `$LLVM_PATH/bin/clang -fpass-plugin=<absolute-path-to>/build/libMS.so -Xclang -disable-O0-optnone test.ll -o mod_clang.ll`, and it should work as well.

Similarly, you should see the message below and the output of `llvm-diff` should be the same as the run with `opt`.

```
*** MULTIPLICATION SHIFTS PASS EXECUTING ***
Some instruction was replaced.
```

## CMake

CMake can be very handy, especially with LLVM passes, as it abstracts many complexities. However, it's often useful to understand what's happening behind the scenes. 

Since we set `CMAKE_EXPORT_COMPILE_COMMANDS` in `CMakeLists.txt`, you can check the commands used to compile and link the plugin. This can be very helpful for debugging when things go wrong.

Check the `build/compile_commands.json` and `build/CMakeFiles/MS.dir/link.txt` files. A good exercise would be to build the plugin using standalone commands, without CMake.

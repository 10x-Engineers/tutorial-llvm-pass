## Write LLVM Plugin

To create an LLVM Plugin, follow these steps:

1. **Write the Pass Code**: Below is the breakdown of an LLVM Example Pass written in C++ (check source [here](HelloWorld/HelloWorld.cpp)). This pass will print "Hello from {name-of-function}" and the number of arguments in each function.

    - Include necessary header files. For this pass, we'll need only these:
        ```cpp
        #include "llvm/Passes/PassBuilder.h"
        #include "llvm/Passes/PassPlugin.h"
        #include "llvm/Support/raw_ostream.h"
        ```
    - Create a struct. Its name will represent our pass. It inherits from `PassInfoMixin`, which is a class provided by LLVM to simplify the creation of passes.
        ```cpp
        struct HelloWorld : public PassInfoMixin<HelloWorld> {
        ```
        - Define the `run` method for your pass. Since this is a pass that'll run over functions, its parameters are: a reference to `Function` class and a reference to `FunctionAnalysisManager` class.
            ```cpp
            PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
            ```
            - All passes need a `run` function. It is its entry point. A good practice is to encapsulate the steps in smaller functions. Here, we'll create a `visitor` function.
            ```cpp
            // This method implements what the pass does
            void visitor(Function &F) {
                errs() << "Hello from: "<< F.getName() << "\n";
                errs() << "  number of arguments: " << F.arg_size() << "\n";
            }

            struct HelloWorld : PassInfoMixin<HelloWorld> {
            // Main entry point, takes IR unit to run the pass on (&F) and the
            // corresponding pass manager (to be queried if need be)
            PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
                visitor(F);
                return PreservedAnalyses::all();
            }

            };
            } // namespace
            ```

            - `errs()` returns a reference to a `raw_fd_ostream` for standard error and `outs()` returns a reference to a `raw_fd_ostream` for standard output. Hence, these are generally used for printing in LLVM passes.  
            - When we write transformation passes, they might change, exclude or add instructions during the process. Because of this, the pass communicates which analyses are still valid after its operation. Since our pass does not change anything, we return `PreservedAnalyses::all()` meaning everything before the pass is still valid.


    - Register the pass as a plugin by creating a `get<MY_PASS_NAME>PluginInfo`. In this case, `PassPluginLibraryInfo getHelloWorldPluginInfo() {`. This will return a struct of type [`PassPluginLibraryInfo`](https://llvm.org/doxygen/structllvm_1_1PassPluginLibraryInfo.html) which contains:
        - `uint32_t APIVersion` - usually `LLVM_PLUGIN_API_VERSION`
        - `const char *PluginName` - a meaninful name of the plugin
        - `const char *PluginVersion` - the version of the plugin
        - `void (*RegisterPassBuilderCallbacks)(PassBuilder &)` - callback for registering plugin passes with LLVM's pass manager

        ```cpp
        PassPluginLibraryInfo getHelloWorldPluginInfo() {
            return {LLVM_PLUGIN_API_VERSION, "HelloWorld", LLVM_VERSION_STRING,
                    [](PassBuilder &PB) {
                        PB.registerPipelineParsingCallback(
                            [](StringRef Name, FunctionPassManager &FPM,
                            ArrayRef<PassBuilder::PipelineElement>) {
                            if (Name == "hello-world") {

                                FPM.addPass(HelloWorld()); // Add the Hello World pass
                                return true;
                            }
                            return false;
                            });
                        PB.registerPipelineStartEPCallback([](ModulePassManager &MPM,
                                                        OptimizationLevel Level) {

                            FunctionPassManager FPM;
                            FPM.addPass(HelloWorld());

                            MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                        });
                    }};
        }
        ```

        - Our pass is integrated into LLVM's pass manager using a lambda function.
            - In the lambda function, we register two callbacks:
            1. `registerPipelineParsingCallback`: this callback is essential for executing the pass with `opt`. It is invoked during pipeline parsing and verifies if the pipeline name is "hello-world". If it matches, the hello world pass is added to the function pass manager (`FPM`). Essentialy, it adds the pass to the pass manager if the name is passed to the pipeline with the `-passes=OUR_PASS_NAME` flag. This mechanism is crucial for out-of-tree passes like ours.
            2. `registerPipelineStartEPCallback`: this is needed for running the pass with `clang`. It is invoked at the start of pass pipeline execution. It creates a new function pass manager (`FPM`), incorporates the hello world pass, and then includes this function pass manager to the module pass manager (`MPM`) using an adaptor. In other words, this ensures our pass is introduced near the start of the pipeline.

    - Lastly, we write the entry point for the pass plugin. It provides LLVM with information about the plugin, returned by the `getHelloWorldPluginInfo` function we just discussed.
        
        ```cpp
        extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
        llvmGetPassPluginInfo() {
            return getHelloWorldPluginInfo();
        }
        ```

2. **Write CMakeLists.txt**: This [file](HelloWorld/CMakeLists.txt) is strongly based on [LLVM Tutor](https://github.com/banach-space/llvm-tutor) and [LLVM Pass Skeleton](https://github.com/sampsyo/llvm-pass-skeleton).

    - Set minimum required CMake Version and project name - this is needed in every CMake project
        
        ```cmake
        cmake_minimum_required(VERSION 3.13.4)
        project(HelloWorld)
        ```

    - (Optional) Export compile commands - this is quite useful for debugging. It will generate a file `compile_commands.json` inside the build folder.

        ```cmake
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
        ```
    
    - Set compiler paths.

        ```cmake
        set(CMAKE_C_COMPILER <path/to>/bin/clang)
        set(CMAKE_CXX_COMPILER <path/to>/bin/clang++)
        ```

    - Load LLVM Configuration

        (Optional) If you want to use LLVM built from source, you can use a variable with its path:
        ```cmake
        # Define a CMake variable LT_LLVM_INSTALL_DIR with an empty string as initial value. This is used to specify the directory where the LLVM toolchain is installed.
        # CACHE is used to make the variable persisent across runs
        set(LT_LLVM_INSTALL_DIR "" CACHE PATH "LLVM installation directory")
        # Append the LLVM installation directory to the CMake search paths. This allows searching for the LLVMConfig.cmake file, necessary for finding and configuring LLVM.
        list(APPEND CMAKE_PREFIX_PATH "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm/")
        ```
        If you are using LLVM built from source, the following statements are the same, but if you installed LLVM from your package manager, you might need to change the following first line to `find_package(LLVM <VERSION> REQUIRED CONFIG)`. For example, `find_package(LLVM 17 REQUIRED CONFIG)`.
        ```cmake
        # Locate and try to load LLVM package. REQUIRED keyword means that if Cmake does not find LLVM, it stops
        find_package(LLVM REQUIRED CONFIG)
        # Include directories specified by LLVM in the project's include path
        include_directories(${LLVM_INCLUDE_DIRS})
        # Include definitions specified by LLVM in the project's options
        add_definitions(${LLVM_DEFINITIONS})
        link_directories(${LLVM_LIBRARY_DIR})
        ```
    
    - Configure build

        ```cmake
        # Sets C++ standard to C++17 for the project.
        # CACHE STRING "" ensures the user can modify it
        set(CMAKE_CXX_STANDARD 17 CACHE STRING "")
        # LLVM is normally built without RTTI. Be consistent with that.
        if(NOT LLVM_ENABLE_RTTI)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
        endif()
        ```

    - Add the target

        ```cmake
        # Adds a target named Hello to the project. It is created as a shared library from the source file HelloWorld.cpp
        add_library(Hello SHARED HelloWorld.cpp)

        # Link against LLVM libraries
        target_link_libraries(Hello ${llvm_libs})
        ```

## Write a test file

The [test](test_hello.c) file contains a lot of functions. No need to understand it, we just needed a good number of functions with arguments to see the pass working.

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function declarations
void printMessage(const char *message);
int add(int a, int b);
double multiply(double a, double b);
char* concatenateStrings(const char *str1, const char *str2);
void printArray(int arr[], int size);
int findMax(int arr[], int size);

int main() {
    // Demonstrating printMessage function
    printMessage("Hello, world!");

    // Demonstrating add function
    int sum = add(5, 7);
    printf("Sum: %d\n", sum);

    // Demonstrating multiply function
    double product = multiply(3.5, 2.0);
    printf("Product: %.2f\n", product);

    // Demonstrating concatenateStrings function
    char *combined = concatenateStrings("Hello, ", "world!");
    printf("Concatenated String: %s\n", combined);
    free(combined);

    // Demonstrating printArray and findMax functions
    int numbers[] = {4, 7, 1, 8, 5, 9};
    int size = sizeof(numbers) / sizeof(numbers[0]);

    printArray(numbers, size);
    int max = findMax(numbers, size);
    printf("Maximum Value: %d\n", max);

    return 0;
}

// Function definitions

void printMessage(const char *message) {
    printf("%s\n", message);
}

int add(int a, int b) {
    return a + b;
}

double multiply(double a, double b) {
    return a * b;
}

char* concatenateStrings(const char *str1, const char *str2) {
    // Allocate memory for the concatenated string
    char *result = (char*)malloc(strlen(str1) + strlen(str2) + 1);
    if (result == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    // Concatenate the strings
    strcpy(result, str1);
    strcat(result, str2);

    return result;
}

void printArray(int arr[], int size) {
    printf("Array: ");
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int findMax(int arr[], int size) {
    int max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

```

### Build the pass

To build the pass, we run the following commands. Optionally, you can define the environment variable `LLVM_PATH` with the path to the toolchain: `LLVM_PATH=<path/to/toolchain>`. If you do not do this, alter the commands with the appropriate path. **Note**: if you are using system LLVM, there is no need for this.

```bash
# Create and go to build folder
$ mkdir build && cd build
# Configure CMake
$ cmake -DLT_LLVM_INSTALL_DIR=$LLVM_PATH ../HelloWorld/
# Build the project (this line could be just 'make')
$ cmake --build . 
```

The output should be something similar to:

```
...
-- Configuring done (0.6s)
-- Generating done (0.0s)
-- Build files have been written to: <path/to>/build
[ 50%] Building CXX object CMakeFiles/Hello.dir/HelloWorld.cpp.o
[100%] Linking CXX shared library libHello.so
[100%] Built target Hello
```

### Generate LLVM IR file

LLVM transformation are typically applied on LLVM Intermediate Representation (IR) files, which can be `.ll` for human-readable format and `.bc` for bitcode format. To generate the IR file from a C source file, run `clang` with the `-emit-llvm` flag. If you prefer the human-readable `.ll` format, you can include the `-S` option. 

So, we could run `$LLVM_PATH/bin/clang -S -emit-llvm test_hello.c -o test_hello.ll`. If you check `test_hello.ll` after this command, you'll see some lines describing function attributes like this: `; Function Attrs: noinline nounwind optnone uwtable`. The attribute `optnone` disables optimizations. Hence, running the pass with `opt` will not work.

Therefore, to prevent this issue, it's necessary to include `-Xclang -disable-O0-optnone` flags with clang, ensuring that optimizations are enabled even at the default optimization level.

```bash
$ $LLVM_PATH/bin/clang -S -emit-llvm -Xclang -disable-O0-optnone test_hello.c -o test_hello.ll
```

### Run pass with opt

Since we used `Hello` as library name for our pass, a file called `libHello.so` was created in `build` folder. That is our plugin.

```bash
# Load the pass to opt and pass the correct name
$ $LLVM_PATH/bin/opt -load-pass-plugin build/libHello.so -passes=hello-world test_hello.ll
```

If everything went fine, you should see:

```
WARNING: You're attempting to print out a bitcode file.
This is inadvisable as it may cause display problems. If
you REALLY want to taste LLVM bitcode first-hand, you
can force output with the `-f' option.

Hello from: main
  number of arguments: 0
Hello from: printMessage
  number of arguments: 1
Hello from: add
  number of arguments: 2
Hello from: multiply
  number of arguments: 2
Hello from: concatenateStrings
  number of arguments: 2
Hello from: printArray
  number of arguments: 2
Hello from: findMax
  number of arguments: 2
```

You can pass `-disable-output` and the WARNING will not appear anymore.

### Run pass with clang

To execute the pass with `clang`, use the following command:

```bash
$ $LLVM_PATH/bin/clang -fpass-plugin=<absolute-path-to>/build/libHello.so -Xclang -disable-O0-optnone test_hello.c
```

Note that using `clang` to run the pass has an advantage: you only need to provide the C source file as input. However, you can also run the pass with `$LLVM_PATH/bin/clang -fpass-plugin=<absolute-path-to>/build/libMS.so -Xclang -disable-O0-optnone test_hello.ll`, and it should work as well.

Similarly, you should see the same output as seen with `opt`.


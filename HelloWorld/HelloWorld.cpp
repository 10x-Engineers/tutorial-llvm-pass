#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

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


// Register the pass as a plugin
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

// Entry point for the pass plugin
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getHelloWorldPluginInfo();
}
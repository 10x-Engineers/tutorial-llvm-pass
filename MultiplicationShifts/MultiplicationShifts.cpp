#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

// Static variable to track modifications
static bool Modified = false;

// MultiplicationShifts pass without printing
struct MultiplicationShifts : public PassInfoMixin<MultiplicationShifts> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
        Modified = false;
        // Iterate over basic blocks in the function
        for (auto &BB : F) {
            // Iterate over instructions in the basic block
            for (auto I = BB.begin(), E = BB.end(); I != E; ) {
                Instruction *Inst = &(*I++);
                // Check if the instruction is a binary operator
                if (auto *Mul = dyn_cast<BinaryOperator>(Inst)) {
                    // Check if the binary operator is multiplication
                    if (Mul->getOpcode() == Instruction::Mul) {
                        // Check if the second operand is a constant power of two
                        if (auto *C = dyn_cast<ConstantInt>(Mul->getOperand(1))) {
                            if (C->getValue().isPowerOf2()) {
                                // If true, replace multiplication with left shift
                                IRBuilder<> Builder(Mul);
                                Value *ShiftAmount = Builder.getInt32(C->getValue().exactLogBase2());
                                Value *NewMul = Builder.CreateShl(Mul->getOperand(0), ShiftAmount);
                                Mul->replaceAllUsesWith(NewMul);
                                Mul->eraseFromParent(); // Remove the multiplication instruction
                                Modified = true;
                            }
                        }
                    }
                }
            }
        }
        // Indicate whether the function was modified or not
        return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};

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

// Entry point for the pass plugin
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getMultiplicationShiftsPluginInfo();
}


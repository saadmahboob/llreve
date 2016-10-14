/*
 * This file is part of
 *    llreve - Automatic regression verification for LLVM programs
 *
 * Copyright (C) 2016 Karlsruhe Institute of Technology
 *
 * The system is published under a BSD license.
 * See LICENSE (distributed with this file) for details.
 */

#include "Preprocess.h"

#include "CFGPrinter.h"
#include "Helper.h"
#include "InferMarks.h"
#include "InlinePass.h"
#include "InlinePass.h"
#include "InstCombine.h"
#include "MonoPair.h"
#include "PathAnalysis.h"
#include "RemoveMarkPass.h"
#include "RemoveMarkRefsPass.h"
#include "SplitEntryBlockPass.h"
#include "UniqueNamePass.h"
#include "Unroll.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/ADCE.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"

using std::map;
using std::vector;
using std::shared_ptr;
using std::make_shared;
using std::string;
using llvm::ErrorOr;

static void nameFunctionArguments(llvm::Function &fun, Program prog) {
    std::map<string, int> instructionNames;
    for (auto &arg : fun.args()) {
        makePrefixed(arg, std::to_string(programIndex(prog)), instructionNames);
    }
}

AnalysisResultsMap
preprocessFunctions(MonoPair<shared_ptr<llvm::Module>> modules,
                    PreprocessOpts opts) {
    AnalysisResultsMap preprocessingResults;
    preprocessFunctions(*modules.first, opts, preprocessingResults,
                        Program::First);
    preprocessFunctions(*modules.second, opts, preprocessingResults,
                        Program::Second);
    return preprocessingResults;
}

void preprocessFunctions(llvm::Module &module, PreprocessOpts opts,
                         AnalysisResultsMap &preprocessingResults,
                         Program prog) {
    for (auto &f : module) {
        if (!f.isIntrinsic() && !isLlreveIntrinsic(f)) {
            if (hasFixedAbstraction(f)) {
                nameFunctionArguments(f, prog);
            } else {
                preprocessingResults.insert(
                    {&f, preprocessFunction(
                             f, std::to_string(programIndex(prog)), opts)});
            }
        }
    }
}

AnalysisResults preprocessFunction(llvm::Function &fun, string prefix,
                                   PreprocessOpts opts) {
    auto fpm =
        std::make_unique<llvm::legacy::FunctionPassManager>(fun.getParent());
    fpm->add(llvm::createUnifyFunctionExitNodesPass());

    fpm->add(new InlinePass());
    fpm->add(llvm::createPromoteMemoryToRegisterPass()); // mem2reg
    fpm->add(llvm::createLoopSimplifyPass());
    fpm->add(llvm::createCFGSimplificationPass());
    fpm->add(new SplitBlockPass());

    MarkAnalysis *markAnalysis = new MarkAnalysis();
    InferMarksAnalysis *inferMarkAnalysis = new InferMarksAnalysis();
    fpm->add(inferMarkAnalysis);
    fpm->add(markAnalysis);
    if (!opts.InferMarks) {
        fpm->add(new RemoveMarkRefsPass());
    }
    fpm->add(new InstCombinePass());
    fpm->add(llvm::createAggressiveDCEPass());
    fpm->add(llvm::createConstantPropagationPass());
    // Passes need to have a default ctor
    UniqueNamePass::Prefix = prefix;
    fpm->add(new UniqueNamePass()); // prefix register names
    if (opts.ShowMarkedCFG) {
        fpm->add(new CFGViewerPass()); // show marked cfg
    }
    if (!opts.InferMarks) {
        fpm->add(new RemoveMarkPass());
    }
    PathAnalysis *pathAnalysis = new PathAnalysis();
    pathAnalysis->InferMarks = opts.InferMarks;
    fpm->add(pathAnalysis);
    if (opts.ShowCFG) {
        fpm->add(new CFGViewerPass()); // show cfg
    }
    fpm->add(llvm::createVerifierPass());
    // FPM.addPass(llvm::PrintFunctionPass(errs())); // dump function
    fpm->doInitialization();
    fpm->run(fun);
    if (opts.InferMarks) {
        return AnalysisResults(inferMarkAnalysis->BlockMarkMap,
                               pathAnalysis->PathsMap);
    } else {
        return AnalysisResults(markAnalysis->BlockMarkMap,
                               pathAnalysis->PathsMap);
    }
}

#pragma once

#include "Memory.h"
#include "MonoPair.h"
#include "PathAnalysis.h"
#include "Program.h"
#include "SMT.h"

auto invariant(int StartIndex, int EndIndex, std::vector<std::string> InputArgs,
               std::vector<std::string> EndArgs, ProgramSelection SMTFor,
               std::string FunName, Memory Heap) -> smt::SMTRef;
auto mainInvariant(int EndIndex, std::vector<std::string> FreeVars,
                   std::string FunName, Memory Heap) -> smt::SMTRef;
auto invariantDeclaration(int BlockIndex, std::vector<std::string> FreeVars,
                          ProgramSelection For, std::string FunName,
                          Memory Heap) -> MonoPair<smt::SMTRef>;
auto mainInvariantDeclaration(int BlockIndex, std::vector<std::string> FreeVars,
                              ProgramSelection For, std::string FunName)
    -> smt::SMTRef;
auto invariantName(int Index, ProgramSelection For, std::string FunName,
                   uint32_t VarArgs = 0) -> std::string;
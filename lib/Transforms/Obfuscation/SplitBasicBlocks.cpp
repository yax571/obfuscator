//===- SplitBasicBlock.cpp - SplitBasicBlokc Obfuscation pass--------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the split basic block pass
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Obfuscation/Split.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/CryptoUtils.h"

#define DEBUG_TYPE "split"

using namespace llvm;
using namespace std;

// Stats
STATISTIC(Split, "Basicblock splitted");

static cl::opt<int> SplitNum("splitNum", cl::init(2),
                             cl::desc("Split perBB time each BB"));

/*
static cl::opt<string> FunctionName(
    "funcSPLIT", cl::init(""),
    cl::desc(
        "Flatten only certain functions: -mllvm -funcFLA=\"func1,func2\""));
*/

/*
static cl::opt<int>
Percentage("perSPLIT", cl::init(100),
           cl::desc("Flatten only a certain percentage of functions"));
*/

namespace {
struct SplitBasicBlock : public FunctionPass {
  static char ID; // Pass identification, replacement for typeid
  bool flag;

  SplitBasicBlock() : FunctionPass(ID) {}
  SplitBasicBlock(bool flag) : FunctionPass(ID) {
    // Check if the number of applications is correct
    if (!((SplitNum > 1) && (SplitNum <= 10))) {
      LLVMContext &ctx = llvm::getGlobalContext();
      ctx.emitError(Twine("Split application basic block percentage\
              -perBB=x must be 1 < x <= 10"));
    }

    this->flag = flag;
  }

  bool runOnFunction(Function &F);
  void split(Function *f);

  bool containsPHI(BasicBlock *b);
  void shuffle(std::vector<int> &vec);
};
}

char SplitBasicBlock::ID = 0;
static RegisterPass<SplitBasicBlock> X("split", "BasicBlock splitting");

Pass *llvm::createSplitBasicBlock(bool flag) {
  return new SplitBasicBlock(flag);
}

bool SplitBasicBlock::runOnFunction(Function &F) {
  Function *tmp = &F;

  // Do we obfuscate
  if (toObfuscate(flag, tmp, "split")) {
    split(tmp);
    ++Split;
  }

  return false;
}

void SplitBasicBlock::split(Function *f) {
  std::vector<BasicBlock *> origBB;
  int splitN = SplitNum;

  // Save all basic blocks
  for (Function::iterator I = f->begin(), IE = f->end(); I != IE; ++I) {
    origBB.push_back(I);
  }

  for (std::vector<BasicBlock *>::iterator I = origBB.begin(),
                                           IE = origBB.end();
       I != IE; ++I) {
    BasicBlock *curr = *I;

    // No need to split a 1 inst bb
    // Or ones containing a PHI node
    if (curr->size() < 2 || containsPHI(curr)) {
      continue;
    }

    // Check splitN and current BB size
    if ((size_t)splitN > curr->size()) {
      splitN = curr->size() - 1;
    }

    // Generate splits point
    std::vector<int> test;
    for (unsigned i = 1; i < curr->size(); ++i) {
      test.push_back(i);
    }

    // Shuffle
    if (test.size() != 1) {
      shuffle(test);
      std::sort(test.begin(), test.begin() + splitN);
    }

    // Split
    BasicBlock::iterator it = curr->begin();
    BasicBlock *toSplit = curr;
    int last = 0;
    for (int i = 0; i < splitN; ++i) {
      for (int j = 0; j < test[i] - last; ++j) {
        ++it;
      }
      last = test[i];
      toSplit = toSplit->splitBasicBlock(it, toSplit->getName() + ".split");
    }

    ++Split;
  }
}

bool SplitBasicBlock::containsPHI(BasicBlock *b) {
  for (BasicBlock::iterator I = b->begin(), IE = b->end(); I != IE; ++I) {
    if (isa<PHINode>(I)) {
      return true;
    }
  }
  return false;
}

void SplitBasicBlock::shuffle(std::vector<int> &vec) {
  int n = vec.size();
  for (int i = n - 1; i > 0; --i) {
    std::swap(vec[i], vec[cryptoutils->get_uint32_t() % (i + 1)]);
  }
}


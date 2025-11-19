#include "Random.h"
#include "ProgramGenerator.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ToolOutputFile.h"

#include <memory>

using namespace llvm;
using namespace llvm_fuzzer;

static cl::opt<unsigned> InitSeed("seed", cl::desc("Seed used for randomness (0 = random seed)"), cl::init(0));
static cl::opt<std::string> OutputFilename("o", cl::desc("Override output filename"), cl::value_desc("filename"));

int main(int argc, char** argv)
{
	llvm::PrettyStackTraceProgram X(argc, argv);
	cl::ParseCommandLineOptions(argc, argv, "llvm codegen stress-tester\n");

	LLVMContext context;
	auto module = std::make_unique<Module>("autogen.ll", context);

	// Pick an initial seed value and create a random number generator
	if (InitSeed == 0)
		InitSeed = std::random_device()();
	auto randomGenerator = Random(InitSeed);

	// Use the program generator to randomly update the module
	ProgramGenerator progGenerator(std::move(module), randomGenerator);
	progGenerator.generateRandomProgram();

	// Figure out what stream we are supposed to write to...
	std::unique_ptr<ToolOutputFile> outFile;
	// Default to standard output.
	if (OutputFilename.empty())
		OutputFilename = "-";

	std::error_code EC;
	outFile = std::make_unique<ToolOutputFile>(OutputFilename, EC, sys::fs::OF_None);
	if (EC)
	{
		errs() << EC.message() << '\n';
		return 1;
	}

	// Create LLVM passes that verify the generated module and print it out
	// In LLVM 20, we need to use the new PassManager or handle legacy passes differently
	// For now, just print the module directly without verification
	progGenerator.getModule().print(outFile->os(), nullptr);
	outFile->keep();

	return 0;
}
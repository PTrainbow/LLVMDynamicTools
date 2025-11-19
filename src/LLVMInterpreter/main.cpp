#include "Interpreter.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
// Removed TargetSelect.h - not needed for pure interpreter

using namespace llvm;
using namespace llvm_interpreter;

cl::opt<std::string> InputFile(cl::desc("<input bitcode>"), cl::Positional, cl::init("-"));

cl::opt<std::string> FunctionName("function", cl::desc("Function to execute (default: main)"), cl::init("main"));

cl::list<std::string> InputArgv(cl::ConsumeAfter, cl::desc("<program arguments>..."));

// Main driver of the interpreter
int main(int argc, char** argv, char* const *envp)
{
	sys::PrintStackTraceOnErrorSignal(argv[0]);
	PrettyStackTraceProgram X(argc, argv);

	LLVMContext context;

	// Removed InitializeNativeTarget calls - not needed for pure interpreter
	// These functions are only needed for code generation, which we don't do

	cl::ParseCommandLineOptions(argc, argv, "llvm interpreter & dynamic compiler\n");

	// Disable core file
	sys::Process::PreventCoreFiles();

	// Read and parse the IR file
	SMDiagnostic err;
	auto module = parseIRFile(InputFile, err, context);
	if (!module)
	{
		err.print(argv[0], errs());
		std::exit(1);
	}

	// Load the whole bitcode file eagerly
	// In LLVM 20, materializeAllPermanently() was removed, functions are materialized on demand
	// No need to explicitly materialize

	Interpreter interpreter(module.get());
	interpreter.evaluateGlobals();

	auto entryFn = module->getFunction(FunctionName);
	if (entryFn == nullptr)
	{
		errs() << "Function \'" << FunctionName << "\' not found in module.\n";
		return -1;
	}
	
	if (FunctionName == "main")
	{
		// For main function, use the special runMain that handles argv
		std::vector<std::string> mainArgs = InputArgv;
		mainArgs.insert(mainArgs.begin(), InputFile);
		auto retInt = interpreter.runMain(entryFn, mainArgs);
		errs() << "Interpreter returns value " << retInt << "\n";
	}
	else
	{
		// For other functions, convert string arguments to DynamicValue
		std::vector<DynamicValue> args;
		
		// Get function signature
		auto funcType = entryFn->getFunctionType();
		unsigned numParams = funcType->getNumParams();
		
		// Convert string arguments to appropriate types
		// Skip sret parameter if present
		unsigned paramStart = 0;
		if (numParams > 0 && funcType->getParamType(0)->isPointerTy())
		{
			// Check if first parameter has sret attribute (struct return)
			auto firstParam = entryFn->arg_begin();
			if (firstParam->hasStructRetAttr())
			{
				// Skip sret parameter - it's an output parameter
				paramStart = 1;
				errs() << "Note: Function uses struct return (sret), first parameter is output\n";
			}
		}
		
		unsigned argIdx = 0;
		for (unsigned i = paramStart; i < numParams && argIdx < InputArgv.size(); ++i, ++argIdx)
		{
			auto paramType = funcType->getParamType(i);
			if (paramType->isIntegerTy())
			{
				int intVal = std::stoi(InputArgv[argIdx]);
				auto bitWidth = cast<IntegerType>(paramType)->getBitWidth();
				args.push_back(DynamicValue::getIntValue(APInt(bitWidth, intVal, true)));
			}
			else if (paramType->isFloatTy() || paramType->isDoubleTy())
			{
				double floatVal = std::stod(InputArgv[argIdx]);
				args.push_back(DynamicValue::getFloatValue(floatVal, paramType->isDoubleTy()));
			}
			else
			{
				errs() << "Unsupported parameter type for function " << FunctionName << "\n";
				return -1;
			}
		}
		
		if (argIdx < InputArgv.size())
		{
			errs() << "Warning: " << (InputArgv.size() - argIdx) << " extra arguments ignored\n";
		}
		
		auto retVal = interpreter.runFunction(entryFn, args);
		
		// Print return value
		if (retVal.isUndefValue())
		{
			errs() << "Interpreter returns void\n";
		}
		else if (retVal.isStructValue())
		{
			errs() << "Interpreter returns struct: " << retVal.toString() << "\n";
		}
		else if (retVal.isArrayValue())
		{
			errs() << "Interpreter returns array: " << retVal.toString() << "\n";
		}
		else if (retVal.isIntValue())
		{
			auto intVal = retVal.getAsIntValue().getInt();
			auto retType = entryFn->getReturnType();
			
			// Check if return type is a struct (packed as i64/i32)
			// In LLVM, small structs are often returned as integers
			if (retType->isStructTy())
			{
				llvm::SmallString<32> hexStr;
				intVal.toString(hexStr, 16, false);
				errs() << "Interpreter returns struct (packed as integer): 0x" << hexStr.str() << "\n";
				errs() << "  Raw value: " << intVal.getSExtValue() << "\n";
				
				// Try to extract fields if it's a 64-bit value (likely 2x i32)
				if (intVal.getBitWidth() == 64)
				{
					uint64_t val = intVal.getZExtValue();
					uint32_t low = val & 0xFFFFFFFF;
					uint32_t high = (val >> 32) & 0xFFFFFFFF;
					errs() << "  Extracted fields (assuming 2x i32): [" << (int32_t)low << ", " << (int32_t)high << "]\n";
				}
			}
			else
			{
				errs() << "Interpreter returns value " << intVal.getSExtValue() << "\n";
			}
		}
		else if (retVal.isFloatValue())
		{
			errs() << "Interpreter returns value " << retVal.getAsFloatValue().getFloat() << "\n";
		}
		else if (retVal.isPointerValue())
		{
			errs() << "Interpreter returns pointer: " << retVal.toString() << "\n";
		}
		else
		{
			errs() << "Interpreter returns: " << retVal.toString() << "\n";
		}
	}

	return 0;
}
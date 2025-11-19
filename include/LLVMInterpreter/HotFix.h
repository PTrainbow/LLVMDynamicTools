#ifndef DYNPTS_HOTFIX_H
#define DYNPTS_HOTFIX_H

#include "Interpreter.h"
#include "DynamicValue.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace llvm_interpreter
{

// Type descriptor for runtime type information
enum class TypeKind {
    INT8, INT16, INT32, INT64,
    UINT8, UINT16, UINT32, UINT64,
    FLOAT, DOUBLE,
    POINTER,  // void* or any pointer type
    STRUCT    // Structure type (size must be provided)
};

struct TypeInfo {
    TypeKind kind;
    size_t size;  // For STRUCT type, this is the struct size
    const char* structName;  // Optional struct name for debugging
};

// HotFix - Runtime code replacement using LLVM interpreter
class HotFix {
private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<Interpreter> interpreter;
    bool initialized;

    // Convert C++ value to DynamicValue based on type info
    DynamicValue convertToDynamicValue(const void* value, const TypeInfo& typeInfo);
    
    // Convert DynamicValue back to C++ value
    void convertFromDynamicValue(const DynamicValue& dv, void* output, const TypeInfo& typeInfo);

public:
    HotFix();
    ~HotFix();

    // Load bitcode from memory buffer
    bool loadBitcode(const char* bitcodeData, size_t bitcodeSize);
    
    // Load bitcode from file
    bool loadBitcodeFromFile(const std::string& filename);
    
    // Load bitcode from string (LLVM IR text format)
    bool loadBitcodeFromString(const std::string& irString);

    // Execute a function with arbitrary arguments
    // Args: array of void* pointers to argument values
    // ArgTypes: array of TypeInfo describing each argument
    // NumArgs: number of arguments
    // ReturnType: type info for return value
    // ReturnValue: output buffer for return value (can be nullptr for void)
    // Returns: true on success, false on error
    bool executeFunction(
        const char* functionName,
        const void* const* args,
        const TypeInfo* argTypes,
        size_t numArgs,
        const TypeInfo* returnType,
        void* returnValue
    );

    // Check if a function exists in the loaded module
    bool hasFunction(const char* functionName) const;

    // Get list of all function names in the module
    std::vector<std::string> getFunctionNames() const;
    
    // Register an external function callback
    // This allows bitcode to call functions from the host program
    // Callback signature: DynamicValue(const llvm::Function* func, const std::vector<DynamicValue>& args)
    void registerExternalFunction(const std::string& name, 
                                  std::function<DynamicValue(const llvm::Function*, const std::vector<DynamicValue>&)> callback);
    
    // Unregister an external function
    void unregisterExternalFunction(const std::string& name);
    
    // Get the underlying interpreter (for advanced usage)
    Interpreter* getInterpreter() { return interpreter.get(); }
};

} // namespace llvm_interpreter

#endif // DYNPTS_HOTFIX_H


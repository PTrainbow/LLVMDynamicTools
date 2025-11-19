#include "HotFix.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DataLayout.h"

#include <cstring>
#include <cassert>

using namespace llvm;
using namespace llvm_interpreter;

HotFix::HotFix() : context(std::make_unique<LLVMContext>()), initialized(false) {
}

HotFix::~HotFix() {
    interpreter.reset();
    module.reset();
    context.reset();
}

bool HotFix::loadBitcode(const char* bitcodeData, size_t bitcodeSize) {
    if (!bitcodeData || bitcodeSize == 0) {
        errs() << "HotFix: Invalid bitcode data\n";
        return false;
    }

    SMDiagnostic err;
    // For binary bitcode, we'd need to use parseBitcodeFile, but for now
    // we'll assume it's IR text format
    auto memBuffer = MemoryBuffer::getMemBufferCopy(
        StringRef(bitcodeData, bitcodeSize),
        "hotfix_bitcode"
    );
    
    module = parseIR(memBuffer->getMemBufferRef(), err, *context);
    if (!module) {
        err.print("HotFix", errs());
        return false;
    }

    interpreter = std::make_unique<Interpreter>(module.get());
    interpreter->evaluateGlobals();
    initialized = true;
    
    return true;
}

bool HotFix::loadBitcodeFromFile(const std::string& filename) {
    SMDiagnostic err;
    module = parseIRFile(filename, err, *context);
    if (!module) {
        err.print("HotFix", errs());
        return false;
    }

    interpreter = std::make_unique<Interpreter>(module.get());
    interpreter->evaluateGlobals();
    initialized = true;
    
    return true;
}

bool HotFix::loadBitcodeFromString(const std::string& irString) {
    SMDiagnostic err;
    auto memBuffer = MemoryBuffer::getMemBufferCopy(irString, "hotfix_ir");
    module = parseIR(memBuffer->getMemBufferRef(), err, *context);
    if (!module) {
        err.print("HotFix", errs());
        return false;
    }

    interpreter = std::make_unique<Interpreter>(module.get());
    interpreter->evaluateGlobals();
    initialized = true;
    
    return true;
}

DynamicValue HotFix::convertToDynamicValue(const void* value, const TypeInfo& typeInfo) {
    if (!value) {
        return DynamicValue::getUndefValue();
    }

    switch (typeInfo.kind) {
        case TypeKind::INT8:
            return DynamicValue::getIntValue(APInt(8, *(int8_t*)value, true));
        case TypeKind::INT16:
            return DynamicValue::getIntValue(APInt(16, *(int16_t*)value, true));
        case TypeKind::INT32:
            return DynamicValue::getIntValue(APInt(32, *(int32_t*)value, true));
        case TypeKind::INT64:
            return DynamicValue::getIntValue(APInt(64, *(int64_t*)value, true));
        case TypeKind::UINT8:
            return DynamicValue::getIntValue(APInt(8, *(uint8_t*)value, false));
        case TypeKind::UINT16:
            return DynamicValue::getIntValue(APInt(16, *(uint16_t*)value, false));
        case TypeKind::UINT32:
            return DynamicValue::getIntValue(APInt(32, *(uint32_t*)value, false));
        case TypeKind::UINT64:
            return DynamicValue::getIntValue(APInt(64, *(uint64_t*)value, false));
        case TypeKind::FLOAT:
            return DynamicValue::getFloatValue(*(float*)value, false);
        case TypeKind::DOUBLE:
            return DynamicValue::getFloatValue(*(double*)value, true);
        case TypeKind::POINTER: {
            // Convert void* to pointer value
            // We use HEAP_SPACE and store the address as-is
            // Note: This assumes the pointer is valid in the host process
            uintptr_t addr = reinterpret_cast<uintptr_t>(*(void**)value);
            return DynamicValue::getPointerValue(
                PointerAddressSpace::HEAP_SPACE,
                static_cast<Address>(addr)
            );
        }
        case TypeKind::STRUCT: {
            // For structs, we need to copy the data into interpreter's memory
            // This is a simplified version - in practice, you might want to
            // map struct fields more carefully
            if (typeInfo.size == 0) {
                return DynamicValue::getUndefValue();
            }
            // Create a struct value and copy data
            // Note: This is a workaround - proper struct handling would require
            // knowing the struct layout
            auto structVal = DynamicValue::getStructValue(typeInfo.size);
            // For now, we'll store the raw bytes in the first field
            // In a real implementation, you'd parse the struct layout
            return structVal;
        }
        default:
            return DynamicValue::getUndefValue();
    }
}

void HotFix::convertFromDynamicValue(const DynamicValue& dv, void* output, const TypeInfo& typeInfo) {
    if (!output || dv.isUndefValue()) {
        return;
    }

    switch (typeInfo.kind) {
        case TypeKind::INT8:
            *(int8_t*)output = (int8_t)dv.getAsIntValue().getInt().getSExtValue();
            break;
        case TypeKind::INT16:
            *(int16_t*)output = (int16_t)dv.getAsIntValue().getInt().getSExtValue();
            break;
        case TypeKind::INT32:
            *(int32_t*)output = (int32_t)dv.getAsIntValue().getInt().getSExtValue();
            break;
        case TypeKind::INT64:
            *(int64_t*)output = (int64_t)dv.getAsIntValue().getInt().getSExtValue();
            break;
        case TypeKind::UINT8:
            *(uint8_t*)output = (uint8_t)dv.getAsIntValue().getInt().getZExtValue();
            break;
        case TypeKind::UINT16:
            *(uint16_t*)output = (uint16_t)dv.getAsIntValue().getInt().getZExtValue();
            break;
        case TypeKind::UINT32:
            *(uint32_t*)output = (uint32_t)dv.getAsIntValue().getInt().getZExtValue();
            break;
        case TypeKind::UINT64:
            *(uint64_t*)output = (uint64_t)dv.getAsIntValue().getInt().getZExtValue();
            break;
        case TypeKind::FLOAT:
            *(float*)output = (float)dv.getAsFloatValue().getFloat();
            break;
        case TypeKind::DOUBLE:
            *(double*)output = dv.getAsFloatValue().getFloat();
            break;
        case TypeKind::POINTER: {
            // Convert pointer value back to void*
            auto ptrVal = dv.getAsPointerValue();
            uintptr_t addr = static_cast<uintptr_t>(ptrVal.getAddress());
            *(void**)output = reinterpret_cast<void*>(addr);
            break;
        }
        case TypeKind::STRUCT:
            // Struct conversion is complex and would require layout information
            // For now, we'll leave it as a placeholder
            break;
    }
}

bool HotFix::executeFunction(
    const char* functionName,
    const void* const* args,
    const TypeInfo* argTypes,
    size_t numArgs,
    const TypeInfo* returnType,
    void* returnValue
) {
    if (!initialized || !interpreter || !module) {
        errs() << "HotFix: Not initialized. Call loadBitcode first.\n";
        return false;
    }

    auto func = module->getFunction(functionName);
    if (!func) {
        errs() << "HotFix: Function '" << functionName << "' not found\n";
        return false;
    }

    if (func->isDeclaration()) {
        errs() << "HotFix: Function '" << functionName << "' is external (not implemented)\n";
        return false;
    }

    // Convert arguments
    std::vector<DynamicValue> dynamicArgs;
    for (size_t i = 0; i < numArgs; ++i) {
        dynamicArgs.push_back(convertToDynamicValue(args[i], argTypes[i]));
    }

    // Execute function
    auto retVal = interpreter->runFunction(func, dynamicArgs);

    // Convert return value
    if (returnType && returnValue) {
        convertFromDynamicValue(retVal, returnValue, *returnType);
    }

    return true;
}

bool HotFix::hasFunction(const char* functionName) const {
    if (!module) return false;
    return module->getFunction(functionName) != nullptr;
}

std::vector<std::string> HotFix::getFunctionNames() const {
    std::vector<std::string> names;
    if (!module) return names;
    
    for (const auto& func : *module) {
        if (!func.isDeclaration()) {
            names.push_back(func.getName().str());
        }
    }
    return names;
}

void HotFix::registerExternalFunction(const std::string& name, 
                                      std::function<DynamicValue(const llvm::Function*, const std::vector<DynamicValue>&)> callback) {
    if (!interpreter) {
        errs() << "HotFix: Not initialized. Call loadBitcode first.\n";
        return;
    }
    interpreter->registerExternalFunction(name, callback);
}

void HotFix::unregisterExternalFunction(const std::string& name) {
    if (!interpreter) return;
    interpreter->unregisterExternalFunction(name);
}


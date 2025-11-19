// HotFix 使用示例
#include "LLVMInterpreter/HotFix.h"
#include <iostream>
#include <cstring>

using namespace llvm_interpreter;

int main() {
    // 创建 HotFix 实例
    HotFix hotfix;
    
    // 示例1: 从字符串加载 LLVM IR
    const char* irCode = R"(
define i32 @add(i32 %a, i32 %b) {
  %sum = add i32 %a, %b
  ret i32 %sum
}

define i32 @multiply(i32 %x, i32 %y) {
  %prod = mul i32 %x, %y
  ret i32 %prod
}
)";
    
    if (!hotfix.loadBitcodeFromString(irCode)) {
        std::cerr << "Failed to load bitcode\n";
        return 1;
    }
    
    // 执行 add 函数
    int32_t a = 10, b = 20;
    const void* args[] = { &a, &b };
    TypeInfo argTypes[] = { 
        { TypeKind::INT32, sizeof(int32_t), nullptr },
        { TypeKind::INT32, sizeof(int32_t), nullptr }
    };
    TypeInfo returnType = { TypeKind::INT32, sizeof(int32_t), nullptr };
    int32_t result = 0;
    
    if (hotfix.executeFunction("add", args, argTypes, 2, &returnType, &result)) {
        std::cout << "add(10, 20) = " << result << std::endl;
    } else {
        std::cerr << "Failed to execute add function\n";
    }
    
    // 执行 multiply 函数
    int32_t x = 7, y = 8;
    const void* args2[] = { &x, &y };
    int32_t result2 = 0;
    
    if (hotfix.executeFunction("multiply", args2, argTypes, 2, &returnType, &result2)) {
        std::cout << "multiply(7, 8) = " << result2 << std::endl;
    } else {
        std::cerr << "Failed to execute multiply function\n";
    }
    
    // 示例2: 测试更复杂的计算
    const char* irCode2 = R"(
define i32 @square(i32 %x) {
  %result = mul i32 %x, %x
  ret i32 %result
}
)";
    
    HotFix hotfix2;
    if (hotfix2.loadBitcodeFromString(irCode2)) {
        int32_t x = 9;
        const void* args3[] = { &x };
        int32_t result3 = 0;
        
        if (hotfix2.executeFunction("square", args3, argTypes, 1, &returnType, &result3)) {
            std::cout << "square(9) = " << result3 << std::endl;
        }
    }
    
    return 0;
}


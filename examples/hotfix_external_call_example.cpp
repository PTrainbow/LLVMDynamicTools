// 演示如何在 bitcode 中调用外部函数（原程序中的函数）
#include "LLVMInterpreter/HotFix.h"
#include <iostream>
#include <cassert>

using namespace llvm_interpreter;

// 原程序中的函数（模拟）
int add(int a, int b) {
    return a + b;
}

int multiply(int x, int y) {
    return x * y;
}

int main() {
    HotFix hotfix;
    
    // 修复后的 bitcode，调用外部函数 add
    // 注意：在 C 代码中只需要声明 extern int add(int, int);
    // 编译后会生成 declare i32 @add(i32, i32) 的 LLVM IR
    const char* bugfixCode = R"(
; 声明外部函数（原程序中的函数）
declare i32 @add(i32, i32)

; 修复后的函数
define i32 @bugfix(i32 %x) {
  %result = call i32 @add(i32 %x, i32 1)
  ret i32 %result
}
)";
    
    if (!hotfix.loadBitcodeFromString(bugfixCode)) {
        std::cerr << "Failed to load bitcode\n";
        return 1;
    }
    
    // 注册外部函数 add，让它调用原程序中的 add 函数
    hotfix.registerExternalFunction("add", 
        [](const llvm::Function* func, const std::vector<DynamicValue>& args) -> DynamicValue {
            // 提取参数
            assert(args.size() == 2);
            int32_t a = args[0].getAsIntValue().getInt().getSExtValue();
            int32_t b = args[1].getAsIntValue().getInt().getSExtValue();
            
            // 调用原程序中的函数
            int32_t result = add(a, b);
            
            // 返回结果
            return DynamicValue::getIntValue(llvm::APInt(32, result, true));
        });
    
    // 执行修复后的函数
    int32_t x = 10;
    const void* args[] = { &x };
    TypeInfo argTypes[] = { 
        { TypeKind::INT32, sizeof(int32_t), nullptr }
    };
    TypeInfo returnType = { TypeKind::INT32, sizeof(int32_t), nullptr };
    int32_t result = 0;
    
    if (hotfix.executeFunction("bugfix", args, argTypes, 1, &returnType, &result)) {
        std::cout << "bugfix(10) = " << result << std::endl;
        std::cout << "Expected: 11 (10 + 1)" << std::endl;
    } else {
        std::cerr << "Failed to execute bugfix function\n";
        return 1;
    }
    
    // 示例2: 调用多个外部函数
    const char* complexFixCode = R"(
declare i32 @add(i32, i32)
declare i32 @multiply(i32, i32)

define i32 @complexFix(i32 %x, i32 %y) {
  %sum = call i32 @add(i32 %x, i32 %y)
  %prod = call i32 @multiply(i32 %sum, i32 2)
  ret i32 %prod
}
)";
    
    HotFix hotfix2;
    if (hotfix2.loadBitcodeFromString(complexFixCode)) {
        // 注册 add 函数
        hotfix2.registerExternalFunction("add",
            [](const llvm::Function* func, const std::vector<DynamicValue>& args) -> DynamicValue {
                int32_t a = args[0].getAsIntValue().getInt().getSExtValue();
                int32_t b = args[1].getAsIntValue().getInt().getSExtValue();
                return DynamicValue::getIntValue(llvm::APInt(32, add(a, b), true));
            });
        
        // 注册 multiply 函数
        hotfix2.registerExternalFunction("multiply",
            [](const llvm::Function* func, const std::vector<DynamicValue>& args) -> DynamicValue {
                int32_t x = args[0].getAsIntValue().getInt().getSExtValue();
                int32_t y = args[1].getAsIntValue().getInt().getSExtValue();
                return DynamicValue::getIntValue(llvm::APInt(32, multiply(x, y), true));
            });
        
        // 执行
        int32_t x2 = 5, y2 = 3;
        const void* args2[] = { &x2, &y2 };
        TypeInfo argTypes2[] = { 
            { TypeKind::INT32, sizeof(int32_t), nullptr },
            { TypeKind::INT32, sizeof(int32_t), nullptr }
        };
        int32_t result2 = 0;
        
        if (hotfix2.executeFunction("complexFix", args2, argTypes2, 2, &returnType, &result2)) {
            std::cout << "complexFix(5, 3) = " << result2 << std::endl;
            std::cout << "Expected: 16 ((5+3)*2)" << std::endl;
        }
    }
    
    return 0;
}


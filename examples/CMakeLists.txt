cmake_minimum_required(VERSION 2.8.8)

# 获取 LLVM 配置
find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Boost
find_package(Boost COMPONENTS exception REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

# 包含头文件路径
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/LLVMInterpreter)

# LLVM 库
llvm_map_components_to_libnames(LLVM_LIBS core irreader object support)

# 编译示例1: hotfix_example
add_executable(hotfix_example hotfix_example.cpp
    ${CMAKE_SOURCE_DIR}/src/LLVMInterpreter/DynamicValue.cpp
    ${CMAKE_SOURCE_DIR}/src/LLVMInterpreter/Evaluation.cpp
    ${CMAKE_SOURCE_DIR}/src/LLVMInterpreter/External.cpp
    ${CMAKE_SOURCE_DIR}/src/LLVMInterpreter/Interpreter.cpp
    ${CMAKE_SOURCE_DIR}/src/LLVMInterpreter/InfoDump.cpp
    ${CMAKE_SOURCE_DIR}/src/LLVMInterpreter/HotFix.cpp
)
target_link_libraries(hotfix_example ${LLVM_LIBS} ${Boost_LIBRARIES})

# 编译示例2: hotfix_external_call_example
add_executable(hotfix_external_call_example hotfix_external_call_example.cpp
    ${CMAKE_SOURCE_DIR}/src/LLVMInterpreter/DynamicValue.cpp
    ${CMAKE_SOURCE_DIR}/src/LLVMInterpreter/Evaluation.cpp
    ${CMAKE_SOURCE_DIR}/src/LLVMInterpreter/External.cpp
    ${CMAKE_SOURCE_DIR}/src/LLVMInterpreter/Interpreter.cpp
    ${CMAKE_SOURCE_DIR}/src/LLVMInterpreter/InfoDump.cpp
    ${CMAKE_SOURCE_DIR}/src/LLVMInterpreter/HotFix.cpp
)
target_link_libraries(hotfix_external_call_example ${LLVM_LIBS} ${Boost_LIBRARIES})

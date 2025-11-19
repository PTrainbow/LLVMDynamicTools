# The external function handling requires boost
find_package(Boost COMPONENTS exception REQUIRED)
message(status ": found Boost Include: ${Boost_INCLUDE_DIR}")
message(status ": found Boost Libraries: ${Boost_LIBRARY_DIRS}")
message(status ": found Boost Libraries: ${Boost_LIBRARIES}")

# Find the FFI library
#find_library(LibFFI NAMES ffi)
#message(status ": found libffi: ${LibFFI}")

# Make sure the compiler can find include files from our library. 
include_directories(${Boost_INCLUDE_DIR})
include_directories(${dynamic_pts_SOURCE_DIR}/include/LLVMInterpreter)
link_directories(${Boost_LIBRARY_DIRS})

set(SourceFiles DynamicValue.cpp Evaluation.cpp External.cpp Interpreter.cpp InfoDump.cpp main.cpp)

add_executable(llvm-interpreter ${SourceFiles}) 

# Find the libraries that correspond to the LLVM components that we wish to use
# Minimal components for pure interpreter: only core IR functionality
# Removed: executionengine, instrumentation, interpreter, native (not needed)
llvm_map_components_to_libnames(ReferencedLLVMLibs core irreader object support)

# Use static linking with aggressive size optimization
target_link_libraries(llvm-interpreter ${ReferencedLLVMLibs} ${Boost_LIBRARIES})

# Aggressive size optimization for static linking
set_target_properties(llvm-interpreter PROPERTIES
    LINK_FLAGS "-Wl,-dead_strip -flto"
    COMPILE_FLAGS "-Os -DNDEBUG -ffunction-sections -fdata-sections -flto"
)
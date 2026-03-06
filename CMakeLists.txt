cmake_minimum_required(VERSION 3.16)
project(llm_log_examples CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

foreach(target basic_log scoped_call query_log)
    add_executable(${target} examples/${target}.cpp)
    target_include_directories(${target} PRIVATE include)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall>
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wextra>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
    )
endforeach()

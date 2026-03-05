cmake_minimum_required(VERSION 3.15)
project(CppAdvancedTopics VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# ==================== 专题 1: 模板元编程 ====================
add_executable(topic_01_template
    01_template_metaprogramming.cpp
)

# ==================== 专题 2: 移动语义 ====================
add_executable(topic_02_move
    02_move_semantics.cpp
)

# ==================== 专题 3: 智能指针 ====================
add_executable(topic_03_smart_ptr
    03_smart_pointers.cpp
)

# ==================== 专题 4: 虚函数 ====================
add_executable(topic_04_virtual
    04_virtual_functions.cpp
)

# ==================== 专题 5: 内存管理 ====================
add_executable(topic_05_memory
    05_memory_management.cpp
)

# ==================== 综合项目 ====================
add_executable(cpp_final_project
    final_project.cpp
)

message(STATUS "C++ 高级特性专题构建完成！")
message(STATUS "可执行文件:")
message(STATUS "  - topic_01_template: 模板元编程")
message(STATUS "  - topic_02_move: 移动语义")
message(STATUS "  - topic_03_smart_ptr: 智能指针")
message(STATUS "  - topic_04_virtual: 虚函数与多态")
message(STATUS "  - topic_05_memory: 内存管理")
message(STATUS "  - cpp_final_project: 综合项目")

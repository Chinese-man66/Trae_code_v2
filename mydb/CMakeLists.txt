cmake_minimum_required(VERSION 3.15)
project(MyDB VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# ==================== Stage 1: 基础阶段 ====================

add_executable(mydb_stage1
    stage1_basics/main.cpp
)

target_include_directories(mydb_stage1 PRIVATE
    ${PROJECT_SOURCE_DIR}/mydb/stage1_basics
)

# ==================== Stage 2: 进阶阶段 ====================

add_executable(mydb_stage2
    stage2_intermediate/main.cpp
)

target_include_directories(mydb_stage2 PRIVATE
    ${PROJECT_SOURCE_DIR}/mydb/stage1_basics
    ${PROJECT_SOURCE_DIR}/mydb/stage2_intermediate
)

# ==================== Stage 3: 高级阶段 ====================

add_executable(mydb_stage3
    stage3_advanced/main.cpp
)

target_include_directories(mydb_stage3 PRIVATE
    ${PROJECT_SOURCE_DIR}/mydb/stage1_basics
    ${PROJECT_SOURCE_DIR}/mydb/stage2_intermediate
    ${PROJECT_SOURCE_DIR}/mydb/stage3_advanced
)

# ==================== 可执行文件说明 ====================

message(STATUS "MyDB 构建完成！")
message(STATUS "可执行文件:")
message(STATUS "  - mydb_stage1: 基础阶段 - 基本存储与 CRUD")
message(STATUS "  - mydb_stage2: 进阶阶段 - 索引与查询优化")
message(STATUS "  - mydb_stage3: 高级阶段 - 事务与并发")

cmake_minimum_required(VERSION 3.16)
project(AlgorithmSwitching VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(BUILD_LEGACY_APP "Build incomplete Qt/CUDA legacy application" OFF)
if(BUILD_LEGACY_APP)


# 包含模块文件
include(cmake/modules.cmake)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 查找所有依赖包
find_all_packages()

# 源文件
file(GLOB_RECURSE SOURCES
    "main.cpp"
    "Interface/*.cpp"
    "Controller/*/*.cpp"
    "Data/*.cpp"
)

# 头文件
file(GLOB_RECURSE HEADERS
    "Interface/*.hpp"
    "Controller/*/*.hpp"
    "Data/*.hpp"
)

# Qt UI文件
file(GLOB UI_FILES "Interface/*.ui")

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI_FILES})

# 设置包含目录
set_all_includes(${PROJECT_NAME})

# Qt自动生成
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# 链接所有依赖库
link_all_libraries(${PROJECT_NAME})

# 复制配置文件到构建目录
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY config/
    DESTINATION etc/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.yaml"
)

# 添加测试
option(BUILD_TESTS "Build the tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 版本信息
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

# 调试信息
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "Qt version: ${Qt5_VERSION}")
message(STATUS "Boost version: ${Boost_VERSION}")
message(STATUS "CUDA version: ${CUDA_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
return()
endif()

find_package(Threads REQUIRED)
add_library(plugin_runtime runtime/runtime.cpp)
target_include_directories(plugin_runtime PUBLIC runtime plugin_sdk/include)
target_link_libraries(plugin_runtime PUBLIC ${CMAKE_DL_LIBS} Threads::Threads)
function(example_plugin target factor)
  add_library(${target} SHARED plugins/reference.cpp)
  target_include_directories(${target} PRIVATE plugin_sdk/include)
  target_compile_definitions(${target} PRIVATE ALGO_BUILD_PLUGIN PLUGIN_FACTOR=${factor} ${ARGN})
endfunction()
example_plugin(cpu_reference_a 1)
example_plugin(cpu_reference_b 2)
add_executable(algorithm_demo apps/cli/main.cpp)
target_link_libraries(algorithm_demo PRIVATE plugin_runtime)
include(CTest)
if(BUILD_TESTING)
  example_plugin(bad_version 1 BAD_VERSION)
  example_plugin(create_failure 1 CREATE_FAILURE)
  example_plugin(infer_failure 1 INFER_FAILURE)
  example_plugin(short_table 1 SHORT_TABLE)
  example_plugin(null_api 1 NULL_API)
  example_plugin(missing_function 1 MISSING_FUNCTION)
  add_library(missing_symbol SHARED tests/missing_symbol.cpp)
  add_executable(runtime_tests tests/runtime_tests.cpp)
  target_link_libraries(runtime_tests PRIVATE plugin_runtime)
  add_test(NAME runtime_contract COMMAND runtime_tests
    $<TARGET_FILE:cpu_reference_a> $<TARGET_FILE:cpu_reference_b>
    $<TARGET_FILE:bad_version> $<TARGET_FILE:create_failure>
    $<TARGET_FILE:infer_failure> $<TARGET_FILE:missing_symbol>
    $<TARGET_FILE:short_table> $<TARGET_FILE:null_api> $<TARGET_FILE:missing_function>)
  add_test(NAME cli_switch COMMAND algorithm_demo
    $<TARGET_FILE:cpu_reference_a> $<TARGET_FILE:cpu_reference_b>)
  set_tests_properties(cli_switch PROPERTIES PASS_REGULAR_EXPRESSION "B: 2 4 6")
endif()

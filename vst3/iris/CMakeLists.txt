cmake_minimum_required(VERSION 3.20)
project(Iris VERSION 1.8.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_PLUGIN_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../vst3sdk ${CMAKE_BINARY_DIR}/vst3sdk)

smtg_add_vst3plugin(Iris
    ${CMAKE_CURRENT_SOURCE_DIR}/source/iris_plugin.cpp
    # SingleComponentEffect 不在 sdk 的默认源列表里，必须显式加入
    ${CMAKE_CURRENT_SOURCE_DIR}/../vst3sdk/public.sdk/source/vst/vstsinglecomponenteffect.cpp
)

if(MSVC)
  target_compile_options(Iris PRIVATE /utf-8 /W3)
endif()

target_include_directories(Iris PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_SOURCE_DIR}/../vst3sdk)
if(TARGET sdk)
  target_link_libraries(Iris PRIVATE sdk)
endif()
target_compile_definitions(Iris PRIVATE IRIS_VERSION_STR="${PROJECT_VERSION}")
target_link_libraries(Iris PRIVATE gdiplus)






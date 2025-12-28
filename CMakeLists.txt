cmake_minimum_required(VERSION 3.11)

project(DPIbypass CXX)
set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    add_compile_options(/W4 /WX)
endif()

find_library(WINDIVERT_LIB 
    NAMES WinDivert.lib
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/bin ${CMAKE_CURRENT_SOURCE_DIR}/include
    NO_DEFAULT_PATH
    REQUIRED
)

add_executable(DPIbypass src/main.cpp
	src/dpi_bypasser.h
	src/dpi_bypasser.cpp
)

target_include_directories(DPIbypass PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(DPIbypass 
    ${WINDIVERT_LIB}
    ws2_32  
)
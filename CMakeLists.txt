cmake_minimum_required(VERSION 2.8)

# MIPS plugin build
set(ISA 			"x86")
set(NAMESPACE		"otawa")
set(ELF_NUM			"3")

project(${ISA})

# look for OTAWA
if(NOT OTAWA_CONFIG)
    find_program(OTAWA_CONFIG otawa-config DOC "path to otawa-config")
    if(NOT OTAWA_CONFIG)
        message(FATAL_ERROR "ERROR: otawa-config is required !")
    endif()
endif()
message(STATUS "otawa-config found at ${OTAWA_CONFIG}")

# get OTAWA configuration
execute_process(COMMAND "${OTAWA_CONFIG}" --cflags
	OUTPUT_VARIABLE OTAWA_CFLAGS  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${OTAWA_CONFIG}" --libs -r -p "${ISA}.eld"
	OUTPUT_VARIABLE OTAWA_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${OTAWA_CONFIG}" --prefix --rpath
	OUTPUT_VARIABLE OTAWA_PREFIX  OUTPUT_STRIP_TRAILING_WHITESPACE)
include("${OTAWA_PREFIX}/share/Otawa/cmake/Otawa.cmake")
set(CMAKE_CXX_STANDARD 11)

add_compile_options(-Wall)
if(CMAKE_VERSION LESS "3.1")
	add_compile_options("--std=c++11")
	message(STATUS "C++11 set using cflags")
else()
	set(CMAKE_CXX_STANDARD 11)
	message(STATUS "C++ set using CMAKE_CXX_STANDARD")
endif()


set(SOURCES
	"prog_Decoder.cpp"
	"prog_DefaultLoader.cpp"
	"${ISA}.cpp"
)


# build the library
set(CMAKE_CXX_FLAGS "-Wall")
include_directories("include")
add_library("${ISA}" SHARED ${SOURCES})
set_property(TARGET "${ISA}" PROPERTY PREFIX "")
set_property(TARGET "${ISA}" PROPERTY COMPILE_FLAGS "${OTAWA_CFLAGS}")
message(STATUS "OTAWA_LDFLAGS=${OTAWA_LDFLAGS}")
target_link_libraries("${ISA}" "gel++")
set(CMAKE_SHARED_LINKER_FLAGS "${OTAWA_LDFLAGS}")

# installation
set(PLUGIN_PATH "${OTAWA_PREFIX}/lib/otawa/${NAMESPACE}")
install(TARGETS "${ISA}" LIBRARY		DESTINATION "${PLUGIN_PATH}")
install(FILES	"${ISA}.eld"			DESTINATION "${PLUGIN_PATH}")
install(FILES	"elf_${ELF_NUM}.eld"	DESTINATION "${OTAWA_PREFIX}/lib/otawa/loader")
install(FILES	"elf_${ELF_NUM}.eld"	DESTINATION "${OTAWA_PREFIX}/lib/otawa/decode")

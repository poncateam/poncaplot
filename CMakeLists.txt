cmake_minimum_required(VERSION 3.0.0)

project("PoncaPlot")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Maybe stop from CMAKEing in the wrong place
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Source and build directories cannot be the same. Go use the /build directory.")
endif()

### Configure output locations
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

### Compiler options
set( CMAKE_EXPORT_COMPILE_COMMANDS 1 ) # Emit a compile flags file to support completion engines

# Add ponca
message("\n\n == CMAKE add Ponca\n")
set( PONCA_CONFIGURE_EXAMPLES CACHE BOOL OFF)
set( PONCA_CONFIGURE_TESTS    CACHE BOOL OFF)
set( PONCA_CONFIGURE_DOC      CACHE BOOL OFF)
add_subdirectory("external/ponca")

# Check if Eigen is used from a package or from Ponca submodules
find_package(Eigen3 QUIET)
set(Eigen_Deps "")
if( NOT Eigen3_FOUND ) # Should use Ponca submodule
	message( "Polyscope should use Eigen from ${EIGEN3_INCLUDE_DIRS}" ) 
	include_directories(${EIGEN3_INCLUDE_DIRS})
else()
	set(Eigen_Deps Eigen3::Eigen)
endif()

# Add nanogui
set( NANOGUI_BUILD_EXAMPLES CACHE BOOL OFF)
set( NANOGUI_BUILD_PYTHON CACHE BOOL OFF)
message("\n\n == CMAKE recursively building NanoGUI\n")
add_subdirectory("external/nanogui")

# Add argparse
set(ARGPARSE_INSTALL OFF)
set(ARGPARSE_BUILD_TESTS OFF)
set(ARGPARSE_BUILD_SAMPLES OFF)
message("\n\n == CMAKE recursively building argparse\n")
add_subdirectory("external/argparse")

# Find OpenMP
find_package(OpenMP)
set(OpenMP_link_libraries )
if(OpenMP_CXX_FOUND)
    set(OpenMP_link_libraries OpenMP::OpenMP_CXX)
    message("OpenMP found")
endif()

# Create an executable
add_executable( poncaplot
        src/dataManager.h
        src/dataManager.cpp
        src/myview.h
        src/myview.cpp
        src/application.h
        src/application.cpp
        src/cli.h
        src/cli.cpp
        src/poncaTypes.h
        src/drawingPass.h
        src/drawingPasses/distanceField.h
        src/drawingPasses/poncaFitField.h
                src/main.cpp
)

# Include settings
target_include_directories(poncaplot PUBLIC
                            "${CMAKE_CURRENT_SOURCE_DIR}/external/nanogui/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/external/ponca/"
                            "${CMAKE_CURRENT_SOURCE_DIR}/external/argparse/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/external/stb/"
                            "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# Link settings
target_link_libraries(poncaplot nanogui ${Eigen_Deps} ${OpenMP_link_libraries})


# Fix potential bug on windows (appears with VSCode, but not with VS)
#   Moves bin to project/bin instead of project/bin/BuidType/
set_target_properties(poncaplot PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}>)

# Fix compilation error with MSVC
if (MSVC)
  target_compile_options(poncaplot PRIVATE /bigobj -openmp:llvm)
endif ()


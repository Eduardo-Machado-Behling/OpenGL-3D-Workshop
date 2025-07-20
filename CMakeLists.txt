# Minimum required version of CMake
cmake_minimum_required(VERSION 3.16)

# Project name and language
project(OpenGLBenchmark LANGUAGES CXX C)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies ---
include(FetchContent)

# Optional: set a common prefix for all downloaded sources
set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/_deps)

# Ensure we build these in Release mode, regardless of main project config
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

### GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

### GLM (header-only, no build)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

### ASSIMP
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG        v6.0.0
)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)

add_subdirectory(vendor/glad)
file(GLOB IMGUI_SOURCE_ALL
    vendor/imgui/*.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
)

# Create a static library named "glad" from the glad.c source file.
# A static library is linked into the final executable at compile time.
add_library(imguiOpenGL STATIC ${IMGUI_SOURCE_ALL})

# Specify the public include directories for the 'glad' library.
# The "PUBLIC" keyword means that any other target that links against 'glad'
# will automatically get this include path added.
target_include_directories(imguiOpenGL PUBLIC
        "${CMAKE_SOURCE_DIR}/vendor/imgui"          # For imgui.h, imgui_internal.h
        "${CMAKE_SOURCE_DIR}/vendor/imgui/backends" # For imgui_impl_opengl3.h, imgui_impl_glfw.h
	)

target_link_libraries(imguiOpenGL PRIVATE glfw)


# --- Executable ---

# Set the output directory for the executable to './bin/'
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)


# CMake function to build an executable with common settings.
#
# Arguments:
#   name   - The name of the executable target (e.g., "indirect").
#   folder - The folder containing the primary source files for the executable.
#            These will be found recursively using GLOB_RECURSE.
#
# Usage Example:
#   build(MyApplication src)
#
function(build name folder)
    # Find all .cpp source files recursively within the specified 'folder'.
    # This will include files like 'main.cpp' if 'src' is passed as 'folder'.
    file(GLOB_RECURSE EXECUTABLE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/${folder}/*.cpp"
    )

    # Explicitly add ImGui's core and backend source files.
    # We assume 'vendor/imgui' is located directly under CMAKE_SOURCE_DIR.
    list(APPEND EXECUTABLE_SOURCES
		${IMGUI_SOURCE}
    )

    # Define the executable target using the collected source files.
    # Mathematical Analogy: This step is like defining the core computational unit
    # (the executable) and specifying all its direct input components (source files)
    # that need to be compiled to produce the final output.
    add_executable(${name} ${EXECUTABLE_SOURCES})

    # Set optimization level to O3 for release builds.
    # This is a property specific to the executable target.
    set_target_properties(${name} PROPERTIES
        COMPILE_FLAGS_RELEASE "-O3"
    )

    # --- Linking Libraries ---
    # Link the executable against required libraries: GLFW, GLM, and GLAD.
    # These packages should be found using 'find_package' in your main CMakeLists.txt
    # before this function is called.
	target_link_libraries(${name} PRIVATE glfw glm::glm glad assimp imguiOpenGL)

    # Add necessary include directories for the target.
    # This tells the compiler where to find header files (e.g., for ImGui, GLAD).
    # Mathematical Analogy: This is like providing the compiler with a "lookup table"
    # or "reference manual" of all the definitions (headers) it might encounter
    # during the compilation of the source files.
    target_include_directories(${name} PRIVATE
        "${CMAKE_SOURCE_DIR}/include"               # General include directory (e.g., for GLAD headers)
    )

    # On Unix-like systems (excluding macOS), link against specific OpenGL and threading libraries.
    if(UNIX AND NOT APPLE)
        target_link_libraries(${name} PRIVATE
            -lGL      # Standard OpenGL library
            -ldl      # Dynamic linking library
            -lpthread # POSIX threads library
        )
    endif()
endfunction()

build(main src)

set(TOOLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tools")

# Use file(GLOB) to find all subdirectories in TOOLS_DIR
# The 'LIST_DIRECTORIES true' argument is crucial to only get directories.
file(GLOB TOOL_FOLDERS LIST_DIRECTORIES true "${TOOLS_DIR}/*")

# Loop through each found folder path
foreach(folder_path IN LISTS TOOL_FOLDERS)
    # Extract just the name of the folder from its full path
    get_filename_component(folder_name "${folder_path}" NAME)

	build(${folder_name} "tools/${folder_name}")
    message(STATUS "Found tool folder: ${folder_name}")

endforeach()

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
    COMMENT "Copying assets to output directory"
)

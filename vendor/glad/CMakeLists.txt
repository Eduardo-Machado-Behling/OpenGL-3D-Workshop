#
# CMakeLists.txt for the GLAD library
#

# Create a static library named "glad" from the glad.c source file.
# A static library is linked into the final executable at compile time.
add_library(glad STATIC glad.c)

# Specify the public include directories for the 'glad' library.
# The "PUBLIC" keyword means that any other target that links against 'glad'
# will automatically get this include path added.
target_include_directories(glad PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

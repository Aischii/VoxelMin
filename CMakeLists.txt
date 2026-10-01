cmake_minimum_required(VERSION 3.20)
project(VoxelMin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(OpenGL REQUIRED)

if(WIN32)
    # Dependencies: GLFW (window/input), GLEW (GL loader), GLM (math), OpenGL.
    # On Windows/MinGW these are expected from the MSYS2 mingw64 prefix, which sits
    # next to the compiler. See docs/BUILD.md for the package list.
    get_filename_component(VOXELMIN_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(VOXELMIN_MINGW_ROOT "${VOXELMIN_COMPILER_DIR}/.." ABSOLUTE)

    set(_saved_suffixes "${CMAKE_FIND_LIBRARY_SUFFIXES}")
    # Prefer the static archives so the produced .exe has no GLFW/GLEW DLL deps.
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    find_library(GLFW3_LIBRARY NAMES glfw3 glfw
                 HINTS "${VOXELMIN_MINGW_ROOT}/lib" "${VOXELMIN_MINGW_ROOT}/lib64")
    find_library(GLEW_LIBRARY NAMES glew32 glew
                 HINTS "${VOXELMIN_MINGW_ROOT}/lib" "${VOXELMIN_MINGW_ROOT}/lib64")
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${_saved_suffixes}")

    if(NOT GLFW3_LIBRARY OR NOT GLEW_LIBRARY)
        message(FATAL_ERROR
            "Could not find GLFW/GLEW. Install them (MSYS2: pacman -S mingw-w64-x86_64-glfw mingw-w64-x86_64-glew) or see docs/BUILD.md.")
    endif()

    set(VOXELMIN_EXTRA_INCLUDES "${VOXELMIN_MINGW_ROOT}/include")
    set(VOXELMIN_EXTRA_LIBS "${GLFW3_LIBRARY}" "${GLEW_LIBRARY}" gdi32 user32 shell32 ole32)
    set(VOXELMIN_EXTRA_DEFS GLEW_STATIC)
else()
    # On Linux / UNIX, use CMake packages or pkg-config / system libraries.
    find_package(glfw3 3.3 QUIET)
    if(TARGET glfw)
        set(VOXELMIN_GLFW_TARGET glfw)
    elseif(TARGET glfw3)
        set(VOXELMIN_GLFW_TARGET glfw3)
    else()
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(GLFW3 QUIET glfw3)
        endif()
        if(GLFW3_FOUND)
            set(VOXELMIN_GLFW_TARGET "${GLFW3_LIBRARIES}")
            list(APPEND VOXELMIN_EXTRA_INCLUDES ${GLFW3_INCLUDE_DIRS})
        else()
            find_library(GLFW3_LIBRARY NAMES glfw3 glfw REQUIRED)
            set(VOXELMIN_GLFW_TARGET "${GLFW3_LIBRARY}")
        endif()
    endif()

    find_package(GLEW REQUIRED)
    if(TARGET GLEW::GLEW)
        set(VOXELMIN_GLEW_TARGET GLEW::GLEW)
    else()
        set(VOXELMIN_GLEW_TARGET "${GLEW_LIBRARIES}")
        list(APPEND VOXELMIN_EXTRA_INCLUDES "${GLEW_INCLUDE_DIRS}")
    endif()

    find_package(glm QUIET)
    if(TARGET glm::glm)
        set(VOXELMIN_GLM_TARGET glm::glm)
    else()
        find_path(GLM_INCLUDE_DIR "glm/glm.hpp")
        if(GLM_INCLUDE_DIR)
            list(APPEND VOXELMIN_EXTRA_INCLUDES "${GLM_INCLUDE_DIR}")
        endif()
    endif()

    # Search for stb include headers (e.g. /usr/include/stb, /usr/include, or third_party)
    find_path(STB_INCLUDE_DIR NAMES "stb/stb_image.h" "stb_image.h" PATH_SUFFIXES stb)
    if(STB_INCLUDE_DIR)
        list(APPEND VOXELMIN_EXTRA_INCLUDES "${STB_INCLUDE_DIR}")
    endif()

    find_package(Threads REQUIRED)

    set(VOXELMIN_EXTRA_LIBS
        ${VOXELMIN_GLFW_TARGET}
        ${VOXELMIN_GLEW_TARGET}
        ${VOXELMIN_GLM_TARGET}
        Threads::Threads
        ${CMAKE_DL_LIBS}
        m
    )
    set(VOXELMIN_EXTRA_DEFS "")
endif()

file(GLOB_RECURSE VOXELMIN_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(VoxelMin ${VOXELMIN_SOURCES})

target_include_directories(VoxelMin PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    ${VOXELMIN_EXTRA_INCLUDES}
)

if(VOXELMIN_EXTRA_DEFS)
    target_compile_definitions(VoxelMin PRIVATE ${VOXELMIN_EXTRA_DEFS})
endif()

if(MSVC)
    target_compile_options(VoxelMin PRIVATE /W4 /permissive-)
else()
    target_compile_options(VoxelMin PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32 AND NOT MSVC)
    # Statically link the GCC runtime (libstdc++, libgcc, winpthread) so the
    # executable runs without extra DLLs on PATH.
    target_link_options(VoxelMin PRIVATE -static)
endif()

target_link_libraries(VoxelMin PRIVATE
    ${VOXELMIN_EXTRA_LIBS}
    OpenGL::GL
)

# Keep the runtime asset tree next to the executable so it can be run directly.
add_custom_command(TARGET VoxelMin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:VoxelMin>/assets"
    COMMENT "Copying assets next to the executable"
)

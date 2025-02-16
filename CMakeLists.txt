cmake_minimum_required(VERSION 3.0...3.25)

project(
    DECALS 
    VERSION 0.0.1 
    DESCRIPTION "Different decal rendering techniques"
)

# add_definitions(-std=c++17)
set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME "App")

set(SOURCE_PATH ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_PATH ${PROJECT_SOURCE_DIR}/include)
set(SUBMODULES_PATH ${PROJECT_SOURCE_DIR}/submodules)
# Shaders
set(SHADERS_PATH ${PROJECT_SOURCE_DIR}/shaders)

# Add subdirectory based lib generation. It compiles examples
# we don't want this. 
option(BASED_ON_SUBDIRECTORY "Use existing CMakeLists.txt from each submodule. This has the issue that it creates the test projects from each submodule." OFF)

# Try and compile the original project to EMCC
option(ORIGINAL_PROJECT "Compile original project" ON)
if(ORIGINAL_PROJECT)
    set(MAIN_SRC ${SOURCE_PATH}/mainOne.cpp)
else()
    set(MAIN_SRC ${SOURCE_PATH}/main.cpp)
endif()
message(STATUS "Main src file: ${MAIN_SRC}")

if(BASED_ON_SUBDIRECTORY)
    # Add GLM
    add_subdirectory(${SUBMODULES_PATH}/glm)

    # Add tinyGLTF
    add_subdirectory(${SUBMODULES_PATH}/tinygltf)

    # Add tinyobjloader
    add_subdirectory(${SUBMODULES_PATH}/tinyobjloader)

    # NanoRT
    add_subdirectory(${SUBMODULES_PATH}/nanort)

else()
    # Add GLM
    set(GLM_PATH ${SUBMODULES_PATH}/glm)
    set(GLM_SOURCE
        ${GLM_PATH}/glm/glm.hpp
        ${GLM_PATH}/glm/gtc/matrix_transform.hpp
        ${GLM_PATH}/glm/gtc/type_ptr.hpp
        ${GLM_PATH}/glm/gtx/hash.hpp
        ${GLM_PATH}/glm/gtx/string_cast.hpp
    )
    # Note the INTERFACE param given it's header only.
    add_library(glm INTERFACE
        ${GLM_SOURCE}
    )
    target_include_directories(glm INTERFACE
        ${GLM_PATH}
    )

    # Add tinyGLTF
    set(TINYGLTF_PATH ${SUBMODULES_PATH}/tinygltf)
    set(TINYGLTF_SOURCE
        ${TINYGLTF_PATH}/tiny_gltf.h
        ${TINYGLTF_PATH}/tiny_gltf.cc
    )
    add_library(tinygltf STATIC
        ${TINYGLTF_SOURCE}
    )
    target_include_directories(tinygltf PUBLIC
        ${TINYGLTF_PATH}
    )

    # Add tinyobjloader
    set(TINYOBJLOADER_PATH ${SUBMODULES_PATH}/tinyobjloader)
    set(TINYOBJLOADER_SOURCE
        ${TINYOBJLOADER_PATH}/tiny_obj_loader.h
        ${TINYOBJLOADER_PATH}/tiny_obj_loader.cc
    )
    add_library(tinyobjloader STATIC
        ${TINYOBJLOADER_SOURCE}
    )
    target_include_directories(tinyobjloader PUBLIC
        ${TINYOBJLOADER_PATH}
    )

    # Add NanoRT
    set(NANORT_PATH ${SUBMODULES_PATH}/nanort)
    add_library(nanort STATIC
        ${NANORT_PATH}/nanort.h
        ${NANORT_PATH}/nanort.cc
    )
    target_include_directories(nanort PUBLIC
        ${NANORT_PATH}
    )
endif()
# The following libs don't provide CMakeLists.txt's out of the box.

# Add ImGui
set(IMGUI_PATH ${SUBMODULES_PATH}/imgui)
set(IMGUI_SRC
    ${IMGUI_PATH}/imgui.h
    ${IMGUI_PATH}/imgui.cpp
    ${IMGUI_PATH}/imgui_demo.cpp
    ${IMGUI_PATH}/imgui_draw.cpp
    ${IMGUI_PATH}/imgui_tables.cpp
    ${IMGUI_PATH}/imgui_widgets.cpp
    ${IMGUI_PATH}/imgui_demo.cpp
    ${IMGUI_PATH}/backends/imgui_impl_sdl2.h
    ${IMGUI_PATH}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_PATH}/backends/imgui_impl_opengl3.h
    ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_PATH}/imgui_internal.h
)
add_library(imgui STATIC
    ${IMGUI_SRC}
)

target_include_directories(imgui PUBLIC
    ${IMGUI_PATH}
)

# Add ImGuizmo
set(IMGUIZMO_PATH ${SUBMODULES_PATH}/ImGuizmo)
add_library(ImGuizmo STATIC
    ${IMGUIZMO_PATH}/ImGuizmo.h
    ${IMGUIZMO_PATH}/ImGuizmo.cpp
)
target_include_directories(ImGuizmo PUBLIC 
    ${IMGUI_PATH}
    ${IMGUIZMO_PATH}
)

#file(GLOB_RECURSE HS "${INCLUDE_PATH}/*.h")
#MESSAGE(STATUS "HS: ${HS}")
# file(GLOB_RECURSE CPPS "${SOURCE_PATH}/*.cpp")

if(DEFINED EMSCRIPTEN)
    message(STATUS "This is an EMSCRIPTEN build!")

    # Set sources for main
    add_executable(${PROJECT_NAME}
        ${INCLUDE_PATH}/OpenGLTF.h
        ${INCLUDE_PATH}/shader.h
        ${MAIN_SRC}
    )

    # Set includes for main
    target_include_directories(${PROJECT_NAME} PUBLIC
        ${INCLUDE_PATH}
    )

    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(USE_FLAGS "-std=c++17 -g -s USE_SDL=2")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${USE_FLAGS})
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS})
    set(CMAKE_EXECUTABLE_SUFFIX .js)

    target_link_libraries(${PROJECT_NAME} PUBLIC
        glm
        tinygltf
        tinyobjloader
        imgui
        ImGuizmo
        nanort
    )

    set(SHADERS_PATH_EMSCRIPTEN ${PROJECT_SOURCE_DIR}/shaders@/shaders)
    set(ASSETS_PATH_EMSCRIPTEN ${PROJECT_SOURCE_DIR}/Assets@/Assets)
    
    # Runtime modules.
    set(RUNTIME_MODULES         "-s EXPORTED_FUNCTIONS=\"['_main','_malloc','_free']\" ")
    list(APPEND RUNTIME_MODULES "-s EXPORTED_RUNTIME_METHODS=\"['ccall']\"")
    #[[list(APPEND RUNTIME_MODULES "-s EXPORTED_RUNTIME_METHODS=[")
    list(APPEND RUNTIME_MODULES "\"ccall\",")
    list(APPEND RUNTIME_MODULES "\"_malloc\"")
    list(APPEND RUNTIME_MODULES "] ")]]
    list(JOIN RUNTIME_MODULES "" RUNTIME_MODULES)

    MESSAGE(STATUS "RUNTIME_MODULES ${RUNTIME_MODULES}")

    option(DEBUG_EMSCRIPTEN_OPENGL "Debug options for OpenGL!" OFF)

    # Emscripten Compile and Link flags.
    set(EMSCRIPTEN_OPTIMIZATION_LEVEL "-O3 ")
    set(EMSCRIPTEN_FLAGS "")
    list(APPEND EMSCRIPTEN_FLAGS ${EMSCRIPTEN_OPTIMIZATION_LEVEL})
    list(APPEND EMSCRIPTEN_FLAGS "-s WASM=1 ")
    list(APPEND EMSCRIPTEN_FLAGS "-s FULL_ES3=1 ")
    list(APPEND EMSCRIPTEN_FLAGS "-s GL_DEBUG=1 ")
    list(APPEND EMSCRIPTEN_FLAGS "-s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 ")
    list(APPEND EMSCRIPTEN_FLAGS "--embed-file ${SHADERS_PATH_EMSCRIPTEN} ")
    list(APPEND EMSCRIPTEN_FLAGS "--preload-file ${ASSETS_PATH_EMSCRIPTEN} ")
    list(APPEND EMSCRIPTEN_FLAGS "-s FORCE_FILESYSTEM=1 ")
    list(APPEND EMSCRIPTEN_FLAGS "-s ALLOW_MEMORY_GROWTH=1 ")
    list(APPEND EMSCRIPTEN_FLAGS ${RUNTIME_MODULES})
    if(DEBUG_EMSCRIPTEN_OPENGL)
        list(APPEND EMSCRIPTEN_FLAGS "-s GL_DEBUG=1 ")
        list(APPEND EMSCRIPTEN_FLAGS "-s GL_ASSERTIONS=1 ")
    else()
    endif()
    list(APPEND EMSCRIPTEN_FLAGS "-s ASSERTIONS=1")
    list(JOIN EMSCRIPTEN_FLAGS " " EMSCRIPTEN_FLAGS)

    MESSAGE(STATUS "EMSCRIPTEN_FLAGS ${EMSCRIPTEN_FLAGS}")

    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${EMSCRIPTEN_OPTIMIZATION_LEVEL})
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS    ${EMSCRIPTEN_FLAGS})

    #set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-03")
	#set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS    "-O3 -s WASM=1 -s FULL_ES3=1 -s GL_DEBUG=1 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 --embed-file '${SHADERS_PATH_EMSCRIPTEN}' --preload-file '${ASSETS_PATH_EMSCRIPTEN}' -s ASSERTIONS")

    # Add index.html to the wasm build folder.
    if(ORIGINAL_PROJECT)
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/wasm/indexOne.html
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
        )
    else()
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/wasm/index.html
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
        )
    endif()
    # Add wasm-arrays.js to the wasm build folder.
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/wasm/wasm-arrays.min.js
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    )
    # Add styles.css to the wasm build folder.
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/wasm/styles.css
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    )

else()
    message(STATUS "This is not an EMSCRIPTEN build!")

    # Add shader files
    set(SHADERS_PATH ${PROJECT_SOURCE_DIR}/shaders)
    file(GLOB_RECURSE SHADERS 
        ${SHADERS_PATH}/*.glsl
        ${SHADERS_PATH}/*.vert
        ${SHADERS_PATH}/*.frag
    )
    # Pass the shaders as a header for the VS project to recognize them.
    set_source_files_properties(${SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
    # Copy shaders to build folder so that the .exe runs on Windows.
    #set(shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)
    #file(COPY ${SHADERS} DESTINATION shaders)

    # Set sources for main. Note the SHADERS variable, this holds the shaders so 
    # that they appear on the VS project for editing.
    add_executable(${PROJECT_NAME}
        ${INCLUDE_PATH}/OpenGLTF.h
        ${INCLUDE_PATH}/shader.h
        ${MAIN_SRC}
        ${SHADERS}
    )

    # Set includes for main
    target_include_directories(${PROJECT_NAME} PUBLIC
        ${INCLUDE_PATH}
    )
    
    # We need this to be able to compile SHARED/dynamic libraries in Windows.
    if(WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
        set(BUILD_SHARED_LIBS TRUE)
    endif()

    # Find SDL2 and OpenGL
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)

    # Link SDL to ImGui
    target_link_libraries(imgui PUBLIC
        SDL2::SDL2 
        SDL2::SDL2main
    )

    # Add GLAD sources
    add_library(glad STATIC
        ${SOURCE_PATH}/glad.c
        ${INCLUDE_PATH}/glad/glad.h
        ${INCLUDE_PATH}/KHR/khrplatform.h
    )
    target_include_directories(glad PUBLIC 
        ${INCLUDE_PATH}
    )

    target_link_libraries(${PROJECT_NAME} PUBLIC
        SDL2::SDL2 
        SDL2::SDL2main
        OpenGL::GL
        glad
        glm
        tinygltf
        tinyobjloader
        imgui
        ImGuizmo
        nanort
    )

    # Set Windows-specific properties
    if(WIN32)
        add_definitions(-DWIN32_LEAN_AND_MEAN)
        target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED)  # Needed to avoid WinMain issues
    endif()

    # Windows only
    if(WIN32)
        # Copy SDL2.dll to the output directory on Windows
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:SDL2::SDL2>/SDL2.dll
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
        #message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
        # Create a shaders folder on the build folder.
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/build/shaders
        )
        # Copy shaders to build folder so that the .exe runs on Windows.
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SHADERS}
            ${CMAKE_CURRENT_SOURCE_DIR}/build/shaders
        )
    endif()
endif()


// #include <iostream>
// #include "calc.h"

// // #define GLM_ENABLE_EXPERIMENTAL
// // #define GLM_FORCE_SWIZZLE
// // #include <glm/glm.hpp>
// // #include <glm/gtc/matrix_transform.hpp>
// // #include <glm/gtc/type_ptr.hpp>
// // #include <glm/gtx/hash.hpp>
// // #include <glm/gtx/string_cast.hpp>

// // #define TINYGLTF_IMPLEMENTATION
// // #define STB_IMAGE_IMPLEMENTATION
// // #define STB_IMAGE_WRITE_IMPLEMENTATION
// // // #include "stb_image.h"
// // #define TINYGLTF_NOEXCEPTION
// // #define JSON_NOEXCEPTION
// // #include "tiny_gltf.h"

// // #define IMGUI_DEFINE_MATH_OPERATORS
// // #include "imgui.h"
// // #include "backends/imgui_impl_sdl3.h"
// // #include "backends/imgui_impl_opengl3.h"
// // #include "imgui_internal.h"

// // #include "ImGuizmo.h"

// // //#include <emscripten.h>
// // //#include <GLES3/gl3.h>
// // #include <functional>
// // #include <string>
// // #include <bitset>
// // #include <unordered_map>

// // #include <chrono>
// // //#include "Include/Shader.h"
// // //#include "Include/OpenGLTF.h"

// // #define TINYOBJLOADER_IMPLEMENTATION
// // #include "tiny_obj_loader.h"

// // #include "nanort.h"

// using namespace std;

// int main()
// {
    
//     const int a = 10,
//               b = 12;
    
//     int c = add(a, b);
//     int d = subtract(a, b);
    
//     std::cout << "A: " << a << " + " << b << " = " << c << "\n";
//     std::cout << "A: " << a << " - " << b << " = " << d << std::endl;
    
//     // nanort::Ray<float> ray;

//     system("pause");

//     return 0;
// }

#include <iostream>
#include <functional>

#ifdef __EMSCRIPTEN__
    #include <emscripten/emscripten.h>
    #define glGenVertexArrays glGenVertexArraysOES
    #define glBindVertexArray glBindVertexArrayOES
    #define GL_GLEXT_PROTOTYPES 1
    #include <SDL_opengles2.h>
#else
    #include <glad/glad.h>
    #define EMSCRIPTEN_KEEPALIVE
#endif
#include <SDL.h>

using namespace std;

// Shader sources
const GLchar* vertexSource =
    "#version 300 es                              \n"
    "precision highp float;                       \n"    
    "layout (location = 0) in vec3 position;      \n"
    "layout (location = 1) in vec3 vertexColor;   \n" 
    "out vec3 color;                              \n"   
    "void main()                                  \n"
    "{                                            \n"
    "  color = vertexColor;                       \n"
    "  gl_Position = vec4(position, 1.0);         \n"
    "}                                            \n";
const GLchar* fragmentSource =
    "#version 300 es                              \n"
    "precision highp float;                       \n" 
    "in vec3 color;                               \n"
    "out vec4 fragColor;                          \n"
    "void main()                                  \n"
    "{                                            \n"
    "fragColor = vec4(color, 1.0);                \n"
    "}                                            \n";

// an example of something we will control from the javascript side
bool background_is_black = true;

// the function called by the javascript code
extern "C" void EMSCRIPTEN_KEEPALIVE toggle_background_color() { background_is_black = !background_is_black; }

static bool main_loop_running = true;
void main_loop();

SDL_Window* window;

int main()
{
    std::cout << "HelloWorld!" << std::endl;
    SDL_CreateWindowAndRenderer(640, 480, 0, &window, nullptr);

    #ifdef __EMSCRIPTEN__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    #else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    #endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    float points[] = {
        0.0f,  0.5f,  0.0f,
        0.5f, -0.5f,  0.0f,
        -0.5f, -0.5f,  0.0f
    };

    float colours[] = {
        1.0f, 0.0f,  0.0f,
        0.0f, 1.0f,  0.0f,
        0.0f, 0.0f,  1.0f
    };

    GLuint points_vbo = 0;
    glGenBuffers(1, &points_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), points, GL_STATIC_DRAW);

    GLuint colours_vbo = 0;
    glGenBuffers(1, &colours_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
    glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), colours, GL_STATIC_DRAW);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, colours_vbo);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    // Link the vertex and fragment shader into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, true);
    #else
    while (main_loop_running)
    {
        main_loop();
    }
    #endif

    window = nullptr;
    SDL_Quit();

    return EXIT_SUCCESS;
}

void main_loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
            {
                main_loop_running = false;
                break;
            }
            case SDL_KEYUP:
            {
                int sym = event.key.keysym.sym;
                if (sym == SDLK_ESCAPE)
                {
                    main_loop_running = false;
                }
                break;
            }
        }
    }
    // Clear the screen
    if( background_is_black )
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw a triangle from the 3 vertices
    glDrawArrays(GL_TRIANGLES, 0, 3);

    SDL_GL_SwapWindow(window);
}

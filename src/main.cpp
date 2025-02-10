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
#include "shader.h"

using namespace std;

// an example of something we will control from the javascript side
bool background_is_black = true;

// the function called by the javascript code
extern "C" void EMSCRIPTEN_KEEPALIVE toggle_background_color() { background_is_black = !background_is_black; }

// Window dimensions
const int SCREEN_WIDTH  = 640;
const int SCREEN_HEIGHT = 480;

// Initialization function.
bool init();

// Main loop, we need this given that we are targeting multiple platforms.
void main_loop();

SDL_Window* window;
SDL_GLContext glContext;
static bool main_loop_running = true;

int main()
{
    if (!init())
    {
        return -1;
    }

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

    Shader shader = Shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    shader.use();

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

bool init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    // Set OpenGL attributes
#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    // Create SDL window
    window = SDL_CreateWindow("OpenGL with SDL2 & GLAD",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) 
    {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create OpenGL context
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) 
    {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

#ifndef __EMSCRIPTEN__
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return false;
    }
#endif

    // Print OpenGL version
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    return true;
}

void mouse_press(SDL_MouseButtonEvent& button)
{
    if (button.button == SDL_BUTTON_LEFT)
    {
        background_is_black = !background_is_black;
    }
}

void main_loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_MOUSEBUTTONDOWN:
            {
                mouse_press(event.button);
                break;
            }
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
            default:
            {
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

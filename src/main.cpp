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

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

#include "ImGuizmo.h"

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

#include "nanort.h"

#include "shader.h"
#include "OpenGLTF.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

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

// Initialize imgui.
ImGuiIO initImgui();

// Transform ImGuizmo helper function.
extern "C" void EMSCRIPTEN_KEEPALIVE editTransform(float* cameraView, float* cameraProjection, float* matrix, bool editTransformDecomposition, ImGuiIO& io);

// Main loop, we need this given that we are targeting multiple platforms.
void main_loop();

SDL_Window* window;
SDL_GLContext glContext;
const char* glsl_version = "#version 300 es";
static bool main_loop_running = true;
static bool show_demo_window = true;

// Setup Dear ImGui context
//ImGuiIO& io = ImGui::GetIO();
static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
static bool useWindow = false;

glm::mat4 model = glm::mat4(1);
glm::mat4 view = glm::mat4(1);
glm::mat4 projectionHalf = glm::mat4(1);
int lastUsing = 0;

int main()
{
    // NanoRT
    nanort::Ray<float> ray;
    ray.org[0] = 0.0f;
    ray.org[1] = 1.0f;
    ray.org[2] = 2.0f;

    ray.dir[0] = 0.0f;
    ray.dir[1] = 1.0f;
    ray.dir[2] = 2.0f;

    float kFar = 1.0e+30f;
    ray.min_t = 0.0f;
    ray.max_t = kFar;
    if (!init())
    {
        return -1;
    }

    ImGuiIO io = initImgui();

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

    #ifdef __EMSCRIPTEN__
    Shader shader = Shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    shader.use();
    emscripten_set_main_loop(main_loop, 0, true);
    #else
    Shader shader = Shader("../shaders/vertex.glsl", "../shaders/fragment.glsl");
    shader.use();
    while (main_loop_running)
    {
        main_loop();
    }
    #endif

    // Clean after ourselves
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
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

ImGuiIO initImgui()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = NULL;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return io;
}

void editTransform(float* cameraView, float* cameraProjection, float* matrix, bool editTransformDecomposition, ImGuiIO& io)
{
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x / 2, io.DisplaySize.y);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
    static bool useSnap = false;
    static float snap[3] = { 1.f, 1.f, 1.f };
    static float bounds[] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
    static float boundsSnap[] = { 0.1f, 0.1f, 0.1f };
    static bool boundSizing = false;
    static bool boundSizingSnap = false;

    if (editTransformDecomposition)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_T))
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E))
            mCurrentGizmoOperation = ImGuizmo::SCALE;
        if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
            mCurrentGizmoOperation = ImGuizmo::SCALE;
        if (ImGui::RadioButton("Universal", mCurrentGizmoOperation == ImGuizmo::UNIVERSAL))
            mCurrentGizmoOperation = ImGuizmo::UNIVERSAL;
        float matrixTranslation[3], matrixRotation[3], matrixScale[3];
        ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation, matrixRotation, matrixScale);
        ImGui::InputFloat3("Tr", matrixTranslation);
        ImGui::InputFloat3("Rt", matrixRotation);
        ImGui::InputFloat3("Sc", matrixScale);
        ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix);

        if (mCurrentGizmoOperation != ImGuizmo::SCALE)
        {
            if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
                mCurrentGizmoMode = ImGuizmo::LOCAL;
            ImGui::SameLine();
            if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
                mCurrentGizmoMode = ImGuizmo::WORLD;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_P))
            useSnap = !useSnap;
        ImGui::Checkbox("##UseSnap", &useSnap);
        ImGui::SameLine();

        switch (mCurrentGizmoOperation)
        {
        case ImGuizmo::TRANSLATE:
            ImGui::InputFloat3("Snap", &snap[0]);
            break;
        case ImGuizmo::ROTATE:
            ImGui::InputFloat("Angle Snap", &snap[0]);
            break;
        case ImGuizmo::SCALE:
            ImGui::InputFloat("Scale Snap", &snap[0]);
            break;
        default:
            break;
        }
        ImGui::Checkbox("Bound Sizing", &boundSizing);
        if (boundSizing)
        {
            ImGui::PushID(3);
            ImGui::Checkbox("##BoundSizing", &boundSizingSnap);
            ImGui::SameLine();
            ImGui::InputFloat3("Snap", boundsSnap);
            ImGui::PopID();
        }
    }

    //ImGuiIO& io = ImGui::GetIO();
    float viewManipulateRight = io.DisplaySize.x;
    float viewManipulateTop = 0;
    static ImGuiWindowFlags gizmoWindowFlags = 0;
    if (useWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_Appearing);
        ImGui::SetNextWindowPos(ImVec2(400, 20), ImGuiCond_Appearing);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImVec4)ImColor(0.35f, 0.3f, 0.3f));
        ImGui::Begin("Gizmo", 0, gizmoWindowFlags);
        ImGuizmo::SetDrawlist();
        float windowWidth = (float)ImGui::GetWindowWidth();
        float windowHeight = (float)ImGui::GetWindowHeight();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
        viewManipulateRight = ImGui::GetWindowPos().x + windowWidth;
        viewManipulateTop = ImGui::GetWindowPos().y;
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        gizmoWindowFlags = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(window->InnerRect.Min, window->InnerRect.Max) ? ImGuiWindowFlags_NoMove : 0;
    }
    else
    {
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x / 2, io.DisplaySize.y);
    }
    ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, useSnap ? &snap[0] : NULL, boundSizing ? bounds : NULL, boundSizingSnap ? boundsSnap : NULL);
    std::cout << "Edit transform!" << std::endl;
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
        ImGui_ImplSDL2_ProcessEvent(&event);
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

    // imgui
    // Start the Dear ImGui and ImGuizmo frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
    
    ImGui::Begin("GUI");
    if (ImGui::Button("Hey"))
    {
        show_demo_window = !show_demo_window;
    }    

    /*for (int i = 0; i < 1; ++i)
    {
        ImGuizmo::SetID(i);
        editTransform(glm::value_ptr(view), glm::value_ptr(projectionHalf), glm::value_ptr(model), lastUsing == i, io);
        if (ImGuizmo::IsUsing())
        {
            lastUsing = i;
        }
    }*/

    ImGui::End();

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

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

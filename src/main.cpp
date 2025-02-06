#include <iostream>
#include "calc.h"

// #define GLM_ENABLE_EXPERIMENTAL
// #define GLM_FORCE_SWIZZLE
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>
// #include <glm/gtx/hash.hpp>
// #include <glm/gtx/string_cast.hpp>

// #define TINYGLTF_IMPLEMENTATION
// #define STB_IMAGE_IMPLEMENTATION
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// // #include "stb_image.h"
// #define TINYGLTF_NOEXCEPTION
// #define JSON_NOEXCEPTION
// #include "tiny_gltf.h"

// #define IMGUI_DEFINE_MATH_OPERATORS
// #include "imgui.h"
// #include "backends/imgui_impl_sdl3.h"
// #include "backends/imgui_impl_opengl3.h"
// #include "imgui_internal.h"

// #include "ImGuizmo.h"

// //#include <emscripten.h>
// //#include <GLES3/gl3.h>
// #include <functional>
// #include <string>
// #include <bitset>
// #include <unordered_map>

// #include <chrono>
// //#include "Include/Shader.h"
// //#include "Include/OpenGLTF.h"

// #define TINYOBJLOADER_IMPLEMENTATION
// #include "tiny_obj_loader.h"

// #include "nanort.h"

using namespace std;

int main()
{
    
    const int a = 10,
              b = 12;
    
    int c = add(a, b);
    int d = subtract(a, b);
    
    std::cout << "A: " << a << " + " << b << " = " << c << "\n";
    std::cout << "A: " << a << " - " << b << " = " << d << std::endl;
    
    // nanort::Ray<float> ray;

    system("pause");

    return 0;
}
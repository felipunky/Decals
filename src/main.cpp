#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

#include "ImGuizmo.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/emscripten.h>
    #define glGenVertexArrays glGenVertexArraysOES
    #define glBindVertexArray glBindVertexArrayOES
    #define GL_GLEXT_PROTOTYPES 1
    #include <GLES3/gl3.h>
    #include <SDL_opengles2.h>
#else
    #include <glad/glad.h>
    #define EMSCRIPTEN_KEEPALIVE
#endif

#include <functional>
#include <string>
#include <bitset>
#include <unordered_map>

#include <chrono>
#include "shader.h"
#include "OpenGLTF.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "nanort.h"

#define PROJECTOR_DISTANCE 50.0f
#define FAR_PLANE  1000.0f
#define NEAR_PLANE 1e-5
#define RADIANS_30 glm::radians(30.0f)
#define RADIANS_45 glm::radians(45.0f)

int WIDTH  = 800,
    HEIGHT = 600;
int halfWidth = WIDTH / 2, halfHeight = HEIGHT / 2,
                quarterHeight = halfHeight / 2;

const float SPEED = 5.0f;
// How much time between frames.
float deltaTime = 0.0f, lastFrame = 0.0f;

SDL_Window* window;
SDL_GLContext context;
const char* glsl_version = nullptr;//"#version 300 es";
static bool main_loop_running = true;

int newVerticesSize;
int flipAlbedo = 0;
uint8_t reload = 0u;
uint8_t downloadImage = 1u;
std::vector<uint8_t> decalImageBuffer;
std::vector<uint8_t> decalResult;
uint16_t widthDecal = 1127u, heightDecal = 699u, changeDecal = 1u,
         widthAlbedo,        heightAlbedo,       changeAlbedo = 0u;
int flip = 0;
bool flipDecal = false;
Shader depthPrePass = Shader(),
       geometryPass = Shader(),
       deferredPass = Shader(),
       hitPosition  = Shader(),
       decalsPass   = Shader();
// Pass GLSL version to the shaders.
#if defined(_WIN32)
std::string GLSLVersion = "#version 150 core\n";
#elif defined(__APPLE__)
std::string GLSLVersion = "#version 150 core\n";
#elif defined(__EMSCRIPTEN__)
std::string GLSLVersion = "#version 300 es\n";
#endif
// Buffers.
unsigned int fbo, dBuffer;
// Attached textures.
unsigned int renderAlbedo, rboDepth;
bool insideImGui   = false,
     keyboardImGui = false;

float nearPlane     = 0.1f;
float farPlane      = 200.0f;
float focalDistance = 3.0f;
float radius        = 1.0f;

glm::vec3 camPos = glm::vec3( 0.0f, 2.5f, 5.0f );
glm::vec3 camFront = glm::vec3( 0.0f, 0.0f, -1.0f );
glm::vec3 camUp = glm::vec3( 0.0f, 1.0f, 0.0f );
glm::vec3 differenceBboxMaxMin;

//glm::mat4 model         = glm::mat4(1.0f);
glm::mat4 view          = glm::mat4(1.0f);
glm::mat4 projection    = glm::mat4(1.0f);
glm::mat4 modelNoGuizmo = glm::mat4(1.0f);
glm::mat4 viewPinned    = glm::mat4(1.0f);
glm::vec2 widthHeight   = glm::vec2(WIDTH, HEIGHT);
glm::vec2 currentWindowWidthAndHeight = widthHeight;
bool isActiveFPS = false;

 /**
 * Start User Interaction
 */
TYPE_OF_CAMERA CAMERA = TRACK_BALL;
// OpenGL is right handed so the last component corresponds to move forward/backwards.
int mousePositionX, mousePositionY;
glm::vec3 mouse;
bool mouseInsideOfWindow;
bool clicked = false;

float time_internal = 0.0f;
// Mouse rotation globals.
bool firstMouse = true;
float lastX = ( float )( WIDTH ) * 0.5f, 
        lastY = ( float )( HEIGHT ), 
        yaw = -90.0f, 
        pitch = 0.0f;

/**
* Start Camera Definitions
*/
// https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/some-interaction/camera-control.html
struct CameraState
{
    // angles.x is the rotation of the camera around the global vertical axis, affected by mouse.x
    // angles.y is the rotation of the camera around its local horizontal axis, affected by mouse.y
    glm::vec2 angles = { 0.0f, 0.0f };
    // zoom is the position of the camera along its local forward axis, affected by the scroll wheel
    float zoom = -10.0f;
    // position is the center on to which the camera rotates around.
    glm::vec2 position = { 0.0f, 0.0f };
};
CameraState m_cameraState;

struct DragState {
    // Whether a drag action is ongoing (i.e., we are between mouse press and mouse release)
    bool active = false;
    // Mouse wheel being pressed.
    bool mouseWheelButtonActive = false;
    uint mouseWheelButtonActiveSince = 0u;
    // The position of the mouse at the beginning of the drag action
    glm::vec2 startMouse;
    // The position of the mouse at the beginning of the mouse wheel button action.
    glm::vec2 mouseWheelActionStartMouse;
    // The camera state at the beginning of the drag action
    CameraState startCameraState;

    // Constant settings
    float sensitivity = 0.01f;
    float scrollSensitivity = 1.0f;
};
DragState m_drag;
/**
* End Camera Definitions
*/

void updateViewMatrix();

/**
 * End User Interaction
 */

/** Start ImGui params. */
bool showBBox      = false;
bool showProjector = false;
bool showHitPoint  = false;
bool normalMap     = false;
bool splitScreen   = false;

int scale = 1;
float blend = 0.5f;

float zoomSide = 0.5f,
        zoomTop  = zoomSide;
/** End ImGui params. */

unsigned int frame   = 0,
             counter = 0,
             flipper = 0;

float iTime;

enum MODEL
{
    WORKBOOT,
    CLAY,
    SHIRT
};
MODEL currentModel = SHIRT;

std::string fileToString(const std::string& fileName)
{
    std::ifstream in(fileName);
    std::string contents((std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>());
    return contents;
}

std::vector<uint8_t> uploadImage()
{
    unsigned int bufferSize = 4 * geometryPass.Width * geometryPass.Height;
    std::vector<uint8_t> buffer(bufferSize);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, geometryPass.Width, geometryPass.Height, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());
    return buffer;
}

std::vector<uint8_t> loadArray(uint8_t* buf, int bufSize)
{
#ifdef OPTIMIZE
#else
    std::cout << "Buffer size: " << bufSize << std::endl;
#endif
    std::vector<uint8_t> result(buf, buf + bufSize);
    return result;
}

struct BBox
{
    glm::vec3 bboxMin = glm::vec3(1e+5f);
    glm::vec3 bboxMax = glm::vec3(-1e+5f);
    glm::vec3 centroid = glm::vec3(0.0f);
};

struct BVO
{
    nanort::BVHAccel<float> accel;
    glm::vec3 hitPos = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 hitNor = glm::vec3(1.0, 1.0, 1.0);
    glm::mat4 decalProjector = glm::mat4(1.0f);
    bool isTracing = false;
};

struct Material
{
    unsigned int baseColor;
    unsigned int normal;
    unsigned int roughness;
    unsigned int metallic;
    unsigned int ao;
    unsigned int decalBaseColor;
};

struct OpenGLObject
{
    unsigned int VBOVertices,
        VBONormals,
        VBOTextureCoordinates,
        VBOTangents,
        VAO,
        EBO;
};

struct ModelData
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> textureCoordinates;
    std::vector<glm::vec4> tangents;
    std::vector<unsigned int> indexes;
    glm::mat4 modelMatrix;
    BBox Bbox;
    BVO bvo;
    Material material;
    OpenGLObject openGLObject;
};

ModelData modelData;

struct ModelFileNames
{
    std::string mesh,
                baseColor,
                normal,
                roughness,
                metallic,
                ao,
                decalBaseColor;
};

std::vector<ModelFileNames> fileNames(2);
std::vector<ModelData> modelsData(fileNames.size());

const void printModelData(const ModelData& modelData)
{
    std::cout << "Vertices size: "         << modelData.vertices.size()                    << "\n" <<
                 "Normals size: "          << modelData.normals.size()                     << "\n" <<
                 "Tangents size: "         << modelData.tangents.size()                    << "\n" <<
                 "Indices size: "          << modelData.indexes.size()                     << "\n" <<
                 "OpenGL VBO Vertices: "   << modelData.openGLObject.VBOVertices           << "\n" <<
                 "OpengGL VBO Normals: "   << modelData.openGLObject.VBONormals            << "\n" <<
                 "OpengGL VBO Tangents: "  << modelData.openGLObject.VBOTangents           << "\n" <<
                 "OpengGL VBO TexCoords: " << modelData.openGLObject.VBOTextureCoordinates << "\n" <<
                 "OpengGL EBO: "           << modelData.openGLObject.EBO                   << "\n" <<
                 "OpengGL VAO: "           << modelData.openGLObject.VAO                   << "\n" <<
    std::endl;
}

void reloadModel(ModelData& modelData);
void ObjLoader(std::string inputFile, ModelData& modelData);
void loadGLTF(tinygltf::Model& model, ModelData& modelData);
void recomputeCamera();

struct frameBuffer
{
    unsigned int framebuffer;
    unsigned int texture;
};

frameBuffer depthFramebuffer;
frameBuffer textureSpaceFramebuffer;

bool isGLTF = false;

// We need this so that we don't need to add each module to the Emscripten build.
extern "C"
{
    EMSCRIPTEN_KEEPALIVE
    void passWindowSize(uint16_t* buf, int bufSize)
    {
        std::cout << "Buffer size: " << bufSize << " window's width: " << 
                     +buf[0] << " window's height: " << +buf[1] << std::endl;
        currentWindowWidthAndHeight = glm::vec2((float) buf[0], (float) buf[1]);                     
        currentWindowWidthAndHeight.y = currentWindowWidthAndHeight.x * 0.75;
        WIDTH = (int) buf[0];
        HEIGHT = (int) currentWindowWidthAndHeight.y;
    }
    EMSCRIPTEN_KEEPALIVE
    void passGLTF(char* buf)
    {
        std::string result = buf;
        tinygltf::Model model;
        std::vector<unsigned char> data(result.begin(), result.end());
        std::cout << "Data GLTF size: " << data.size() << std::endl;
        std::string err, warn;
        bool res = GLTF::GetGLTFModel(&model, err, warn, data);
        //assert(res);
        if (!res)
        {
            #ifdef EXCEPTIONS
            throw std::runtime_error("Unable to read GLTF!");
            #else
            std::cout << "Unable to read GLTF!" << std::endl;
            std::cout << "Error: " << err << std::endl;
            std::cout << "Warning: " << warn << std::endl;
            #endif
        }
        loadGLTF(model, modelData);
        reloadModel(modelData);
        //recomputeCamera();
        isGLTF = true;
        data.clear();
    }
    EMSCRIPTEN_KEEPALIVE
    void passGLB(uint8_t* buf, int bufSize)
    {
        tinygltf::Model model;
        std::vector<unsigned char> data = loadArray(buf, bufSize);
        std::cout << "Data GLB size: " << data.size() << std::endl;
        std::string err, warn;
        bool res = GLTF::GetGLBModel(&model, err, warn, data);
        if (!res)
        {
            #ifdef EXCEPTIONS
            throw std::runtime_error("Unable to read GLTF!");
            #else
            std::cout << "Unable to read GLB!" << std::endl;
            std::cout << "Error: " << err << std::endl;
            std::cout << "Warning: " << warn << std::endl;
            #endif
        }
        loadGLTF(model, modelData);
        reloadModel(modelData);
        //recomputeCamera();
        isGLTF = true;
        data.clear();
    }
    EMSCRIPTEN_KEEPALIVE
    void passObj(char* buf)
    {
        //std::cout << "New mesh content: " << buf << std::endl;
        //clearBBox(bboxMin, bboxMax, centroid);
        std::cout << "BBox Cleared Center: {x: " << modelData.Bbox.centroid.x << ", y: " << modelData.Bbox.centroid.y << ", z:" << modelData.Bbox.centroid.z << "}\n";
        std::string result = buf;
        ObjLoader(result, modelData);
        reloadModel(modelData);
        //recomputeCamera();
        isGLTF = false;
    }
    EMSCRIPTEN_KEEPALIVE
    void load(uint8_t* buf, int bufSize) 
    {
        //printf("[WASM] Loading Texture \n");
#ifdef OPTIMIZE
#else
        std::cout << "Reading decal image!" << std::endl;
        std::cout << "Decal buffer size: " << bufSize << std::endl;
#endif
        decalImageBuffer = loadArray(buf, bufSize);
    }
    EMSCRIPTEN_KEEPALIVE
    void passSize(uint16_t* buf, int bufSize)
    {
#ifdef OPTIMIZE
#else
        std::cout << "Reading decal image size!" << std::endl;
        #endif
        widthDecal  = buf[0];
        heightDecal = buf[1];
        changeDecal = buf[2];
#ifdef OPTIMIZE
#else
        std::cout << "Decal Width: "  << +widthDecal  << std::endl;
        std::cout << "Decal Height: " << +heightDecal << std::endl;
        std::cout << "Clicked :"      << +changeDecal     << std::endl;
#endif
    }
    EMSCRIPTEN_KEEPALIVE
    void loadAlbedo(uint8_t* buf, int bufSize)
    {
#ifdef OPTIMIZE
#else
        std::cout << "Reading albedo from file!" << std::endl;
        std::cout << "Albedo buffer size: " << bufSize << std::endl;
#endif
        geometryPass.createTextureFromFile(&(modelData.material.baseColor), buf, geometryPass.Width, geometryPass.Height, "BaseColor", 0);
    }
    EMSCRIPTEN_KEEPALIVE
    void passSizeAlbedo(uint16_t* buf, int bufSize)
    {
#ifdef OPTIMIZE
#else
        std::cout << "Reading Albedo image size!" << std::endl;
#endif
        int width  = (int)buf[0];
        int height = (int)buf[1];

        geometryPass.use();
        geometryPass.Width  = width;
        geometryPass.Height = height;
        flipAlbedo = (int)buf[2];
        
#ifdef OPTIMIZE
#else
        std::cout << "Albedo size changed regenerating glTexImage2D" << std::endl;
#endif
        glDeleteTextures(1, &(textureSpaceFramebuffer.texture));
        glDeleteTextures(1, &(modelData.material.baseColor));

        glBindFramebuffer(GL_FRAMEBUFFER, textureSpaceFramebuffer.framebuffer);

        glGenTextures(1, &(textureSpaceFramebuffer.texture));
        glBindTexture(GL_TEXTURE_2D, textureSpaceFramebuffer.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, geometryPass.Width, geometryPass.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureSpaceFramebuffer.texture, 0);
        
#ifdef OPTIMIZE
#else
        std::cout << "Albedo Width: "  << +geometryPass.Width  << std::endl;
        std::cout << "Albedo Height: " << +geometryPass.Height << std::endl;
        std::cout << "Changed Albedo: "<< +flipAlbedo        << std::endl;
#endif
    }
    EMSCRIPTEN_KEEPALIVE
    void loadNormal(uint8_t* buf, int bufSize)
    {
#ifdef OPTIMIZE
#else
        std::cout << "Reading normal from file!" << std::endl;
        std::cout << "Normal buffer size: " << bufSize << std::endl;
#endif
        geometryPass.createTextureFromFile(&(modelData.material.normal), buf, geometryPass.Width, geometryPass.Height, "BaseColor", 0);
    }
    EMSCRIPTEN_KEEPALIVE
    void passSizeNormal(uint16_t* buf, int bufSize)
    {
#ifdef OPTIMIZE
#else
        std::cout << "Reading Normal image size!" << std::endl;
#endif
        int width  = (int)buf[0];
        int height = (int)buf[1];

        geometryPass.use();

#ifdef OPTIMIZE
#else
        std::cout << "Normal size changed regenerating glTexImage2D" << std::endl;
#endif
        glDeleteTextures(1, &(textureSpaceFramebuffer.texture));
        glDeleteTextures(1, &(modelData.material.normal));

        glBindFramebuffer(GL_FRAMEBUFFER, textureSpaceFramebuffer.framebuffer);

        glGenTextures(1, &(textureSpaceFramebuffer.texture));
        glBindTexture(GL_TEXTURE_2D, textureSpaceFramebuffer.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, geometryPass.Width, geometryPass.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureSpaceFramebuffer.texture, 0);

        geometryPass.Width  = width;
        geometryPass.Height = height;
#ifdef OPTIMIZE
#else
        std::cout << "Normal Width: "  << +geometryPass.Width  << std::endl;
        std::cout << "Normal Height: " << +geometryPass.Height << std::endl;
#endif
    }
    EMSCRIPTEN_KEEPALIVE
    uint8_t* downloadDecal(uint8_t *buf, int bufSize) 
    {
        if (decalResult.size() > 0)
        {
#ifdef OPTIMIZE
#else
            std::cout << "Successful loading the image into data!" << std::endl;
#endif
            uint8_t* result = &decalResult[0];
            return result;
        }
        else
        {
#ifdef OPTIMIZE
#else
            std::cout << "Unsuccesful loading the image into data!" << std::endl;
#endif
            /*int size = 4 * geometryPass.Width * geometryPass.Height;
            uint8_t values[size];
    
            for (int i = 0; i < size; i++) 
            {
                values[i] = 0u;
            }
        
            auto arrayPtr = &values[0];
            return arrayPtr;*/
            return buf;
        }
    }
    EMSCRIPTEN_KEEPALIVE
    void flipDecalTrigger()
    {
#ifdef OPTIMIZE
#else
            std::cout << "Flip Decal Trigger!\n" << "downloadImage:" << std::to_string(downloadImage) << std::endl;
#endif
        downloadImage = 1u;
#ifdef OPTIMIZE
#else
            std::cout << "downloadImage:" << std::to_string(downloadImage) << std::endl;
#endif
    }
}

bool useWindow = false;
float camDistance = 1.f;
static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);

// an example of something we will control from the javascript side

void main_loop();

// Projector
glm::vec3 projectorPos;
glm::vec3 projectorDir;
// glm::mat4 projectorView;
// glm::mat4 projectorProjection;
float     projectorSize     = 0.5f;
float     projectorRotation = 0.0f;

int lastUsing = 0;

const glm::vec4 kFrustumCorners[] = {
    glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),  // Far-Bottom-Left
    glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),   // Far-Top-Left
    glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),    // Far-Top-Right
    glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),   // Far-Bottom-Right
    glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f), // Near-Bottom-Left
    glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),  // Near-Top-Left
    glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),   // Near-Top-Right
    glm::vec4(1.0f, -1.0f, -1.0f, 1.0f)   // Near-Bottom-Right
};

std::array<glm::vec3, 8> cubeCorners(const glm::vec3& min, const glm::vec3& max) 
{
    std::array<glm::vec3, 8> kCubeCorners = {
        glm::vec3(min.x, min.y, max.z),  // Far-Bottom-Left   
        glm::vec3(min.x, max.y, max.z),  // Far-Top-Left      
        max,                             // Far-Top-Right     
        glm::vec3(max.x, min.y, max.z),  // Far-Bottom-Right  
        min,                             // Near-Bottom-Left  
        glm::vec3(min.x, max.y, min.z),  // Near-Top-Left     
        glm::vec3(max.x, max.y, min.z),  // Near-Top-Right    
        glm::vec3(max.x, min.y, min.z)   // Near-Bottom-Right 
    };
    return kCubeCorners;
}

/*void ClearModelDataBVO(ModelData& modelData)
{
    modelData.bvo.accel          = nanort::BVHAccel<<#typename T#>>
    modelData.bvo.decalProjector = glm::mat4(1.0f);
    modelData.bvo.hitNor         = glm::vec3(1.0f);
    modelData.bvo.hitPos         = glm::vec3(0.0f);
    modelData.bvo.isTracing      = false;
}*/

void rayTrace(const int& mousePositionX, const int& mousePositionY, const glm::vec2& widthHeight, 
              /* In and Out */ ModelData& modelData,
              const glm::mat4& projection, const glm::mat4& view, const bool& debug)
{
    if (debug)
    {
        std::cout << "Right button pressed!" << std::endl;
        std::cout << "mousePositionX: " << mousePositionX << " \n" <<
            "mousePositionY: " << mousePositionY << std::endl;
    }
    
    // https://stackoverflow.com/questions/53467077/opengl-ray-tracing-using-inverse-transformations
    glm::vec2 normalizedMouse = glm::vec2(2.0f, 2.0f) * glm::vec2(mousePositionX, mousePositionY) / widthHeight;
    float x_ndc = normalizedMouse.x - 1.0;
    float y_ndc = 1.0 - normalizedMouse.y; // flipped

    glm::vec4 p_near_ndc = glm::vec4(x_ndc, y_ndc, -1.0f, 1.0f); // z near = -1
    glm::vec4 p_far_ndc  = glm::vec4(x_ndc, y_ndc,  1.0f, 1.0f); // z far = 1

    glm::mat4 invMVP = glm::inverse(projection * view * modelData.modelMatrix);

    glm::vec4 p_near_h = invMVP * p_near_ndc;
    glm::vec4 p_far_h  = invMVP * p_far_ndc;

    glm::vec3 p0 = glm::vec3(p_near_h) / glm::vec3(p_near_h.w, p_near_h.w, p_near_h.w);
    glm::vec3 p1 = glm::vec3(p_far_h)  / glm::vec3(p_far_h.w, p_far_h.w, p_far_h.w);

    glm::vec3 rayOri = p0;
    glm::vec3 rayDir = glm::normalize(p1 - p0);

    nanort::Ray<float> ray;
    ray.org[0] = rayOri.x;
    ray.org[1] = rayOri.y;
    ray.org[2] = rayOri.z;

    ray.dir[0] = rayDir.x;
    ray.dir[1] = rayDir.y;
    ray.dir[2] = rayDir.z;

    float kFar = 1.0e+30f;
    ray.min_t = 0.0f;
    ray.max_t = kFar;

    nanort::TriangleIntersector<> triangle_intersector(glm::value_ptr(modelData.vertices[0]), &(modelData.indexes[0]), sizeof(float) * 3);
    nanort::TriangleIntersection<> isect;
    bool hit = modelData.bvo.accel.Traverse(ray, triangle_intersector, &isect);
    if (hit)
    {
        modelData.bvo.hitPos = rayOri + rayDir * isect.t;
        
        unsigned int fid = isect.prim_id;
        unsigned int id  = modelData.indexes[3 * fid];
        modelData.bvo.hitNor = modelData.normals[id];

        projectorPos = modelData.bvo.hitPos + modelData.bvo.hitNor * PROJECTOR_DISTANCE;
        projectorDir = -modelData.bvo.hitNor;

        float ratio = float(heightDecal) / float(widthDecal);
        float proportionateHeight = projectorSize * ratio;

        glm::mat4 rotate = glm::mat4(1.0f);
        rotate = glm::rotate(rotate, glm::radians(projectorRotation), projectorDir);

        glm::vec4 axis                = rotate * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        glm::mat4 projectorView       = glm::lookAt(projectorPos, modelData.bvo.hitPos, axis.xyz());
        glm::mat4 projectorProjection = glm::ortho(-projectorSize, projectorSize, -proportionateHeight, proportionateHeight, 0.001f, FAR_PLANE);

        modelData.bvo.decalProjector = projectorProjection * projectorView;
        if (debug)
        {
            printf("Hit Point: %s\n", glm::to_string(modelData.bvo.hitPos).c_str());
            printf("Hit Normal: %s\n", glm::to_string(modelData.bvo.hitNor).c_str());
            printf("Decal Projector: %s\n", glm::to_string(modelData.bvo.decalProjector).c_str());
        }
    }
}

// Returns near .x far .y
glm::vec3 calculateNearFarPlane()
{
    // https://community.khronos.org/t/automatically-center-3d-object/20892/6
    //glm::vec3 nearFarPlaneBSphere = bboxMax - centroid;
    glm::vec3 nearFarPlaneBSphere = glm::vec3(modelData.Bbox.centroid.x, modelData.Bbox.centroid.y, modelData.Bbox.bboxMin.z) -
                                    glm::vec3(modelData.Bbox.centroid.x, modelData.Bbox.centroid.y, modelData.Bbox.bboxMax.z);
    float r = glm::length(nearFarPlaneBSphere);
    //float r = glm::dot(nearFarPlaneBSphere, nearFarPlaneBSphere);

    float fDistance = r / 0.57735f; // where 0.57735f is tan(30 degrees)
    // The near and far clipping planes then lay either side of this point, allwoing for the radius of the sphere. So -
    // dNear = fDistance - r;
    // dFar  = fDistance + r;
    glm::vec2 result = glm::vec2(fDistance, fDistance) + glm::vec2(-r, r);
    /*near          = result.x;
    far           = result.y;
    focalDistance = fDistance;
    radius        = r;*/
    return nearFarPlaneBSphere;
}

void renderLineStrip(glm::vec3* vertices, const int& count)
{
    unsigned int lineStripVAO = 0,
                 lineStripVBO;
    glGenVertexArrays(1, &lineStripVAO);
    glGenBuffers(1, &lineStripVBO);
    glBindVertexArray(lineStripVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineStripVBO);
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(float) * count, vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glDrawArrays(GL_LINE_STRIP, 0, count);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void renderLine(glm::vec3& A, glm::vec3& B)
{
    glm::vec3 vertices[] = {A, B};
    unsigned int lineVAO = 0,
                 lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

void renderLineCube(const glm::vec3& min, const glm::vec3& max)
{
    std::array<glm::vec3, 8> corners = cubeCorners(min, max);
    
    glm::vec3 _far[5] = { corners[0], corners[1], corners[2], corners[3], corners[0] };
    renderLineStrip(&_far[0], 5);

    glm::vec3 _near[5] = { corners[4], corners[5], corners[6], corners[7], corners[4] };
    renderLineStrip(&_near[0], 5);

    renderLine(corners[0], corners[4]);
    renderLine(corners[1], corners[5]);
    renderLine(corners[2], corners[6]);
    renderLine(corners[3], corners[7]);
}

void renderFrustum(const glm::mat4& view_proj)
{
    glm::mat4 inverse = glm::inverse(view_proj);
    glm::vec3 corners[8];

    for (int i = 0; i < 8; i++)
    {
        glm::vec4 v = inverse * kFrustumCorners[i];
        v           = v / v.w;
        corners[i]  = glm::vec3(v.x, v.y, v.z);
    }

    glm::vec3 _far[5] = { corners[0], corners[1], corners[2], corners[3], corners[0] };

    renderLineStrip(&_far[0], 5);

    glm::vec3 _near[5] = { corners[4], corners[5], corners[6], corners[7], corners[4] };

    renderLineStrip(&_near[0], 5);

    renderLine(corners[0], corners[4]);
    renderLine(corners[1], corners[5]);
    renderLine(corners[2], corners[6]);
    renderLine(corners[3], corners[7]);
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void EditTransform(float* cameraView, float* cameraProjection, float* matrix, bool editTransformDecomposition, bool splitScreen, ImGuiIO& io)
{
    ImGuizmo::SetRect(0, 0, (splitScreen ? io.DisplaySize.x / 2 : io.DisplaySize.x), io.DisplaySize.y);
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
      ImGui::SetNextWindowPos(ImVec2(400,20), ImGuiCond_Appearing);
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
      ImGuizmo::SetRect(0, 0, (splitScreen ? io.DisplaySize.x/2 : io.DisplaySize.x), io.DisplaySize.y);
   }
   ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, useSnap ? &snap[0] : NULL, boundSizing ? bounds : NULL, boundSizingSnap ? boundsSnap : NULL);
}

// http://marcelbraghetto.github.io/a-simple-triangle/2019/04/14/part-09/
std::string LoadTextFile(const std::string& path)
{
    SDL_RWops* file{SDL_RWFromFile(path.c_str(), "r")};
    size_t fileLength{static_cast<size_t>(SDL_RWsize(file))};
    void* data{SDL_LoadFile_RW(file, nullptr, 1)};
    std::string result(static_cast<char*>(data), fileLength);
    SDL_free(data);

    return result;
}

std::bitset<256> VertexBitHash(glm::vec3* v, glm::vec3* n, glm::vec2* u) {
    std::bitset<256> bits{};
    bits |= reinterpret_cast<unsigned &>(v->x); bits <<= 32;
    bits |= reinterpret_cast<unsigned &>(v->y); bits <<= 32;
    bits |= reinterpret_cast<unsigned &>(v->z); bits <<= 32;
    bits |= reinterpret_cast<unsigned &>(n->x); bits <<= 32;
    bits |= reinterpret_cast<unsigned &>(n->y); bits <<= 32;
    bits |= reinterpret_cast<unsigned &>(n->z); bits <<= 32;
    bits |= reinterpret_cast<unsigned &>(u->x); bits <<= 32;
    bits |= reinterpret_cast<unsigned &>(u->y);
    return bits;
}

void ComputeTangents(ModelData& modelData)
{
    modelData.tangents.clear();
    modelData.tangents.resize(modelData.indexes.size(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    for(unsigned int i = 0; i < modelData.indexes.size(); i += 3)
    {

        glm::vec3 vertex0 = modelData.vertices[modelData.indexes[i]];
        glm::vec3 vertex1 = modelData.vertices[modelData.indexes[i + 1]];
        glm::vec3 vertex2 = modelData.vertices[modelData.indexes[i + 2]];


        glm::vec3 deltaPos;
        if(vertex0 == vertex1)
        {
            deltaPos = vertex2 - vertex0;
        }
        else
        {
            deltaPos = vertex1 - vertex0;
        }
        glm::vec2 uv0 = modelData.textureCoordinates[modelData.indexes[i]];
        glm::vec2 uv1 = modelData.textureCoordinates[modelData.indexes[i + 1]];
        glm::vec2 uv2 = modelData.textureCoordinates[modelData.indexes[i + 2]];

        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;

        glm::vec3 tan; // tangents

        // avoid divion by 0
        if(deltaUV1.s != 0)
        {
            tan = deltaPos / deltaUV1.s;
        }
        else
        {
            tan = deltaPos / 1.0f;
        }
        glm::vec3 normal = modelData.normals[modelData.indexes[i]];
        
        glm::vec3 vectorOne = vertex1 - vertex0;
        glm::vec3 vectorTwo = vertex2 - vertex0;

        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        glm::vec3 tan2 = glm::vec3((deltaUV1.x * vectorTwo.x - deltaUV2.x * vectorOne.x) * r, 
                                   (deltaUV1.x * vectorTwo.y - deltaUV2.x * vectorOne.y) * r,
                                   (deltaUV1.x * vectorTwo.z - deltaUV2.x * vectorOne.z) * r);
        
        tan = glm::normalize(tan - glm::dot(normal, tan) * normal);
        glm::vec4 tangent = glm::vec4(tan, (glm::dot(glm::cross(normal, tan), tan2) < 0.0f ? -1.0f : 1.0f));

        // write into array - for each vertex of the face the same value
        modelData.tangents[modelData.indexes[i]]     = tangent;
        modelData.tangents[modelData.indexes[i + 1]] = tangent;
        modelData.tangents[modelData.indexes[i + 2]] = tangent;
    }
}

void ClearModelVertexData(ModelData& modelData)
{
    modelData.vertices.clear();
    modelData.normals.clear();
    modelData.textureCoordinates.clear();
    modelData.tangents.clear();
    modelData.indexes.clear();
    modelData.Bbox.bboxMin  = glm::vec3(1e+5);
    modelData.Bbox.bboxMax  = glm::vec3(-1e+5);
    modelData.Bbox.centroid = glm::vec3(0.0f);
    modelData.modelMatrix   = glm::mat4(1.0f);
    modelData.bvo.isTracing = false;
    modelData.bvo.decalProjector = glm::mat4(1.0f);
}

void ObjLoader(std::string inputFile, ModelData& modelData)
{
    ClearModelVertexData(modelData);

    std::istringstream stream = std::istringstream(inputFile);
    //std::cout << stream.str() << std::endl;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warning;
    std::string error;

    if (!tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &error,
            &warning,
            &stream,
            NULL,
            true))
    {
        #ifdef EXCEPTIONS
        throw std::runtime_error("loadOBJFile: Error: " + warning + error);
        #else
        std::cout << "loadOBJFile: Error: " << warning << error << std::endl;
        #endif
    }

    std::unordered_map<std::bitset<256>, uint32_t> uniqueVertices;

    uint32_t counter = 0;

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            glm::vec3 position{
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]};
            glm::vec3 normal{
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]};
            glm::vec2 textureCoordinates{
                attrib.texcoords[2 * abs(index.texcoord_index) + 0],
                attrib.texcoords[2 * abs(index.texcoord_index) + 1]};
            auto hash = VertexBitHash(&position, &normal, &textureCoordinates);
            if (uniqueVertices.count(hash) == 0)
            {
                modelData.vertices.push_back(position);
                modelData.normals.push_back(normal);
                modelData.textureCoordinates.push_back(textureCoordinates);
                // BBox
                modelData.Bbox.bboxMax = glm::max(modelData.Bbox.bboxMax, position);
                modelData.Bbox.bboxMin = glm::min(modelData.Bbox.bboxMin, position);
                modelData.indexes.push_back(counter);
                uniqueVertices[hash] = counter;
                ++counter;
            }
            else
            {
                modelData.indexes.push_back(uniqueVertices[hash]);
            }
        }
    }
    modelData.Bbox.centroid = (modelData.Bbox.bboxMin + modelData.Bbox.bboxMax) * 0.5f;
    ComputeTangents(modelData);

    std::cout << "BboxMax: {x: " << modelData.Bbox.bboxMax.x << ", y: " << modelData.Bbox.bboxMax.y << ", z: " << modelData.Bbox.bboxMax.z << "}\n";
    std::cout << "BboxMin: {x: " << modelData.Bbox.bboxMin.x << ", y: " << modelData.Bbox.bboxMin.y << ", z: " << modelData.Bbox.bboxMin.z << "}\n";

#ifdef OPTIMIZE
#else
    std::cout << "Vertices: " << modelData.vertices.size() << "\n";
    std::cout << "Normals: " << modelData.normals.size() << "\n";
    std::cout << "Tangents: " << modelData.tangents.size() << "\n";
    std::cout << "Indices: " << modelData.indexes.size() << "\n";
    std::cout << "Texture Coordinates: " << modelData.textureCoordinates.size() << "\n";
    std::cout << "Materials: " << materials.size() << "\n";
    std::cout << "BboxMax: {x: " << modelData.Bbox.bboxMax.x << ", y: " << modelData.Bbox.bboxMax.y << ", z: " << modelData.Bbox.bboxMax.z << "}\n";
    std::cout << "BboxMin: {x: " << modelData.Bbox.bboxMin.x << ", y: " << modelData.Bbox.bboxMin.y << ", z: " << modelData.Bbox.bboxMin.z << "}\n";
#endif
}

const float* GetDataFromAccessorGLTF(const tinygltf::Model &model, const tinygltf::Accessor& accessor)
{
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    // cast to float type read only. Use accessor and bufview byte offsets to determine where position data 
    // is located in the buffer.
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    // bufferView byteoffset + accessor byteoffset tells you where the actual position data is within the buffer. From there
    // you should already know how the data needs to be interpreted.
    const float* result = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
    return result;
}

void loadGLTF(tinygltf::Model &model, ModelData& modelData) 
{
    ClearModelVertexData(modelData);

    std::unordered_map<std::bitset<256>, uint32_t> uniqueVertices;
    uint32_t counter = 0;
    int primCounter = 0;

    std::vector<unsigned int> _u32Buffer;
    std::vector<unsigned short> _u16Buffer;
    std::vector<unsigned char> _u8Buffer;

    for (auto &mesh : model.meshes) 
    {
#ifdef OPTIMIZE
#else
        std::cout << "mesh : " << mesh.name << std::endl;
#endif
        for (const auto& prim : mesh.primitives)
        {

            bool result = GLTF::GetAttributes<glm::vec3>(model, prim, modelData.vertices,           "POSITION");
                 result = GLTF::GetAttributes<glm::vec3>(model, prim, modelData.normals,            "NORMAL");
                 result = GLTF::GetAttributes<glm::vec2>(model, prim, modelData.textureCoordinates, "TEXCOORD_0");
                 result = GLTF::GetAttributes<glm::vec4>(model, prim, modelData.tangents,           "TANGENT");

            if (prim.indices > -1)
            {
                const tinygltf::Accessor& indexAccessor = model.accessors[prim.indices];
                const tinygltf::BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                switch (indexAccessor.componentType)
                {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                    _u32Buffer.resize(indexAccessor.count);
                    std::memcpy(&_u32Buffer[0], &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset], indexAccessor.count * sizeof(unsigned int));
                    modelData.indexes.insert(modelData.indexes.end(), std::make_move_iterator(_u32Buffer.begin()), std::make_move_iterator(_u32Buffer.end()));
                    break;
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                    _u16Buffer.resize(indexAccessor.count);
                    std::memcpy(&_u16Buffer[0], &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset], indexAccessor.count * sizeof(unsigned short));
                    modelData.indexes.insert(modelData.indexes.end(), std::make_move_iterator(_u16Buffer.begin()), std::make_move_iterator(_u16Buffer.end()));
                    break;
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                    _u8Buffer.resize(indexAccessor.count);
                    std::memcpy(&_u8Buffer[0], &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset], indexAccessor.count * sizeof(unsigned char));
                    modelData.indexes.insert(modelData.indexes.end(), std::make_move_iterator(_u8Buffer.begin()), std::make_move_iterator(_u8Buffer.end()));
                    break;
                default:
                    std::cerr << "Unknown index component type : " << indexAccessor.componentType << " is not supported" << std::endl;
                    return;
                }
            }
            else
            {
                //! Primitive without indices, creating them
                const auto& accessor = model.accessors[prim.attributes.find("POSITION")->second];
                for (unsigned int i = 0; i < accessor.count; ++i)
                    modelData.indexes.push_back(i);
            }

        }
    }

    if (modelData.tangents.size() == 0)
    {
        std::cout << "No tangents, computing them!" << std::endl;
        ComputeTangents(modelData);
    }

#ifdef OPTIMIZE
#else
    std::cout << "Vertices: " << modelData.vertices.size() << "\n";
    std::cout << "Normals: " << modelData.normals.size() << "\n";
    std::cout << "Texture Coordinates: " << modelData.textureCoordinates.size() << "\n";
    std::cout << "Tangents: " << modelData.tangents.size() << "\n";
    std::cout << "Indices: " << modelData.indexes.size() << "\n";
    //std::cout << "Materials: " << materials.size() << "\n";
    std::cout << "BboxMax: {x: " << modelData.Bbox.bboxMax.x << ", y: " << modelData.Bbox.bboxMax.y << ", z: " << modelData.Bbox.bboxMax.z << "}\n";
    std::cout << "BboxMin: {x: " << modelData.Bbox.bboxMin.x << ", y: " << modelData.Bbox.bboxMin.y << ", z: " << modelData.Bbox.bboxMin.z << "}\n";
#endif
    for (int i = 0; i < modelData.vertices.size(); ++i)
    {
        glm::vec3 vertex = modelData.vertices[i];
        modelData.Bbox.bboxMin = glm::min(vertex, modelData.Bbox.bboxMin);
        modelData.Bbox.bboxMax = glm::max(vertex, modelData.Bbox.bboxMax);
    }
    modelData.Bbox.centroid = (modelData.Bbox.bboxMin + modelData.Bbox.bboxMax) * 0.5f;
    std::cout << "BboxMax: {x: " << modelData.Bbox.bboxMax.x << ", y: " << modelData.Bbox.bboxMax.y << ", z: " << modelData.Bbox.bboxMax.z << "}\n";
    std::cout << "BboxMin: {x: " << modelData.Bbox.bboxMin.x << ", y: " << modelData.Bbox.bboxMin.y << ", z: " << modelData.Bbox.bboxMin.z << "}\n";

    _u8Buffer.clear();
    _u16Buffer.clear();
    _u32Buffer.clear();

    flipAlbedo = 1;
}

void CreateBOs(ModelData& modelData)
{
    // Create a Vertex Buffer Object and copy the vertex data to it
    //unsigned int VBOVertices, VBONormals, VBOTextureCoordinates, VAO, EBO;
    glGenVertexArrays(1, &(modelData.openGLObject.VAO));
    glGenBuffers(1, &(modelData.openGLObject.VBOVertices));
	glBindBuffer(GL_ARRAY_BUFFER, modelData.openGLObject.VBOVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * modelData.vertices.size(), modelData.vertices.data(), GL_STATIC_DRAW);
    //modelDataVertices.clear();

	glGenBuffers(1, &(modelData.openGLObject.VBONormals));
	glBindBuffer(GL_ARRAY_BUFFER, modelData.openGLObject.VBONormals);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * modelData.normals.size(), modelData.normals.data(), GL_STATIC_DRAW);
    //modelDataNormals.clear();

	glGenBuffers(1, &(modelData.openGLObject.VBOTextureCoordinates));
	glBindBuffer(GL_ARRAY_BUFFER, modelData.openGLObject.VBOTextureCoordinates);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * modelData.textureCoordinates.size(), modelData.textureCoordinates.data(), GL_STATIC_DRAW);
    //modelDataTextureCoordinates.clear();

    glGenBuffers(1, &(modelData.openGLObject.VBOTangents));
    glBindBuffer(GL_ARRAY_BUFFER, modelData.openGLObject.VBOTangents);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * modelData.tangents.size(), modelData.tangents.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &(modelData.openGLObject.EBO));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelData.openGLObject.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * modelData.indexes.size(), modelData.indexes.data(), GL_STATIC_DRAW);
    //indexes.clear();
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(modelData.openGLObject.VAO);

    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, modelData.openGLObject.VBOVertices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelData.openGLObject.EBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, modelData.openGLObject.VBONormals);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, modelData.openGLObject.VBOTextureCoordinates);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, modelData.openGLObject.VBOTangents);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
}

void BuildBVH(ModelData& modelData)
{
    nanort::BVHBuildOptions<float> build_options;  // Use default option
    build_options.cache_bbox = false;

#ifdef OPTIMIZE
#else
    printf("  BVH build option:\n");
    printf("    # of leaf primitives: %d\n", build_options.min_leaf_primitives);
    printf("    SAH binsize         : %d\n", build_options.bin_size);
#endif
    //std::cout << "Vertices size: " << (sizeof(vertices) / sizeof(vertices[0])) << std::endl;

    nanort::TriangleMesh<float> triangle_mesh(glm::value_ptr(modelData.vertices[0]), &(modelData.indexes[0]), sizeof(float) * 3);
    nanort::TriangleSAHPred<float> triangle_pred(glm::value_ptr(modelData.vertices[0]), &(modelData.indexes[0]), sizeof(float) * 3);

    //printf("num_triangles = %zu\n", modelDataVertices.size() / 3);
    //printf("faces = %p\n", indexes.size());

    modelData.bvo.accel = nanort::BVHAccel<float>();

    //nanort::BVHAccel<float> accelDummy;
    bool ret = modelData.bvo.accel.Build(modelData.indexes.size() / 3, triangle_mesh, triangle_pred, build_options);
    assert(ret);

    nanort::BVHBuildStatistics stats = modelData.bvo.accel.GetStatistics();

#ifdef OPTIMIZE
#else
    printf("  BVH statistics:\n");
    printf("    # of leaf   nodes: %d\n", stats.num_leaf_nodes);
    printf("    # of branch nodes: %d\n", stats.num_branch_nodes);
    printf("  Max tree depth     : %d\n", stats.max_tree_depth);
#endif
    float bmin[3], bmax[3];
    modelData.bvo.accel.BoundingBox(bmin, bmax);
    // bboxMax = glm::vec3(bmax[0], bmax[1], bmax[2]);
    // bboxMin = glm::vec3(bmin[0], bmin[1], bmax[2]);
#ifdef OPTIMIZE
#else
    printf("  Bmin               : %f, %f, %f\n", bmin[0], bmin[1], bmin[2]);
    printf("  Bmax               : %f, %f, %f\n", bmax[0], bmax[1], bmax[2]);
#endif
    //modelData.bvo.accel = accelDummy;
}

void recomputeCamera()
{
#ifdef OPTIMIZE
#else
    std::cout << "Recomputing camera!" << std::endl;
#endif
    glm::vec3 diff = calculateNearFarPlane();// / radius;
    //camPos   = centroid + diff * glm::vec3(0.0f, 0.0f, (far - near));
    //camFront = glm::normalize(diff);//centroid - glm::vec3(0.0f, 0.0f, 1.0f));//camPos);
    
    //glm::vec3 bboxMinCentroid = glm::vec3(bboxMin.x, bboxMin.y, centroid.z);
    glm::vec3 centroidBboxMin = glm::vec3(modelData.Bbox.centroid.x, modelData.Bbox.centroid.y, modelData.Bbox.bboxMin.z);
    // TOA = Adjacent = Opposite / Tan(30)
    // Opposite.
    float bboxMinCentroidMinusCentroidBboxMin = modelData.Bbox.centroid.z - modelData.Bbox.bboxMin.z;
    float adjacentEqualsOppositeOverTanThirty = bboxMinCentroidMinusCentroidBboxMin / tan(glm::radians(15.0f));
    adjacentEqualsOppositeOverTanThirty *= adjacentEqualsOppositeOverTanThirty;
    /*float c = bboxMinCentroidMinusCentroidBboxMin / sin(glm::radians(15.0f));
    float a = sqrt(c*c - bboxMinCentroidMinusCentroidBboxMin*bboxMinCentroidMinusCentroidBboxMin);*/

    glm::vec3 diffNormalize = glm::normalize(diff);// */ diff * adjacentEqualsOppositeOverTanThirty;

#ifdef OPTIMIZE
#else
    std::cout << "adjacentEqualsOppositeOverTanThirty: " << adjacentEqualsOppositeOverTanThirty << 
                 " DifNormalize: x:" << diffNormalize.x << " y: " << diffNormalize.y 
                 << " z: " << diffNormalize.z << std::endl;
#endif

    camPos   = centroidBboxMin - (adjacentEqualsOppositeOverTanThirty * diffNormalize);// * radius * 8.0f;
    camFront = diffNormalize;
	camUp    = glm::vec3(0.0f, 1.0f, 0.0f);

#ifdef OPTIMIZE
#else
    std::cout << "Dif: " << glm::to_string(diff) << "\n";
    std::cout << "camPos: " << glm::to_string(camPos) << "\n";
    std::cout << "camFront: " << glm::to_string(camFront) << "\n";
    std::cout << "Radius: " << radius << "\n";
    std::cout << "Centroid: " << glm::to_string(modelData.Bbox.centroid) << "\n";
    std::cout << "Focal Distance: " << focalDistance << "\n";
    std::cout << "Near: " << nearPlane << "\n";
    std::cout << "Far: " << farPlane << "\n";
    #endif
}

void reloadModel(ModelData& modelData)
{
    BuildBVH(modelData);
    CreateBOs(modelData);
    std::cout << "Recomputed BBox Center: {x: " << modelData.Bbox.centroid.x << ", y: " << modelData.Bbox.centroid.y << ", z:" << modelData.Bbox.centroid.z << "}\n";
}

void recomputeDecalBaseColorTexture()
{
    //decalsPass.createTexture(&decalTexture, "Assets/Textures/WatchMen.jpeg", "iChannel0", 1);
    //decalsPass.createTextureFromFile(&decalTexture, decalImageBuffer, widthDecal, heightDecal, "iChannel0", 1);
#ifdef OPTIMIZE
#else
    std::cout << "Changing texture" << std::endl;
#endif
    flip = 1;
    glGenTextures(1, &(modelData.material.decalBaseColor));
    glBindTexture(GL_TEXTURE_2D, modelData.material.decalBaseColor);

    // In an ideal world this should be exposed as input params to the function.
    // Texture wrapping params.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Texture filtering params.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Get the texture format automatically.
    auto format = GL_RGBA;    
    glTexImage2D(GL_TEXTURE_2D, 0, format, widthDecal, heightDecal, 0, format, GL_UNSIGNED_BYTE, decalImageBuffer.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    // Clear the data.
    decalImageBuffer.clear();

    // Bind the uniform sampler.
    decalsPass.use();
    decalsPass.setInt("iChannel0", 1);
}

float max(const glm::vec3& x)
{
    return std::max(std::max(x.x, x.y), x.z);
}

bool init()
{
#if defined(_WIN32) || defined(__EMSCRIPTEN__)
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
#endif
#ifdef __APPLE__
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    
    // Create SDL window
    window = SDL_CreateWindow("Decal Editor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if (!window) 
    {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create OpenGL context
    context = SDL_GL_CreateContext(window);
    if (!context) 
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
    std::cout << "GL Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // Enable V-Sync
    /*if (SDL_GL_SetSwapInterval(1) == -1)
    {
        std::cout << "VSync not supported on this platform!" << std::endl;
    }*/

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
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return io;
}

void initializeShaderSource()
{
    // Set attribute locations. We need this given that we want to be
    // able to support Mac and on Mac, there's no glsl version that
    // supports layout qualifiers.
    std::string vertexPositionAttributeQualifierName = "VertexPosition",
                normalPositionAttributeQualifierName = "VertexNormals",
                texCoordsPositionAttributeQualiferName = "VertexTextureCoords",
                vertexTangentPositionAttributeQualifierName = "VertexTangents";
    std::vector<std::string> depthPrePassAttributeLocations(1);
    depthPrePassAttributeLocations[0] = vertexPositionAttributeQualifierName;
    
    std::vector<std::string> geometryPassAttributeLocations(4);
    geometryPassAttributeLocations[0] = vertexPositionAttributeQualifierName;
    geometryPassAttributeLocations[1] = normalPositionAttributeQualifierName;
    geometryPassAttributeLocations[2] = texCoordsPositionAttributeQualiferName;
    geometryPassAttributeLocations[3] = vertexTangentPositionAttributeQualifierName;
    
    std::vector<std::string> deferredPassAttributeLocations(4);
    deferredPassAttributeLocations[0] = vertexPositionAttributeQualifierName;
    deferredPassAttributeLocations[1] = normalPositionAttributeQualifierName;
    deferredPassAttributeLocations[2] = texCoordsPositionAttributeQualiferName;
    deferredPassAttributeLocations[3] = vertexTangentPositionAttributeQualifierName;
    
    std::vector<std::string> hitPositionAttributeLocations(1);
    deferredPassAttributeLocations[0] = vertexPositionAttributeQualifierName;
    //deferredPassAttributeLocations[1] = texCoordsPositionAttributeQualiferName;
    
    std::vector<std::string> decalsPassAttributeLocations(3);
    decalsPassAttributeLocations[0] = vertexPositionAttributeQualifierName;
    decalsPassAttributeLocations[1] = normalPositionAttributeQualifierName;
    decalsPassAttributeLocations[2] = texCoordsPositionAttributeQualiferName;
#ifdef __EMSCRIPTEN__
    depthPrePass  = Shader("shaders/DBuffer.vert",      "shaders/DBuffer.frag",      depthPrePassAttributeLocations, GLSLVersion);
    geometryPass  = Shader("shaders/GBuffer.vert",      "shaders/GBuffer.frag",      geometryPassAttributeLocations, GLSLVersion);
    deferredPass  = Shader("shaders/DeferredPass.vert", "shaders/DeferredPass.frag", deferredPassAttributeLocations, GLSLVersion);
    hitPosition   = Shader("shaders/HitPosition.vert",  "shaders/HitPosition.frag",  hitPositionAttributeLocations,  GLSLVersion);
    decalsPass    = Shader("shaders/Decals.vert",       "shaders/Decals.frag",       decalsPassAttributeLocations,   GLSLVersion);
#else
    depthPrePass  = Shader("../shaders/DBuffer.vert",      "../shaders/DBuffer.frag",      depthPrePassAttributeLocations, GLSLVersion);
    geometryPass  = Shader("../shaders/GBuffer.vert",      "../shaders/GBuffer.frag",      geometryPassAttributeLocations, GLSLVersion);
    deferredPass  = Shader("../shaders/DeferredPass.vert", "../shaders/DeferredPass.frag", deferredPassAttributeLocations, GLSLVersion);
    hitPosition   = Shader("../shaders/HitPosition.vert",  "../shaders/HitPosition.frag",  hitPositionAttributeLocations,  GLSLVersion);
    decalsPass    = Shader("../shaders/Decals.vert",       "../shaders/Decals.frag",       decalsPassAttributeLocations,   GLSLVersion);
#endif
}

frameBuffer createAndAttachDepthPrePassRbo()
{
    /** Start Depth Buffer **/
    glGenFramebuffers(1, &(depthFramebuffer.framebuffer));
    glBindFramebuffer(GL_FRAMEBUFFER, depthFramebuffer.framebuffer);

    depthPrePass.use();

    // create and attach depth buffer (renderbuffer)
    glGenTextures(1, &(depthFramebuffer.texture));
    glBindTexture(GL_TEXTURE_2D, depthFramebuffer.texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, WIDTH/4, HEIGHT/4, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthFramebuffer.texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
    {
        std::cerr << "Framebuffer configuration failed" << std::endl;
    }

    unsigned int attachments[1] = {GL_NONE};
    glDrawBuffers(1, attachments);
    std::cout << "Depth frame buffer: framebuffer: " << depthFramebuffer.framebuffer << " texture: " << depthFramebuffer.texture << std::endl;
    /** End Depth Buffer **/
    return depthFramebuffer;
}

frameBuffer createAndAttachTextureSpaceRbo()
{
    /** Start Texture Space Buffer **/
    glGenFramebuffers(1, &(textureSpaceFramebuffer.framebuffer));
    glBindFramebuffer(GL_FRAMEBUFFER, textureSpaceFramebuffer.framebuffer);

    geometryPass.use();
#ifdef OPTIMIZE
#else
    std::cout << "Albedo Width: "  << geometryPass.Width  << std::endl;
    std::cout << "Albedo Height: " << geometryPass.Height << std::endl;
#endif

    glGenTextures(1, &(textureSpaceFramebuffer.texture));
    glBindTexture(GL_TEXTURE_2D, textureSpaceFramebuffer.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, geometryPass.Width, geometryPass.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureSpaceFramebuffer.texture, 0);

    // Set the list of draw buffers.
    unsigned int drawBuffersFBO[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBuffersFBO); // "1" is the size of DrawBuffers

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    deferredPass.use();
    deferredPass.setInt("gNormal",    0);
    deferredPass.setInt("gAlbedo",    1);
    // deferredPass.setInt("gMetallic",  2);
    // deferredPass.setInt("gRoughness", 3);
    // deferredPass.setInt("gAO",        4);
    /** End Texture Space Buffer **/
    std::cout << "Texture space frame buffer: framebuffer: " << textureSpaceFramebuffer.framebuffer << " texture: " << textureSpaceFramebuffer.texture << std::endl;
    return textureSpaceFramebuffer;
}

bool fileExists(const std::string& file)
{
    return std::filesystem::exists(file);
}

// https://stackoverflow.com/questions/5607589/right-way-to-split-an-stdstring-into-a-vectorstring
std::vector<std::string> split(std::string text, char delim) 
{
    std::string line;
    std::vector<std::string> vec;
    std::stringstream ss(text);
    while (std::getline(ss, line, delim)) {
        vec.push_back(line);
    }
    return vec;
}

int main()
{
#ifdef __EMSCRIPTEN__
    fileNames[0].mesh = "Assets/Pilot/source/PilotShirtDraco.glb";
    fileNames[0].baseColor = "../Assets/Pilot/textures/T_DefaultMaterial_B_1k.jpg";
    fileNames[0].normal = "../Assets/Pilot/textures/T_DefaultMaterial_N_1k.jpg";

    fileNames[1].mesh = "Assets/CaterpillarWorkboot/source/sh_catWorkBoot_draco.glb";
    fileNames[1].baseColor = "../Assets/CaterpillarWorkboot/textures/sh_catWorkBoot_albedo.jpeg";
    fileNames[1].normal = "../Assets/CaterpillarWorkboot/textures/sh_catWorkBoot_nrm.jpeg";

    fileNames[2].mesh = "../Assets/RV/source/RV.glb";
    fileNames[2].baseColor = "../Assets/RV/textures/clay_baseColor.png";
    fileNames[2].normal = "../Assets/Clay/textures/map_normals.jpg";
#else
    fileNames[0].mesh = "../Assets/Pilot/source/PilotShirtDraco.glb";
    fileNames[0].baseColor = "../Assets/Pilot/textures/T_DefaultMaterial_B_1k.jpg";
    fileNames[0].normal = "../Assets/Pilot/textures/T_DefaultMaterial_N_1k.jpg";

    fileNames[1].mesh = "../Assets/CaterpillarWorkboot/source/sh_catWorkBoot_draco.glb";
    fileNames[1].baseColor = "../Assets/CaterpillarWorkboot/textures/sh_catWorkBoot_albedo.jpeg";
    fileNames[1].normal = "../Assets/CaterpillarWorkboot/textures/sh_catWorkBoot_nrm.jpeg";
    /*fileNames[2].mesh = "../Assets/RV/source/RV.glb";
    fileNames[2].baseColor = "../Assets/RV/textures/clay_baseColor.png";
    fileNames[2].normal = "../Assets/Clay/textures/map_normals.jpg";*/
    #endif
#ifdef OPTIMIZE
    std::cout << "Optimize" << std::endl;
#else
    std::cout << "Don't Optimize" << std::endl;
#endif
    /**
     * Start Window
     */

    if (!init())
    {
        return -1;
    }
    /**
     * End Window
     */
    
    /**
     * Start ImGui
     */
    ImGuiIO io = initImgui();
    /**
     * End ImGui
     */

    /**
     * Start Shader Setup
     */
    initializeShaderSource();
    
    //ObjLoader("Assets/t-shirt-lp/source/Shirt.obj");

    const int pick = 1;

    /**
     * Start Read Models
     */
    for (uint8_t i = 0u; i < fileNames.size(); ++i)
    {
        //int i = pick;
        ClearModelVertexData(modelsData[i]);
        if (fileExists(fileNames[i].mesh))
        {
            std::cout << "File: " << fileNames[i].mesh << " exists" << std::endl;
        }
        else
        {
            std::cout << "File: " << fileNames[i].mesh << " does not exist" << std::endl;
        }

        std::vector<std::string> splittedMeshName = split(fileNames[i].mesh, '.');
        std::cout << "splittedMeshName size: " << splittedMeshName.size() << std::endl;
        std::string fileType = splittedMeshName[splittedMeshName.size() - 1];
        std::cout << "File type: " << fileType << std::endl;
        if (fileType == "glb" || fileType == "gltf")
        {
            tinygltf::Model modelGLTF;
            if (!GLTF::loadModel(modelGLTF, fileNames[i].mesh))
            {
#ifdef EXCEPTIONS
                throw std::runtime_error("load GLTF Error!");
#else
                std::cout << "Load GLTF Error!" << std::endl;
#endif
            }
            loadGLTF(modelGLTF, modelsData[i]);
        }
        else if (fileType == "obj")
        {
            std::string fileString = fileToString(fileNames[i].mesh);
            ObjLoader(fileString, modelsData[i]);
        }
        /**
         * End Read Models
         */

        // Build the acceleration structure for ray tracing.
        BuildBVH(modelsData[i]);
        //reloadModel(modelsData[i]);

        //centroid = (bboxMin + bboxMax) * 0.5f;
        std::cout << "2nd BBox Center: {x: " << modelsData[i].Bbox.centroid.x << ", y: " << modelsData[i].Bbox.centroid.y << ", z:" << modelsData[i].Bbox.centroid.z << "}\n";
        //camPos = centroid + glm::vec3(0.0f, 0.0f, 5.0f);//glm::vec3( 0.0f, -0.7f, 5.0f ); 
    }
    ClearModelVertexData(modelData);
    modelData = modelsData[pick];
    //BuildBVH(modelData);
    
    geometryPass.use();

#ifdef __EMSCRIPTEN__
    geometryPass.createTexture(&(modelData.material.baseColor), fileNames[pick].baseColor, "BaseColor", 0);
    geometryPass.createTexture(&(modelData.material.normal),    fileNames[pick].normal,    "Normal",    1);
#else
    geometryPass.createTexture(&(modelData.material.baseColor), fileNames[pick].baseColor, "BaseColor", 0);
    geometryPass.createTexture(&(modelData.material.normal),    fileNames[pick].normal,    "Normal",    1);
#endif

    /** Start Create Decals Texture **/
#ifdef __EMSCRIPTEN__
    decalsPass.createTexture(&(modelData.material.decalBaseColor), "Assets/Textures/Watchmen.png"/*Batman.jpg"*/, "iChannel0", 1);
#else
    decalsPass.createTexture(&(modelData.material.decalBaseColor), "../Assets/Textures/Watchmen.png"/*Batman.jpg"*/, "iChannel0", 1);
#endif
    decalsPass.setInt("iChannel1", 0);
    decalsPass.setInt("iDepth", 2);
    /** End Create Decals Texture **/

    /** Start Depth Buffer **/
    depthFramebuffer = createAndAttachDepthPrePassRbo();
    /** End Depth Buffer **/

    /** Start Texture Space Buffer **/
    createAndAttachTextureSpaceRbo();
    /** End Texture Space Buffer **/

    /**
     * End Shader Setup
     */

    /**
     * Start Geometry Definition
     */

    // Create a Vertex Buffer Object and copy the vertex data to it
    CreateBOs(modelData);

    glEnable(GL_DEPTH_TEST); 
    /**
     * End Geometry Definition
     */
    
	// Create the camera (eye).
	view = glm::mat4(1.0f);
	modelData.modelMatrix = glm::mat4(1.0f);

    iTime = 0.0f;

    //unsigned int decalTexture = -1;
    // decalsPass.createTexture(&decalTexture, "Assets/Textures/Batman.jpg", "iChannel0", 1);
    // decalsPass.setInt("iChannel1", 0);
    // decalsPass.setInt("iDepth", 2);

    int click = 0;

    modelData.bvo.hitPos = glm::vec3(0.0, 0.0, 0.0);
    modelData.bvo.hitNor = glm::vec3(1.0, 1.0, 1.0);

    modelNoGuizmo = modelData.modelMatrix;

    mousePositionX = WIDTH / 2;
    mousePositionY = HEIGHT / 2;

    widthHeight = glm::vec2(WIDTH, HEIGHT);

    //recomputeCamera();

    projection = glm::perspective(RADIANS_30, widthHeight.x / widthHeight.y, nearPlane, farPlane);
    if (CAMERA == FPS)
    {
        recomputeCamera();
        view = glm::lookAt(camPos, camPos + camFront, camUp);
        viewPinned = view;
    }
    else if (CAMERA == TRACK_BALL)
    {
        updateViewMatrix();
        recomputeCamera();
        viewPinned = glm::lookAt(camPos, camPos + camFront, camUp);
    }
    
    rayTrace(mousePositionX, mousePositionY, widthHeight, 
             /** In and Out **/ modelData,
             projection, view, false);

    std::cout << "Dec Width: " << +widthDecal << " Dec Height: "
              << +heightDecal << std::endl;

    #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, true);
    #else 
    while (main_loop_running)
    {
        main_loop();
    }
    #endif

    // TODO: add proper cleanup.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    ClearModelVertexData(modelData);
    for (auto& model : modelsData)
    {
        ClearModelVertexData(model);
    }
    //delete[] mesh.Vertices;
    //delete[] mesh.Normals;
    //delete[] mesh.Faces;
    //delete newVertices;
    //delete newNormals;
    //delete newTextureCoords;

    return EXIT_SUCCESS;
}

void updateViewMatrix()
{
    glm::quat qYaw   = glm::angleAxis(-m_cameraState.angles.x, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat qPitch = glm::angleAxis(-m_cameraState.angles.y, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat orientation = glm::normalize(qYaw * qPitch);
    glm::mat4 rotate = glm::mat4_cast(orientation);
    
    glm::mat4 translate = glm::mat4(1.0f);
    glm::vec3 eye = modelData.Bbox.centroid - glm::vec3(-m_cameraState.position, m_cameraState.zoom);
    translate = glm::translate(translate, -eye);
    view = translate * rotate;
}

void mouse_press(SDL_Event& event)
{
    mouse = glm::vec3(event.motion.x, -event.motion.y + HEIGHT, 1.0f);
    switch (event.button.button)
    {    
        case SDL_BUTTON_LEFT:
        {
            if (CAMERA == FPS)
            {
                isActiveFPS = true;
                lastX = mouse.x;
                lastY = mouse.y;
            }
            else if (CAMERA == TRACK_BALL)
            {
                m_drag.active = true;
                m_drag.startMouse.x = mouse.x;
                m_drag.startMouse.y = mouse.y;
                m_drag.startCameraState = m_cameraState;
            }
            break;
        }
        case SDL_BUTTON_RIGHT:
        {
            // Raytracing.
            modelData.bvo.isTracing = true;
            break;
        }
        if (CAMERA == TRACK_BALL)
        {
            case SDL_BUTTON_MIDDLE:
            {
                m_drag.mouseWheelButtonActive = true;
                m_drag.mouseWheelActionStartMouse.x = mouse.x;
                m_drag.mouseWheelActionStartMouse.y = mouse.y;
                m_drag.startCameraState = m_cameraState;
                break;
            }
        }
        default:
        {
            break;
        }
    }
}

void mouse_unpressed(SDL_MouseButtonEvent& button)
{
    switch (button.button)
    {
        case SDL_BUTTON_LEFT:
        {
            if (CAMERA == FPS)
            {
                isActiveFPS = false;
            }
            else if (CAMERA == TRACK_BALL)
            {
                m_drag.active = false;
            }
            break;
        }
        case SDL_BUTTON_RIGHT:
        {
            modelData.bvo.isTracing = false;
            break;
        }
        if (CAMERA == TRACK_BALL)
        {
            case SDL_BUTTON_MIDDLE:
            {
                m_drag.mouseWheelButtonActive = false;
                break;
            }
        }
        default:
        {
            break;
        }
    }
}

void mouse_motion(SDL_Event& event)
{
    glm::vec2 currentMouse = glm::vec2(event.motion.x, -event.motion.y + HEIGHT);
    if (CAMERA == FPS)
    {
        if (isActiveFPS)
        {
            float xoffset = currentMouse.x - lastX;
            float yoffset = lastY - currentMouse.y; // reversed since y-coordinates go from bottom to top
            lastX = currentMouse.x;
            lastY = currentMouse.y;

            float sensitivity = 0.1f; // Who doesn't like magic values?
            xoffset *= sensitivity;
            yoffset *= sensitivity;

            yaw += xoffset;
            pitch += yoffset;

            // make sure that when pitch is out of bounds, screen doesn't get flipped
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;

            // Kill me math wizards, or lock me then Gimbal...
            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            camFront = glm::normalize(front);
            view = glm::lookAt(camPos, camPos + camFront, camUp);
        }
    }
    else if (CAMERA == TRACK_BALL)
    {
        // Rotate.
        if (m_drag.active)
        {
            glm::vec2 delta = (glm::vec2(m_drag.startMouse.x, currentMouse.y) -
                               glm::vec2(currentMouse.x, m_drag.startMouse.y)) *
                               m_drag.sensitivity;
            m_cameraState.angles = m_drag.startCameraState.angles + delta;
            updateViewMatrix();
        }
        // Pan.
        if (m_drag.mouseWheelButtonActive)
        {
            glm::vec2 delta = (m_drag.mouseWheelActionStartMouse - 
                               glm::vec2(currentMouse.x, currentMouse.y)) *
                               m_drag.sensitivity;
            m_cameraState.position = m_drag.startCameraState.position + delta;
            updateViewMatrix();
        }
    }
    // Raytrace.
    if (modelData.bvo.isTracing)
    {
        rayTrace(event.motion.x + (splitScreen ? WIDTH / 4 : 0), event.motion.y, widthHeight, 
                 /* In and Out */modelData, 
                 projection, view, false);
    }
}

void mouse_wheel(SDL_MouseWheelEvent& mouseWheel)
{
    if (CAMERA == TRACK_BALL)
    {
        m_cameraState.zoom += m_drag.scrollSensitivity * mouseWheel.preciseY;
        updateViewMatrix();
    }
}

void key_down(SDL_Event& event)
{
    int keyCode = event.key.keysym.sym;
    
    if (CAMERA == TRACK_BALL)
    {
        switch(keyCode)
        {
            case SDLK_s:
            //case SDLK_LSHIFT:
            {
                m_drag.mouseWheelButtonActive = true;
                //if (m_drag.mouseWheelButtonActiveSince == 0u)
                {
                    //m_drag.mouseWheelActionStartMouse.x = (float)(event.motion.x);
                    //m_drag.mouseWheelActionStartMouse.y = (float)(-event.motion.y + HEIGHT);
                    m_drag.startCameraState = m_cameraState;
                }
                m_drag.mouseWheelButtonActiveSince++;
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if (CAMERA == FPS)
    {
        float camSpeed = deltaTime * SPEED;
        switch (keyCode)
        {
            case SDLK_w:
            {
                camPos += camSpeed * camFront;
                //std::cout << "Pressed W" <<"\n";
                break;
            }
            case SDLK_s:
            {
                camPos -= camSpeed * camFront;
                //std::cout << "Pressed S" <<"\n";
                break;
            }
            case SDLK_a:
            {
                camPos -= camSpeed * glm::normalize(glm::cross(camFront, camUp));
                //std::cout << "Pressed A" <<"\n";
                break;
            }
            case SDLK_d:
            {
                camPos += camSpeed * glm::normalize(glm::cross(camFront, camUp));
                //std::cout << "Pressed D" <<"\n";
                break;
            }
            default:
            {
                break;
            }
        }
        view = glm::lookAt(camPos, camPos + camFront, camUp);
    }
}

void key_up(SDL_Event& event)
{
    if (CAMERA == TRACK_BALL)
    {
        int key = event.key.keysym.sym;
        switch(key)
        {
            case SDLK_s:
            //case SDLK_LSHIFT:
            {
                //m_drag.mouseWheelActionStartMouse.x = event.motion.x;
                //m_drag.mouseWheelActionStartMouse.y = -event.motion.y + HEIGHT;
                std::cout << "S Key Out" << std::endl;
                m_drag.mouseWheelButtonActive = false;
                m_drag.mouseWheelButtonActiveSince = 0u;
                break;
            }
            default:
            {
                break;
            }
        }
    }
}
/*
 EMSCRIPTEN_KEEPALIVE
 void loadAlbedo(uint8_t* buf, int bufSize)
 {
#ifdef OPTIMIZE
#else
     std::cout << "Reading albedo from file!" << std::endl;
     std::cout << "Albedo buffer size: " << bufSize << std::endl;
#endif
     geometryPass.createTextureFromFile(&(modelData.material.baseColor), buf, geometryPass.Width, geometryPass.Height, "BaseColor", 0);
 }
 EMSCRIPTEN_KEEPALIVE
 void passSizeAlbedo(uint16_t* buf, int bufSize)
 {
#ifdef OPTIMIZE
#else
     std::cout << "Reading Albedo image size!" << std::endl;
#endif
     int width  = (int)buf[0];
     int height = (int)buf[1];

     geometryPass.use();
     geometryPass.Width  = width;
     geometryPass.Height = height;
     flipAlbedo = (int)buf[2];
     
#ifdef OPTIMIZE
#else
     std::cout << "Albedo size changed regenerating glTexImage2D" << std::endl;
#endif
     glDeleteTextures(1, &(textureSpaceFramebuffer.texture));
     glDeleteTextures(1, &(modelData.material.baseColor));

     glBindFramebuffer(GL_FRAMEBUFFER, textureSpaceFramebuffer.framebuffer);

     glGenTextures(1, &(textureSpaceFramebuffer.texture));
     glBindTexture(GL_TEXTURE_2D, textureSpaceFramebuffer.texture);
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, geometryPass.Width, geometryPass.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureSpaceFramebuffer.texture, 0);
     
#ifdef OPTIMIZE
#else
     std::cout << "Albedo Width: "  << +geometryPass.Width  << std::endl;
     std::cout << "Albedo Height: " << +geometryPass.Height << std::endl;
     std::cout << "Changed Albedo: "<< +flipAlbedo        << std::endl;
#endif
 }
 */

void regenerateTexture(Shader& shader, frameBuffer& framebuffer, unsigned int* texture, const std::string& fileName, const std::string& samplerName, const int& uniform)
{
    shader.use();
    glDeleteTextures(1, &(framebuffer.texture));
    glDeleteTextures(1, texture);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.framebuffer);
    shader.createTexture(texture, fileName, samplerName, uniform);
}

void regenerateModel(Shader& shader, ModelData& newModelData, frameBuffer& framebuffer, const std::string& fileNameBaseColor, const std::string& fileNameNormal)
{
    /*ClearModelVertexData(modelData);
    modelData = modelsData[1];
    isGLTF = true;
    flipper = true;
    regenerateTexture(geometryPass, textureSpaceFramebuffer, &(modelData.material.baseColor), fileNames[0].normal, "BaseColor", 0);
    regenerateTexture(geometryPass, textureSpaceFramebuffer, &(modelData.material.normal), fileNames[0].normal, "Normal", 1);
    CreateBOs(modelData)*/
    ClearModelVertexData(modelData);
    modelData = newModelData;
    isGLTF = true;
    flipper = true;
    regenerateTexture(shader, framebuffer, &(modelData.material.baseColor), fileNameBaseColor, "BaseColor", 0);
    regenerateTexture(shader, framebuffer, &(modelData.material.normal),    fileNameNormal,    "Normal",    1);
    CreateBOs(modelData);
}

void main_loop()
{
    widthHeight = glm::vec2(WIDTH, HEIGHT);

    // No need to compute this every frame as the FOV stays always the same.
    glm::vec2 halfWidthHeight = widthHeight * glm::vec2(0.5, 0.5);
    float aspect = halfWidthHeight.x / halfWidthHeight.y;

    // calculateNearFarPlane(bboxMax, centroid, /** Out **/ near, far);
    // differenceBboxMaxMin = bboxMax - bboxMin;
    projection = glm::perspective(RADIANS_30, widthHeight.x / widthHeight.y, nearPlane, farPlane);
    glm::mat4 projectionHalf = (splitScreen ? 
                                glm::perspective(RADIANS_30, halfWidthHeight.x / widthHeight.y, nearPlane, farPlane) : 
                                projection);
    glm::mat4 projectionSide //= glm::ortho(bboxMin.x, bboxMax.x, bboxMin.y, bboxMax.y, bboxMin.z, bboxMax.z);
        = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1000.0f, 1000.0f);
    //= glm::ortho(0.0f, halfWidthHeight.x, halfWidthHeight.y / 2.0f, 0.0f, near, far);
    
    if (decalImageBuffer.size() > 0)
    {
        recomputeDecalBaseColorTexture();
    }

    /**
     * Start ImGui
     */	        
    bool shiftIsPressed = false;

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

        // Crude way to solve dragging lag.
        const bool dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
        // TODO: fix this
        /*if (SDL_GL_SetSwapInterval(dragging ? 0 : 1) == -1)
        {
            std::cout << "Unable to disable VSync!" << std::endl;
        }*/

        insideImGui = io.WantCaptureMouse;
        keyboardImGui = io.WantCaptureKeyboard;

        if (insideImGui || keyboardImGui)
        {
            break;
        }
        
        // Only do input handling on the perspective viewport.
        bool isInPerspectiveViewport = (event.motion.x < halfWidth ? true : false);
        isInPerspectiveViewport = splitScreen ? isInPerspectiveViewport : true;

        if (!isInPerspectiveViewport)
        {
            break;
        }

        switch(event.type)
        {
            case SDL_MOUSEBUTTONDOWN:
            {
                mouse_press(event);
                break;
            }
            case SDL_MOUSEBUTTONUP:
            {
                mouse_unpressed(event.button);
                break;
            }
            case SDL_MOUSEMOTION:
            {
                mouse_motion(event);
                break;
            }
            case SDL_MOUSEWHEEL:
            {
                mouse_wheel(event.wheel);
                break;
            }
            case SDL_KEYDOWN:
            {
                key_down(event);
                break;
            }
            case SDL_WINDOWEVENT:
            {
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    WIDTH = event.window.data1;
                    HEIGHT = event.window.data2;
                }
                break;
            }
            case SDL_KEYUP:
            {
                int key = event.key.keysym.sym;
                if (key != SDLK_ESCAPE)
                {
                    key_up(event);
                }
                else
                {
                    main_loop_running = false;
                }
                break;
            }
            case SDL_QUIT:
            {
                main_loop_running = false;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    ImGui::Begin("Graphical User Interface");   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    std::string printTime = std::to_string(deltaTime * 1000.0f) + " ms.\n";
    ImGui::TextUnformatted(printTime.c_str());
    static const char* cameraTypes[]{"FPS", "Trackball"};
    static const char* selectedItem = cameraTypes[1];
    if (ImGui::BeginCombo("Camera", selectedItem, 0))
    {
        for (int n = 0; n < IM_ARRAYSIZE(cameraTypes); n++)
        {
            bool is_selected = (selectedItem == cameraTypes[n]);
            if (ImGui::Selectable(cameraTypes[n], is_selected))
                selectedItem = cameraTypes[n];
            if (is_selected)
                ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
        }
        ImGui::EndCombo();
    }
    //std::cout << "CAMERA: " << CAMERA << std::endl;
    if (strcmp(selectedItem, "FPS") == 0)
    {
        CAMERA = FPS;
        //std::cout << "FPS" << std::endl;
    }
    else if (strcmp(selectedItem, "Trackball") == 0)
    {
        CAMERA = TRACK_BALL;
        //std::cout << "Trackball" << std::endl;
    }
    static const char* modelsToSelect[]{ "Workboot", /*"Clay",*/ "Shirt" };
    static const char* selectedModel = modelsToSelect[0];
    if (ImGui::BeginCombo("Model", selectedModel, 0))
    {
        for (int n = 0; n < IM_ARRAYSIZE(modelsToSelect); n++)
        {
            bool is_selected = (selectedModel == modelsToSelect[n]);
            if (ImGui::Selectable(modelsToSelect[n], is_selected))
                selectedModel = modelsToSelect[n];
            if (is_selected)
                ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
        }
        ImGui::EndCombo();
    }
    //std::cout << "CAMERA: " << CAMERA << std::endl;
    if (strcmp(selectedModel, "Workboot") == 0)
    {
        if (currentModel != SHIRT)
        {
            currentModel = SHIRT;
            regenerateModel(geometryPass, modelsData[1], textureSpaceFramebuffer, fileNames[1].baseColor, fileNames[1].normal);
            /*ClearModelVertexData(modelData);
            modelData = modelsData[0];
            isGLTF = !true;
            geometryPass.createTexture(&(modelData.material.baseColor), fileNames[0].baseColor, "BaseColor", 0);
            geometryPass.createTexture(&(modelData.material.normal),    fileNames[0].normal,    "Normal",    1);
            //reloadModel(modelData);
            CreateBOs(modelData);*/
            //rayTrace(WIDTH / 2, HEIGHT / 2, widthHeight,
            //         /** In and Out **/ modelData,
            //         projection, view, false);
            std::cout << "Workboot" << std::endl;
        }
    }
    /*else if (strcmp(selectedModel, "Clay") == 0)
    {
        currentModel = CLAY;
    }*/
    else if (strcmp(selectedModel, "Shirt") == 0)
    {
        if (currentModel != WORKBOOT)
        {
            currentModel = WORKBOOT;
            regenerateModel(geometryPass, modelsData[0], textureSpaceFramebuffer, fileNames[0].baseColor, fileNames[0].normal);
            /*ClearModelVertexData(modelData);
            modelData = modelsData[1];
            isGLTF = true;
            flipper = true;
            regenerateTexture(geometryPass, textureSpaceFramebuffer, &(modelData.material.baseColor), fileNames[0].normal, "BaseColor", 0);
            regenerateTexture(geometryPass, textureSpaceFramebuffer, &(modelData.material.normal), fileNames[0].normal, "Normal", 1);
            CreateBOs(modelData);*/
            std::cout << "Shirt" << std::endl;
        }
    }
    if (ImGui::Button("Split Screen"))
    {
        splitScreen = !splitScreen;
    }
    if (ImGui::Button("Enable Normal Map"))
    {
        normalMap = !normalMap;
    }
    ImGui::SliderInt("Texture Coordinates Scale", &scale, 1, 10);
    ImGui::SliderFloat("Blend Factor", &blend, 0.0f, 1.0f);
    ImGui::SliderFloat("Projector Size", &projectorSize, 0.001f, 1.0f);
    ImGui::SliderFloat("Projector Orientation", &projectorRotation, 0.0f, 360.0f); 
    if (ImGui::Button("Flip decals"))
    {
        flipDecal = !flipDecal;
    }
    if (ImGui::Button("Show Bounding Box"))
    {
        showBBox = !showBBox;
    }
    if (ImGui::Button("Show Projector"))      
    {
        showProjector = !showProjector;
    }
    if (ImGui::Button("Show Hit Point"))
    {
        showHitPoint = !showHitPoint;
    }
    ImGui::SliderFloat("Side Zoom", &zoomSide, 0.1f, 2.0f);
    ImGui::SliderFloat("Front Zoom", &zoomTop, 0.1f, 2.0f);
    // If we have a GLTF we need to invert the texture coordinates.
    flipper = isGLTF ? 1 : 0;
    decalsPass.use();
    decalsPass.setInt("iFlipper", flipper);

    for (int i = 0; i < 1; ++i)
    {
        ImGuizmo::SetID(i);
        EditTransform(glm::value_ptr(view), glm::value_ptr(projectionHalf), glm::value_ptr(modelData.modelMatrix), lastUsing == i, splitScreen, io);
        if (ImGuizmo::IsUsing())
        {
            lastUsing = i;
        }
    }
    
    ImGui::End();

    flip = (flipDecal ? 1 : 0);

    // Calculate the time between frames.
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    time_internal = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    deltaTime = time_internal - lastFrame;
    lastFrame = time_internal;

    // render
    // ------
    /** Start Depth Pre-Pass **/
    glEnable(GL_DEPTH_TEST);
    glDepthMask(true);
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFramebuffer.framebuffer);

    depthPrePass.use();
    glBindVertexArray(modelData.openGLObject.VAO); 
    depthPrePass.setMat4("decalProjector", modelData.bvo.decalProjector);
    glViewport(0, 0, WIDTH/4, HEIGHT/4);

    glClear(GL_DEPTH_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, modelData.indexes.size(), GL_UNSIGNED_INT, 0);

    /** End Depth Pre-Pass **/

    float flipFloat = (float) flip; 
    float flipAlbedoFloat = (float) flipAlbedo;


    /** Start Decal Pass **/
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, textureSpaceFramebuffer.framebuffer);
    decalsPass.use();
    decalsPass.setMat4("model", modelData.modelMatrix);
    decalsPass.setMat4("projection", projection);
    decalsPass.setMat4("view", view);
    decalsPass.setVec2("iResolution", widthHeight);
    decalsPass.setInt("iScale", scale);
    decalsPass.setFloat("iFlip", flipFloat);
    decalsPass.setFloat("iBlend", blend);
    decalsPass.setMat4("decalProjector", modelData.bvo.decalProjector);
    decalsPass.setFloat("iFlipAlbedo", flipAlbedoFloat);

    glBindVertexArray(modelData.openGLObject.VAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, modelData.material.baseColor);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, modelData.material.decalBaseColor);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthFramebuffer.texture);

    glViewport(0, 0, geometryPass.Width, geometryPass.Height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, modelData.indexes.size(), GL_UNSIGNED_INT, 0);

    if (downloadImage == 1 || frame == 0u)
    {
        decalResult = uploadImage();
        // counter = 0;
        downloadImage = 0u;
#ifdef OPTIMIZE
#else
        std::cout << "Uploading image!" << std::endl;
#endif
    }
    /** End Decal Pass **/

    /** Start Light Pass **/
    //glScissor(0, 0, WIDTH/2, HEIGHT);
    int enableNormals = normalMap ? 1 : 0;
    if (splitScreen)
    {
        glViewport(0, 0, WIDTH/2, HEIGHT);
    }
    else
    {
        //std::cout << "Window width: " << WIDTH << " height: " << HEIGHT << " aspect: " << aspect << std::endl;
        glViewport(0, 0, WIDTH, HEIGHT);
    }
    /*std::cout << "Print data before first draw call: " << std::endl;
    printModelData(modelData);*/
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(modelData.openGLObject.VAO);
    deferredPass.use();
    deferredPass.setMat4("model", modelData.modelMatrix);
    deferredPass.setMat4("projection", projectionHalf);
    deferredPass.setMat4("view", view);
    deferredPass.setFloat("iFlipAlbedo", flipAlbedoFloat);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, modelData.material.normal);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureSpaceFramebuffer.texture);
    // glActiveTexture(GL_TEXTURE2);
    // glBindTexture(GL_TEXTURE_2D, material.metallic);
    // glActiveTexture(GL_TEXTURE3);
    // glBindTexture(GL_TEXTURE_2D, material.ao);

    deferredPass.setVec3("viewPos", camPos);
    deferredPass.setFloat("iTime", iTime);
    deferredPass.setInt("iFlipper", flipper);
    deferredPass.setInt("iNormals", enableNormals);
    deferredPass.setFloat("iTime", iTime);
    // finally render quad
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, modelData.indexes.size(), GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(0);
    
    /** End Light Pass **/
    
    if (showBBox)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);

        hitPosition.use();
        hitPosition.setMat4("model",      modelData.modelMatrix);
        hitPosition.setMat4("projection", projectionHalf);
        hitPosition.setMat4("view",       view);
        renderLineCube(modelData.Bbox.bboxMin, modelData.Bbox.bboxMax);
    }

    if (showHitPoint)
    {
        glm::mat4 hitPositionModel = modelData.modelMatrix;
        hitPositionModel = glm::translate(hitPositionModel, modelData.bvo.hitPos);
        hitPositionModel = glm::scale(hitPositionModel, glm::vec3(0.01f));

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);

        hitPosition.use();
        hitPosition.setMat4("model",      hitPositionModel);
        hitPosition.setMat4("projection", projectionHalf);
        hitPosition.setMat4("view",       view);
        renderCube();
    }

    if (showProjector)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);

        hitPosition.use();
        hitPosition.setMat4("model",      modelData.modelMatrix);
        hitPosition.setMat4("projection", projectionHalf);
        hitPosition.setMat4("view",       view);
        renderFrustum(modelData.bvo.decalProjector);
    }
    
    if (splitScreen)
    {
        // Side View.
        glViewport(WIDTH/2, 0, WIDTH/2, HEIGHT/2);

        // Scale for the side view.
        glm::mat4 modelSide = glm::scale(modelNoGuizmo, glm::vec3(zoomSide, zoomSide, zoomSide));
        modelSide = glm::rotate(modelSide, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        //modelSide = glm::translate(modelSide, glm::vec3(0.0f, -0.5f, 0.0f) * differenceBboxMaxMin);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(modelData.openGLObject.VAO);
        deferredPass.use();
        deferredPass.setMat4("model", modelSide);
        deferredPass.setMat4("projection", projectionSide);
        deferredPass.setMat4("view", viewPinned);/*glm::mat4(-0.113205, -0.187880, 0.975646, 0.000000, 
                                                0.000000, 0.981959, 0.189095, 0.000000, 
                                                -0.993572, 0.021407, -0.111162, 0.000000, 
                                                -0.064962, 0.388481, -4.221102, 1.000000));*/
        deferredPass.setFloat("iFlipAlbedo", flipAlbedoFloat);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, modelData.material.normal);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureSpaceFramebuffer.texture);
        // glActiveTexture(GL_TEXTURE2);
        // glBindTexture(GL_TEXTURE_2D, material.metallic);
        // glActiveTexture(GL_TEXTURE3);
        // glBindTexture(GL_TEXTURE_2D, material.ao);

        deferredPass.setVec3("viewPos", camPos);
        deferredPass.setFloat("iTime", iTime);
        deferredPass.setInt("iFlipper", flipper);
        deferredPass.setInt("iNormals", 0);//enableNormals);
        deferredPass.setFloat("iTime", iTime);

        glDrawElements(GL_TRIANGLES, modelData.indexes.size(), GL_UNSIGNED_INT, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(0);

        // Front View.
        glViewport(WIDTH/2, HEIGHT/2, WIDTH/2, HEIGHT/2);

        // Scale for the side view.
        glm::mat4 modelTop = glm::scale(modelNoGuizmo, glm::vec3(zoomTop, zoomTop, zoomTop));
        //modelTop = glm::translate(modelTop, glm::vec3(0.0f, -0.5f, 0.5f) * differenceBboxMaxMin);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(modelData.openGLObject.VAO);
        deferredPass.use();
        deferredPass.setMat4("model", modelTop);
        deferredPass.setMat4("projection", projectionSide);
        deferredPass.setMat4("view", viewPinned);/*glm::mat4(1.0, 0.0,  0.0, 0.0, 
                                                0.0, 1.0,  0.0, 0.0, 
                                                0.0, 0.0,  1.0, 0.0, 
                                                0.0, 0.0, -1.0, 1.0));*/
        deferredPass.setFloat("iFlipAlbedo", flipAlbedoFloat);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, modelData.material.normal);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureSpaceFramebuffer.texture);
        // glActiveTexture(GL_TEXTURE2);
        // glBindTexture(GL_TEXTURE_2D, material.metallic);
        // glActiveTexture(GL_TEXTURE3);
        // glBindTexture(GL_TEXTURE_2D, material.ao);

        deferredPass.setVec3("viewPos", camPos);
        deferredPass.setFloat("iTime", iTime);
        deferredPass.setInt("iFlipper", flipper);
        deferredPass.setInt("iNormals", 0);//enableNormals);
        deferredPass.setFloat("iTime", iTime);

        glDrawElements(GL_TRIANGLES, modelData.indexes.size(), GL_UNSIGNED_INT, 0);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(0);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);

    SDL_PumpEvents();  // make sure we have the lascale mouse state.

    SDL_GL_GetDrawableSize(window, &WIDTH, &HEIGHT);

    iTime += 1.0f / 100.0f;
    frame++;
    counter++;
    changeDecal = 0u;
}

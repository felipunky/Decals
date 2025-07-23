#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

#include "ImGuizmo.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/emscripten.h>
    #include <emscripten/html5.h>
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
#include <optional>

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
const int JFA_FACTOR = 4;
const int DEPTH_FACTOR = 4;

const float SPEED = 5.0f;
// How much time between frames.
float deltaTime = 0.0f, lastFrame = 0.0f;
const float depthBias = 0.00025;

SDL_Window* window;
SDL_GLContext context;
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;
#endif
const char* glsl_version = nullptr;//"#version 300 es";
static bool main_loop_running = true;
bool frameIsEven = false;

int newVerticesSize;
int flipAlbedo = 0;
uint8_t reload = 0u;
uint8_t downloadImage = 1u;
std::vector<uint8_t> decalAlbedoImageBuffer,
                     decalNormalImageBuffer;
std::vector<uint8_t> decalAlbedoResult,
                     decalNormalResult;
uint16_t widthDecal  = 1024u,
         heightDecal = 1024u,
         widthAlbedo,
         heightAlbedo,
         changeAlbedo = 0u;
int flip = 0;
bool flipDecal = false;
Shader depthPrePass   = Shader(),
       geometryPass   = Shader(),
       deferredPass   = Shader(),
       hitPosition    = Shader(),
       decalsPass     = Shader(),
       JFAPass        = Shader(),
       SDFPass        = Shader(),
       fullScreenPass = Shader();
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

glm::mat4 view             = glm::mat4(1.0f);
glm::mat4 projection       = glm::mat4(1.0f);
glm::mat4 modelNoGuizmo    = glm::mat4(1.0f);
glm::mat4 viewPinnedTop    = glm::mat4(1.0f);
glm::mat4 viewPinnedBottom = glm::mat4(1.0f);
glm::vec2 widthHeight      = glm::vec2(WIDTH, HEIGHT);
glm::vec2 currentWindowWidthAndHeight = widthHeight;
bool isActiveFPS = false;
bool iMetal = true,
     iRough = true;

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
    // Same as zoom but for the side panels (the top and side views).
    float zoomSide = 0.5f;
    float zoomTop  = 0.5f;
    // Positions to pan on side panels.
    glm::vec2 positionTop = { 0.0f, 0.0f };
    glm::vec2 positionBottom  = { 0.0f, 0.0f };
};
CameraState m_cameraState;

struct DragState {
    // Whether a drag action is ongoing (i.e., we are between mouse press and mouse release)
    bool active = false;
    // Mouse wheel being pressed.
    bool mouseWheelButtonActive = false;
    uint mouseWheelButtonActiveSince = 0u;
    // Drag action for the side panels.
    bool activeTopPanel = false;
    bool activeBottomPanel = false;
    // Drag action for tracing with touch pad.
    bool activeTouchPad = false;
    // The position of the mouse at the beginning of the drag action
    glm::vec2 startMouse;
    // The position of the mouse at the beginning of the mouse wheel button action.
    glm::vec2 mouseWheelActionStartMouse;
    // The position of the mouse at the beginning of the right click on the top panel.
    glm::vec2 mouseTopActionStartMouse;
    // The position of the mouse at the beginning of the right click on the bottom panel.
    glm::vec2 mouseBottomActionStartMouse;
    // The camera state at the beginning of the drag action
    CameraState startCameraState;

    // Constant settings
    float sensitivity = 0.01f;
    float scrollSensitivity = 1.0f;
    float scrollSensitivityPanels = 0.1f;
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
bool showBBox      = !false;
bool showProjector = !false;
bool showHitPoint  = !false;
bool normalMap     = !false;
bool splitScreen   = false;
bool alphaJFA      = true;
bool showSDFBox    = false;
bool pbr           = true;

int scale = 1;
float blend = 0.5f;
float alphaCut = 0.1f;
float smoothAlpha = 0.5f;
int distanceWidthJFA = 8;
int fullScreenPassRepeat = 2;
int jfaFactor = 4;

enum SHOW_TEXTURES
{
    CURRENT_MODEL,
    TEXTURE_SPACE,
    NORMALS,
    METALLIC,
    ROUGHNESS,
    JFA
};
SHOW_TEXTURES showTextures = CURRENT_MODEL;

/** End ImGui params. */

unsigned int frame    = 0,
             frameJFA = 0,
             counter  = 0,
             flipper  = 0;

float iTime;

enum MODEL
{
    WORKBOOT,
    SHIRT,
    JACKET,
    SPHERE
};
MODEL currentModel = SPHERE;

GLenum drawingMode = GL_TRIANGLE_STRIP;

std::string fileToString(const std::string& fileName)
{
    std::ifstream in(fileName);
    std::string contents((std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>());
    return contents;
}

std::vector<uint8_t> uploadImage(int attachment)
{
    unsigned int bufferSize = 4 * geometryPass.Width * geometryPass.Height;
    std::vector<uint8_t> buffer(bufferSize);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_COLOR_ATTACHMENT0 + attachment);
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
    unsigned int metallic;
    unsigned int roughness;
    unsigned int decalBaseColor;
    unsigned int decalNormal;
    unsigned int ao;
    // Each element of the following array contains the number of channels of each of the textures.
    // 0 Albedo, 1 Normal, 2 Metallic, 3 Roughness, 4 AO, 5 Decal Albedo, 6 Decal Normal. 
    std::array<int, 7> channels;
};

struct OpenGLObject
{
    unsigned int /*VBOVertices,
        VBONormals,
        VBOTextureCoordinates,
        VBOTangents,*/
        VBO,
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

std::vector<ModelData> modelData;

struct ModelFileNames
{
    std::string mesh,
                baseColor,
                normal,
                roughness,
                metallic,
                ao,
                decalBaseColor,
                decalNormal;
};

std::vector<ModelFileNames> fileNames(4);
std::vector<std::vector<ModelData>> modelsData(fileNames.size());

const void printModelData(const ModelData& modelData)
{
    std::cout << "Vertices size: "         << modelData.vertices.size()                    << "\n" <<
                 "Normals size: "          << modelData.normals.size()                     << "\n" <<
                 "Tangents size: "         << modelData.tangents.size()                    << "\n" <<
                 "Indices size: "          << modelData.indexes.size()                     << "\n" <<
                 //"OpenGL VBO Vertices: "   << modelData.openGLObject.VBOVertices           << "\n" <<
                 //"OpenGL VBO Normals: "    << modelData.openGLObject.VBONormals            << "\n" <<
                 //"OpenGL VBO Tangents: "   << modelData.openGLObject.VBOTangents           << "\n" <<
                 //"OpenGL VBO TexCoords: "  << modelData.openGLObject.VBOTextureCoordinates << "\n" <<
                 "OpenGL VBO: "            << modelData.openGLObject.VBO                   << "\n" <<
                 "OpenGL EBO: "            << modelData.openGLObject.EBO                   << "\n" <<
                 "OpenGL VAO: "            << modelData.openGLObject.VAO                   << "\n" <<
                 "Texture BaseColor: "     << modelData.material.baseColor                 << "\n" <<
                 "Texture Normal: "        << modelData.material.normal                    << "\n" <<
    std::endl;
}

const void printModelData(const std::vector<ModelData>& modelData)
{
    for (const auto& model : modelData)
    {
        printModelData(model);
    }
}

void reloadModel(std::vector<ModelData>& modelData);
void ObjLoader(std::string inputFile, std::vector<ModelData>& modelData);
void loadGLTF(tinygltf::Model& model, std::vector<ModelData>& modelData);

struct frameBuffer
{
    unsigned int framebuffer;
    std::vector<unsigned int> textures;
};

struct FrameBufferTextureParams
{
    GLint INTERNAL_FORMAT,
          FORMAT;
    GLenum TYPE;
};

void regenerateFramebufferTextureJFA(const glm::ivec2& widthHeight, frameBuffer& framebuffer, const FrameBufferTextureParams& frameBufferTextureParams)
{
    glDeleteTextures(1, &(framebuffer.textures[0]));
    
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.framebuffer);
    glGenTextures(1, &(framebuffer.textures[0]));
    glBindTexture(GL_TEXTURE_2D, framebuffer.textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, frameBufferTextureParams.INTERNAL_FORMAT, widthHeight.x, widthHeight.y, 0, frameBufferTextureParams.FORMAT, frameBufferTextureParams.TYPE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer.textures[0], 0);
}

void regenerateAllFramebufferTexturesJFA(std::vector<frameBuffer>& jfaFrameBuffer, frameBuffer& sdfFramebuffer, glm::ivec4& widthHeightJFA, const FrameBufferTextureParams& frameBufferTextureParams, FrameBufferTextureParams& frameBufferTextureParamsSDF)
{
    widthHeightJFA = glm::ivec4(decalsPass.Width, decalsPass.Height, decalsPass.Width / jfaFactor, decalsPass.Height / jfaFactor);
    
    for (uint8_t i = 0u; i < (uint8_t)jfaFrameBuffer.size(); ++i)
    {
        regenerateFramebufferTextureJFA(glm::ivec2(widthHeightJFA.z, widthHeightJFA.w), jfaFrameBuffer[i], frameBufferTextureParams);
    }
    regenerateFramebufferTextureJFA(glm::ivec2(widthHeightJFA.z, widthHeightJFA.w), sdfFramebuffer, frameBufferTextureParamsSDF);
}

frameBuffer depthFramebuffer;
frameBuffer textureSpaceFramebuffer;
std::vector<frameBuffer> jfaFrameBuffer(2);
frameBuffer sdfFramebuffer;

Shader::TEXTURE_WRAP_PARAMS textureWrapParamsDecalOutputs = Shader::CLAMP_TO_EDGE;// REPEAT;
Shader::TEXTURE_SAMPLE_PARAMS textureSampleParamsDecalOutputs = Shader::LINEAR_MIPS;// LINEAR;

#ifdef __EMSCRIPTEN__
FrameBufferTextureParams frameBufferTextureParamsJFA{ GL_RGBA16F, GL_RGBA, GL_FLOAT };
FrameBufferTextureParams frameBufferTextureParamsSDF{ GL_R16F, GL_RED, GL_FLOAT };
#else
FrameBufferTextureParams frameBufferTextureParamsJFA{ GL_RGBA32F, GL_RGBA, GL_FLOAT };
FrameBufferTextureParams frameBufferTextureParamsSDF{ GL_R32F, GL_RED, GL_FLOAT };
#endif

bool isGLTF = false;

void regenerateFramebufferTexture(frameBuffer& framebuffer, const FrameBufferTextureParams& textureParams, Shader& shader, 
                                  const Shader::TEXTURE_WRAP_PARAMS& wrapParam, const Shader::TEXTURE_SAMPLE_PARAMS& sampleParam, const int& index)
{
    //glDeleteTextures(1, &(framebuffer.textures[index]));

    glGenTextures(1, &(framebuffer.textures[index]));
    glBindTexture(GL_TEXTURE_2D, framebuffer.textures[index]);
    int mipLevels = (int)(std::floor(std::log2(std::max(shader.Width, shader.Height)))) + 1;
	int mipWidth = shader.Width;
	int mipHeight = shader.Height;
	//glTexStorage2D(GL_TEXTURE_2D, mipLevels, textureParams.INTERNAL_FORMAT, shader.Width, shader.Height);
    for (int i = 0; i < mipLevels; ++i)
    {
        glTexImage2D(GL_TEXTURE_2D, i, textureParams.INTERNAL_FORMAT, mipWidth, mipHeight, 0, textureParams.FORMAT, textureParams.TYPE, NULL);
		mipWidth = std::max(1, mipWidth / 2);
		mipHeight = std::max(1, mipHeight / 2);
    }
    shader.textureWrap(wrapParam);
    shader.textureSample(sampleParam);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + ((GLenum)index), GL_TEXTURE_2D, framebuffer.textures[index], 0);

    // Check framebuffer texture creation.
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) 
    {
        std::cout << "Framebuffer is complete!" << std::endl;
    }
    else 
    {
        std::cerr << "Framebuffer is not complete. Status: " << status << std::endl;
        // Handle incomplete framebuffer (e.g., log more details, exit)
    }
}

void regenerateTextureSpaceFramebuffer(ModelData& modelData, frameBuffer& framebuffer, Shader& shader, const Shader::TEXTURE_WRAP_PARAMS& wrapParam, const Shader::TEXTURE_SAMPLE_PARAMS& sampleParam)
{
    uint8_t maxIter = framebuffer.textures.size();
    //uint8_t iWrap = maxIter-1u;
    
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.framebuffer);

    FrameBufferTextureParams fboTextureParams = { modelData.material.channels[0], modelData.material.channels[0], GL_UNSIGNED_BYTE};
    for (size_t i = 0; i < maxIter; ++i)
    {
        regenerateFramebufferTexture(framebuffer, fboTextureParams, shader, wrapParam, sampleParam, (int)i);
    }
}

void rayTrace(const int& mousePositionX, const int& mousePositionY, const glm::vec2& widthHeight, 
              /* In and Out */ std::vector<ModelData>& modelData,
              const glm::mat4& projection, const glm::mat4& view, const bool& debug);

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
        isGLTF = true;
        data.clear();
    }
    EMSCRIPTEN_KEEPALIVE
    void passObj(char* buf)
    {
        std::string result = buf;
        ObjLoader(result, modelData);
        reloadModel(modelData);
        isGLTF = false;
    }
    EMSCRIPTEN_KEEPALIVE
    void loadDecalAlbedo(uint8_t* buf, int bufSize) 
    {
        //printf("[WASM] Loading Texture \n");
#ifdef OPTIMIZE
#else
        std::cout << "Reading decal albedo image!" << std::endl;
        std::cout << "Decal albedo buffer size: " << bufSize << std::endl;
#endif
        glm::ivec4 widthHeightJFA = glm::ivec4(decalsPass.Width, decalsPass.Height, decalsPass.Width / jfaFactor, decalsPass.Height / jfaFactor);
            
        frameJFA = 0;
        decalsPass.createTextureFromFile(&(modelData[0].material.decalBaseColor), buf, decalsPass.Width, decalsPass.Height, "iChannel0", 1);
        regenerateAllFramebufferTexturesJFA(jfaFrameBuffer, sdfFramebuffer, widthHeightJFA, frameBufferTextureParamsJFA, frameBufferTextureParamsSDF);
        rayTrace(mousePositionX + (splitScreen ? WIDTH / 4 : 0), mousePositionY, widthHeight, 
                 /* In and Out */ modelData,
                 projection, view, false);
        downloadImage = 1u;
    }
    EMSCRIPTEN_KEEPALIVE
    void passSizeDecalAlbedo(uint16_t* buf, int bufSize)
    {
#ifdef OPTIMIZE
#else
        std::cout << "Reading decal albedo image size!" << std::endl;
        #endif
        decalsPass.Width  = buf[0];
        decalsPass.Height = buf[1];
#ifdef OPTIMIZE
#else
        std::cout << "Decal Albedo Width: "  << +decalsPass.Width  << std::endl;
        std::cout << "Decal Albedo Height: " << +decalsPass.Height << std::endl;
#endif
    }
    EMSCRIPTEN_KEEPALIVE
    void loadDecalNormal(uint8_t* buf, int bufSize) 
    {
        //printf("[WASM] Loading Texture \n");
#ifdef OPTIMIZE
#else
        std::cout << "Reading decal normal image!" << std::endl;
        std::cout << "Decal normal buffer size: " << bufSize << std::endl;
#endif
        glm::ivec4 widthHeightJFA = glm::ivec4(decalsPass.Width, decalsPass.Height, decalsPass.Width / jfaFactor, decalsPass.Height / jfaFactor);
            
        frameJFA = 0;
        decalsPass.createTextureFromFile(&(modelData[0].material.decalNormal), buf, decalsPass.Width, decalsPass.Height, "iChannel2", 4);
        regenerateAllFramebufferTexturesJFA(jfaFrameBuffer, sdfFramebuffer, widthHeightJFA, frameBufferTextureParamsJFA, frameBufferTextureParamsSDF);
        rayTrace(mousePositionX + (splitScreen ? WIDTH / 4 : 0), mousePositionY, widthHeight, 
                 /* In and Out */ modelData,
                 projection, view, false);
        downloadImage = 1u;
    }
    EMSCRIPTEN_KEEPALIVE
    void passSizeDecalNormal(uint16_t* buf, int bufSize)
    {
#ifdef OPTIMIZE
#else
        std::cout << "Reading decal image size!" << std::endl;
        #endif
        decalsPass.Width  = buf[0];
        decalsPass.Height = buf[1];
#ifdef OPTIMIZE
#else
        std::cout << "Decal Normal Width: "  << +decalsPass.Width  << std::endl;
        std::cout << "Decal Normal Height: " << +decalsPass.Height << std::endl;
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
        geometryPass.createTextureFromFile(&(modelData[0].material.baseColor), buf, geometryPass.Width, geometryPass.Height, "iAlbedo", 0);
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
        glDeleteTextures(1, &(modelData[0].material.baseColor));
        regenerateTextureSpaceFramebuffer(modelData[0], textureSpaceFramebuffer, geometryPass, textureWrapParamsDecalOutputs, textureSampleParamsDecalOutputs);
        
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
        geometryPass.createTextureFromFile(&(modelData[0].material.normal), buf, geometryPass.Width, geometryPass.Height, "Normal", 1);
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
        glDeleteTextures(1, &(modelData[0].material.normal));

        geometryPass.Width  = width;
        geometryPass.Height = height;

        regenerateTextureSpaceFramebuffer(modelData[0], textureSpaceFramebuffer, geometryPass, textureWrapParamsDecalOutputs, textureSampleParamsDecalOutputs);
#ifdef OPTIMIZE
#else
        std::cout << "Normal Width: "  << +geometryPass.Width  << std::endl;
        std::cout << "Normal Height: " << +geometryPass.Height << std::endl;
#endif
    }
    EMSCRIPTEN_KEEPALIVE
    uint8_t* downloadDecalAlbedo(uint8_t *buf, int bufSize) 
    {
        if (decalAlbedoResult.size() > 0)
        {
#ifdef OPTIMIZE
#else
            std::cout << "Successful loading the image into data!" << std::endl;
#endif
            uint8_t* result = &decalAlbedoResult[0];
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
    uint8_t* downloadDecalNormal(uint8_t *buf, int bufSize) 
    {
        if (decalNormalResult.size() > 0)
        {
#ifdef OPTIMIZE
#else
            std::cout << "Successful loading the image into data!" << std::endl;
#endif
            uint8_t* result = &decalNormalResult[0];
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

void rayTrace(const int& mousePositionX, const int& mousePositionY, const glm::vec2& widthHeight,
    /* In and Out */ std::vector<ModelData>& modelData,
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
    glm::vec4 p_far_ndc = glm::vec4(x_ndc, y_ndc, 1.0f, 1.0f); // z far = 1

    for (size_t i = 0; i < modelData.size(); ++i)
    {
        if (modelData[i].bvo.isTracing)
        {
            glm::mat4 invMVP = glm::inverse(projection * view * modelData[i].modelMatrix);

            glm::vec4 p_near_h = invMVP * p_near_ndc;
            glm::vec4 p_far_h = invMVP * p_far_ndc;

            glm::vec3 p0 = glm::vec3(p_near_h) / glm::vec3(p_near_h.w, p_near_h.w, p_near_h.w);
            glm::vec3 p1 = glm::vec3(p_far_h) / glm::vec3(p_far_h.w, p_far_h.w, p_far_h.w);

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

            nanort::TriangleIntersector<> triangle_intersector(glm::value_ptr(modelData[i].vertices[0]), &(modelData[i].indexes[0]), sizeof(float) * 3);
            nanort::TriangleIntersection<> isect;
            bool hit = modelData[i].bvo.accel.Traverse(ray, triangle_intersector, &isect);
            if (hit)
            {
                modelData[i].bvo.hitPos = rayOri + rayDir * isect.t;

                unsigned int fid = isect.prim_id;
                unsigned int id = modelData[i].indexes[3 * fid];
                modelData[i].bvo.hitNor = modelData[i].normals[id];

                projectorPos = modelData[i].bvo.hitPos + modelData[i].bvo.hitNor * PROJECTOR_DISTANCE;
                projectorDir = -modelData[i].bvo.hitNor;

                float ratio = float(decalsPass.Height) / float(decalsPass.Width);
                float proportionateHeight = projectorSize * ratio;

                glm::mat4 rotate = glm::mat4(1.0f);
                rotate = glm::rotate(rotate, glm::radians(projectorRotation), projectorDir);

                glm::vec4 axis = rotate * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
                glm::mat4 projectorView = glm::lookAt(projectorPos, modelData[i].bvo.hitPos, axis.xyz());
                glm::mat4 projectorProjection = glm::ortho(-projectorSize, projectorSize, -proportionateHeight, proportionateHeight, 1e-5f, FAR_PLANE);

                modelData[i].bvo.decalProjector = projectorProjection * projectorView;
                if (debug)
                {
                    printf("Hit Point: %s\n", glm::to_string(modelData[i].bvo.hitPos).c_str());
                    printf("Hit Normal: %s\n", glm::to_string(modelData[i].bvo.hitNor).c_str());
                    printf("Decal Projector: %s\n", glm::to_string(modelData[i].bvo.decalProjector).c_str());
                }
            }
        }
    }
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

void ClearModelVertexData(ModelData& modelData)
{
    modelData.vertices.clear();
    modelData.normals.clear();
    modelData.textureCoordinates.clear();
    modelData.tangents.clear();
    modelData.indexes.clear();
    modelData.Bbox.bboxMin = glm::vec3(1e+5);
    modelData.Bbox.bboxMax = glm::vec3(-1e+5);
    modelData.Bbox.centroid = glm::vec3(0.0f);
    modelData.modelMatrix = glm::mat4(1.0f);
    modelData.bvo.isTracing = false;
    modelData.bvo.decalProjector = glm::mat4(1.0f);
}

void ClearModelsVertexData(std::vector<ModelData>& modelData)
{
    for (uint16_t i = 0u; i < modelData.size(); ++i)
    {
        ClearModelVertexData(modelData[i]);
    }
}
//void ComputeTangents(std::vector<ModelData>& modelData);

// https://learnopengl.com/code_viewer_gh.php?code=src/6.pbr/1.2.lighting_textured/lighting_textured.cpp
void renderSphere(ModelData& modelData)
{
    ClearModelVertexData(modelData);

    const unsigned int X_SEGMENTS = 256;
    const unsigned int Y_SEGMENTS = 256;
    const float PI = 3.14159265359f;
    const float T_PI = PI * 2.0f;

    glm::vec2 xySegments = glm::vec2(X_SEGMENTS, Y_SEGMENTS);
    //unsigned int idx = 0;
    for (unsigned int u = 0; u <= X_SEGMENTS; ++u)
    {
        for (unsigned int v = 0; v <= Y_SEGMENTS; ++v)
        {

            glm::vec2 uv = glm::vec2(u, v) / xySegments;

            float sinPIV = sin(PI * uv.y);
            float cosTwoPIUTimesSinPIV = cos(T_PI * uv.x) * sinPIV;
            float sinTwoPIU = sin(T_PI * uv.x);
            float sinTwoPIUTimesSinPIV = sinTwoPIU * sinPIV;
                
            glm::vec3 pos = glm::vec3(cosTwoPIUTimesSinPIV,
                                        cos(PI * uv.y),
                                        sinTwoPIUTimesSinPIV);
                
            glm::vec3 nor = glm::normalize(pos);
                
            glm::vec4 tan = glm::vec4(-T_PI * sinTwoPIUTimesSinPIV,
                                        0.0,                        
                                        T_PI * cosTwoPIUTimesSinPIV,
                                        1.0);

            modelData.Bbox.bboxMax = glm::max(modelData.Bbox.bboxMax, pos);
            modelData.Bbox.bboxMin = glm::min(modelData.Bbox.bboxMin, pos);
                
            modelData.textureCoordinates.push_back(uv);
            modelData.vertices.push_back(pos);
            modelData.normals.push_back(nor);
            modelData.tangents.push_back(tan);
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
    {
        if (!oddRow) // even rows: y == 0, y == 2; and so on
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                modelData.indexes.push_back(y * (X_SEGMENTS + 1) + x);
                modelData.indexes.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        else
        {
            for (int x = X_SEGMENTS; x >= 0; --x)
            {
                modelData.indexes.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                modelData.indexes.push_back(y * (X_SEGMENTS + 1) + x);
            }
        }
        oddRow = !oddRow;
    }
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

// https://web.archive.org/web/20110708081637/http://www.terathon.com/code/tangent.html
void ComputeTangents(ModelData& modelData)
{
    modelData.tangents.clear();
    modelData.tangents.resize(modelData.indexes.size(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

    std::vector<glm::vec3> tan1(modelData.vertices.size()),
                           tan2(modelData.vertices.size());
    for (unsigned int i = 0; i < modelData.indexes.size(); i += 3)
    {
        unsigned int idx0 = modelData.indexes[i];
        unsigned int idx1 = modelData.indexes[i+1];
        unsigned int idx2 = modelData.indexes[i+2];

        glm::vec3 vertex0 = modelData.vertices[idx0];
        glm::vec3 vertex1 = modelData.vertices[idx1];
        glm::vec3 vertex2 = modelData.vertices[idx2];

        glm::vec2 uv0 = modelData.textureCoordinates[idx0];
        glm::vec2 uv1 = modelData.textureCoordinates[idx1];
        glm::vec2 uv2 = modelData.textureCoordinates[idx2];

        float x1 = vertex1.x - vertex0.x;
        float x2 = vertex2.x - vertex0.x;
        float y1 = vertex1.y - vertex0.y;
        float y2 = vertex2.y - vertex0.y;
        float z1 = vertex1.z - vertex0.z;
        float z2 = vertex2.z - vertex0.z;

        float s1 = uv1.x - uv0.x;
        float s2 = uv2.x - uv0.x;
        float t1 = uv1.y - uv0.y;
        float t2 = uv2.y - uv0.y;

        float r = 1.0f / (s1 * t2 - s2 * t1);
        glm::vec3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
            (t2 * z1 - t1 * z2) * r);
        glm::vec3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
            (s1 * z2 - s2 * z1) * r);

        tan1[idx0] += sdir;
        tan1[idx1] += sdir;
        tan1[idx2] += sdir;

        tan2[idx0] += tdir;
        tan2[idx1] += tdir;
        tan2[idx2] += tdir;
    }
    for (long a = 0; a < modelData.vertices.size(); a++)
    {
        const glm::vec3& n = modelData.vertices[a];
        const glm::vec3& t = tan1[a];

        // Gram-Schmidt orthogonalize
        glm::vec3 tangent = glm::normalize(t - n * glm::dot(n, t));
        modelData.tangents[a].x = tangent.x; 
        modelData.tangents[a].y = tangent.y; 
        modelData.tangents[a].z = tangent.z; 

        // Calculate handedness
        modelData.tangents[a].w = (glm::dot(glm::cross(n, t), tan2[a]) < 0.0f) ? -1.0f : 1.0f;
    }
    tan1.clear();
    tan2.clear();
}

void ObjLoader(std::string inputFile, std::vector<ModelData>& modelData)
{
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

	uint16_t meshNumber = 0;
    uint32_t counter = 0;
    modelData.resize(shapes.size());
    for (const auto& shape : shapes)
    {
		ClearModelVertexData(modelData[meshNumber]);
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
            // Fix this.
            if (true)
            {
                modelData[meshNumber].vertices.push_back(position);
                modelData[meshNumber].normals.push_back(normal);
                modelData[meshNumber].textureCoordinates.push_back(textureCoordinates);
                // BBox
                modelData[meshNumber].Bbox.bboxMax = glm::max(modelData[meshNumber].Bbox.bboxMax, position);
                modelData[meshNumber].Bbox.bboxMin = glm::min(modelData[meshNumber].Bbox.bboxMin, position);
                modelData[meshNumber].indexes.push_back(counter);
                counter++;
            }
            else
            {
                auto hash = VertexBitHash(&position, &normal, &textureCoordinates);
                if (uniqueVertices.count(hash) == 0)
                {
                    modelData[meshNumber].vertices.push_back(position);
                    modelData[meshNumber].normals.push_back(normal);
                    modelData[meshNumber].textureCoordinates.push_back(textureCoordinates);
                    // BBox
                    modelData[meshNumber].Bbox.bboxMax = glm::max(modelData[meshNumber].Bbox.bboxMax, position);
                    modelData[meshNumber].Bbox.bboxMin = glm::min(modelData[meshNumber].Bbox.bboxMin, position);
                    modelData[meshNumber].indexes.push_back(counter);
                    uniqueVertices[hash] = counter;
                    ++counter;
                }
                else
                {
                    modelData[meshNumber].indexes.push_back(uniqueVertices[hash]);
                }
            }
        }
        modelData[meshNumber].Bbox.centroid = (modelData[meshNumber].Bbox.bboxMin + modelData[meshNumber].Bbox.bboxMax) * 0.5f;
		ComputeTangents(modelData[meshNumber]);
        meshNumber++;
    }
#ifdef OPTIMIZE
#else
	printModelData(modelData);
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

void loadGLTF(tinygltf::Model &model, std::vector<ModelData>& modelData) 
{
    std::unordered_map<std::bitset<256>, uint32_t> uniqueVertices;
    uint32_t counter = 0;
    int primCounter = 0;

    std::vector<unsigned int> _u32Buffer;
    std::vector<unsigned short> _u16Buffer;
    std::vector<unsigned char> _u8Buffer;

	modelData.resize(model.meshes.size());

	uint16_t meshNumber = 0;

    for (auto &mesh : model.meshes) 
    {
        ClearModelVertexData(modelData[meshNumber]);
#ifdef OPTIMIZE
#else
        std::cout << "mesh : " << mesh.name << std::endl;
#endif
        for (const auto& prim : mesh.primitives)
        {

            bool result = GLTF::GetAttributes<glm::vec3>(model, prim, modelData[meshNumber].vertices,           "POSITION");
                 result = GLTF::GetAttributes<glm::vec3>(model, prim, modelData[meshNumber].normals,            "NORMAL");
                 result = GLTF::GetAttributes<glm::vec2>(model, prim, modelData[meshNumber].textureCoordinates, "TEXCOORD_0");
                 result = GLTF::GetAttributes<glm::vec4>(model, prim, modelData[meshNumber].tangents,           "TANGENT");

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
                    modelData[meshNumber].indexes.insert(modelData[meshNumber].indexes.end(), std::make_move_iterator(_u32Buffer.begin()), std::make_move_iterator(_u32Buffer.end()));
                    break;
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                    _u16Buffer.resize(indexAccessor.count);
                    std::memcpy(&_u16Buffer[0], &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset], indexAccessor.count * sizeof(unsigned short));
                    modelData[meshNumber].indexes.insert(modelData[meshNumber].indexes.end(), std::make_move_iterator(_u16Buffer.begin()), std::make_move_iterator(_u16Buffer.end()));
                    break;
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                    _u8Buffer.resize(indexAccessor.count);
                    std::memcpy(&_u8Buffer[0], &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset], indexAccessor.count * sizeof(unsigned char));
                    modelData[meshNumber].indexes.insert(modelData[meshNumber].indexes.end(), std::make_move_iterator(_u8Buffer.begin()), std::make_move_iterator(_u8Buffer.end()));
                    break;
                default:
                    std::cerr << "Unknown index component type : " << indexAccessor.componentType << " is not supported" << std::endl;
                    return;
                }
            }
            else
            {
                std::cout << "Primitive without indices, creating them" << std::endl;
                const auto& accessor = model.accessors[prim.attributes.find("POSITION")->second];
                for (unsigned int i = 0; i < accessor.count; ++i)
                {
                    modelData[meshNumber].indexes.push_back(i);
                }
            }
        }
        if (modelData[meshNumber].tangents.size() == 0)
        {
            std::cout << "No tangents, computing them!" << std::endl;
            ComputeTangents(modelData[meshNumber]);
        }
        meshNumber += 1u;
    }

    

#ifdef OPTIMIZE
#else
	printModelData(modelData);
#endif
	// Compute bounding boxes.
    for (int i = 0; i < modelData.size(); ++i)
    {
        for (int j = 0; j < modelData[i].vertices.size(); ++j)
        {
            glm::vec3 vertex = modelData[i].vertices[j];
            modelData[i].Bbox.bboxMin = glm::min(vertex, modelData[i].Bbox.bboxMin);
            modelData[i].Bbox.bboxMax = glm::max(vertex, modelData[i].Bbox.bboxMax);
        }
        modelData[i].Bbox.centroid = (modelData[i].Bbox.bboxMin + modelData[i].Bbox.bboxMax) * 0.5f;
    }
    
    /*std::cout << "BboxMax: {x: " << modelData.Bbox.bboxMax.x << ", y: " << modelData.Bbox.bboxMax.y << ", z: " << modelData.Bbox.bboxMax.z << "}\n";
    std::cout << "BboxMin: {x: " << modelData.Bbox.bboxMin.x << ", y: " << modelData.Bbox.bboxMin.y << ", z: " << modelData.Bbox.bboxMin.z << "}\n";*/

    _u8Buffer.clear();
    _u16Buffer.clear();
    _u32Buffer.clear();

    flipAlbedo = 1;
}

void CreateBOs(ModelData& modelData)
{
    std::vector<float> data;
    for (unsigned int i = 0; i < modelData.vertices.size(); ++i)
    {
        data.push_back(modelData.vertices[i].x);
        data.push_back(modelData.vertices[i].y);
        data.push_back(modelData.vertices[i].z);
        if (modelData.normals.size() > 0)
        {
            data.push_back(modelData.normals[i].x);
            data.push_back(modelData.normals[i].y);
            data.push_back(modelData.normals[i].z);
        }
        if (modelData.textureCoordinates.size() > 0)
        {
            data.push_back(modelData.textureCoordinates[i].x);
            data.push_back(modelData.textureCoordinates[i].y);
        }
        if (modelData.tangents.size() > 0)
        {
            data.push_back(modelData.tangents[i].x);
            data.push_back(modelData.tangents[i].y);
            data.push_back(modelData.tangents[i].z);
            data.push_back(modelData.tangents[i].w);
        }
    }

    glGenVertexArrays(1, &(modelData.openGLObject.VAO));
    glGenBuffers(1, &(modelData.openGLObject.VBO));
    glGenBuffers(1, &(modelData.openGLObject.EBO));

    glBindVertexArray(modelData.openGLObject.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, modelData.openGLObject.VBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelData.openGLObject.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, modelData.indexes.size() * sizeof(unsigned int), &modelData.indexes[0], GL_STATIC_DRAW);
    unsigned int stride = (3 + 3 + 2 + 4) * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
}

void CreateBOs(std::vector<ModelData>& modelData)
{
    for (uint16_t i = 0u; i < modelData.size(); ++i)
    {
        CreateBOs(modelData[i]);
    }
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
    bool ret = modelData.bvo.accel.Build((unsigned int)modelData.indexes.size() / 3u, triangle_mesh, triangle_pred, build_options);
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

void BuildBVH(std::vector<ModelData>& modelData)
{
    for (uint16_t i = 0u; i < modelData.size(); ++i)
    {
        BuildBVH(modelData[i]);
    }
}

float computeBiasDepthComparison(const ModelData& modelData, const float& scale = 0.00025)
{
    return fabs(modelData.Bbox.bboxMax.z - modelData.Bbox.bboxMin.z) * scale;
}

void reloadModel(ModelData& modelData)
{
    BuildBVH(modelData);
    CreateBOs(modelData);
    float biasDepthComparison = computeBiasDepthComparison(modelData, depthBias);
    decalsPass.setFloat("bias", biasDepthComparison);
    //std::cout << "Recomputed BBox Center: {x: " << modelData.Bbox.centroid.x << ", y: " << modelData.Bbox.centroid.y << ", z:" << modelData.Bbox.centroid.z << "}\n";
}

void reloadModel(std::vector<ModelData>& modelData)
{
    for (auto model : modelData)
    {
        reloadModel(model);
    }
}

void recomputeDecalBaseColorTexture(Shader& shader, const Shader::TEXTURE_WRAP_PARAMS& wrapParam, const Shader::TEXTURE_SAMPLE_PARAMS& sampleParam)
{
    //decalsPass.createTexture(&decalTexture, "Assets/Textures/WatchMen.jpeg", "iChannel0", 1);
    //decalsPass.createTextureFromFile(&decalTexture, decalAlbedoImageBuffer, widthDecal, heightDecal, "iChannel0", 1);
#ifdef OPTIMIZE
#else
    std::cout << "Changing texture" << std::endl;
#endif
    flip = 1;
    glGenTextures(1, &(modelData[0].material.decalBaseColor));
    glBindTexture(GL_TEXTURE_2D, modelData[0].material.decalBaseColor);

    shader.textureWrap(wrapParam);
    shader.textureSample(sampleParam);
    /*
    // In an ideal world this should be exposed as input params to the function.
    // Texture wrapping params.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Texture filtering params.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/

    // Get the texture format automatically.
    auto format = GL_RGBA;    
    glTexImage2D(GL_TEXTURE_2D, 0, format, decalsPass.Width, decalsPass.Height, 0, format, GL_UNSIGNED_BYTE, decalAlbedoImageBuffer.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    // Clear the data.
    decalAlbedoImageBuffer.clear();

    // Bind the uniform sampler.
    decalsPass.use();
    //decalsPass.setInt("iChannel0", 1);

    // Update decal through triggering the ray trace.
    updateViewMatrix();
    mousePositionX = WIDTH / 2;
    mousePositionY = HEIGHT / 2;
    rayTrace(mousePositionX + (splitScreen ? WIDTH / 4 : 0), mousePositionY, widthHeight, 
             /* In and Out */ modelData, 
             projection, view, false);
}

void recomputeDecalNormalTexture(Shader& shader, const Shader::TEXTURE_WRAP_PARAMS& wrapParam, const Shader::TEXTURE_SAMPLE_PARAMS& sampleParam)
{
    //decalsPass.createTexture(&decalTexture, "Assets/Textures/WatchMen.jpeg", "iChannel0", 1);
    //decalsPass.createTextureFromFile(&decalTexture, decalAlbedoImageBuffer, widthDecal, heightDecal, "iChannel0", 1);
#ifdef OPTIMIZE
#else
    std::cout << "Changing texture" << std::endl;
#endif
    flip = 1;
    glGenTextures(1, &(modelData[0].material.decalNormal));
    glBindTexture(GL_TEXTURE_2D, modelData[0].material.decalNormal);

    shader.textureWrap(wrapParam);
    shader.textureSample(sampleParam);
    /*
    // In an ideal world this should be exposed as input params to the function.
    // Texture wrapping params.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Texture filtering params.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/

    // Get the texture format automatically.
    auto format = GL_RGBA;    
    glTexImage2D(GL_TEXTURE_2D, 0, format, decalsPass.Width, decalsPass.Height, 0, format, GL_UNSIGNED_BYTE, decalNormalImageBuffer.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    // Clear the data.
    decalNormalImageBuffer.clear();

    // Bind the uniform sampler.
    decalsPass.use();
    //decalsPass.setInt("iChannel3", 5);

    // Update decal through triggering the ray trace.
    updateViewMatrix();
    mousePositionX = WIDTH / 2;
    mousePositionY = HEIGHT / 2;
    rayTrace(mousePositionX + (splitScreen ? WIDTH / 4 : 0), mousePositionY, widthHeight, 
             /* In and Out */ modelData, 
             projection, view, false);
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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
    
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
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
#ifndef __EMSCRIPTEN__
    context = SDL_GL_CreateContext(window);
    if (!context) 
    {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return false;
    }
#else
    // Skip SDL_GL_CreateContext. Manually create WebGL2 context instead.
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = true;
    attrs.depth = true;
    attrs.stencil = false;
    attrs.antialias = false;
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    attrs.enableExtensionsByDefault = true;

    ctx = emscripten_webgl_create_context("#canvas", &attrs);
    if (ctx <= 0) {
        printf("Failed to create WebGL2 context.\n");
        return 1;
    }

    emscripten_webgl_make_context_current(ctx);
    // Activate WebGL extensions.
    std::array<const char*, 5> webglExtensions;
    webglExtensions[0] = "OES_texture_float";
    webglExtensions[1] = "EXT_color_buffer_float";
    webglExtensions[2] = "WEBGL_color_buffer_float";
    webglExtensions[3] = "OES_texture_half_float";
    webglExtensions[4] = "EXT_color_buffer_half_float"; 
    for (uint8_t i = 0u; i < (uint8_t)webglExtensions.size(); ++i)
    {
        EMSCRIPTEN_RESULT result = emscripten_webgl_enable_extension(ctx, webglExtensions[i]);
        if (result == EMSCRIPTEN_RESULT_SUCCESS)
        {
            std::cout << "Enabled the " << webglExtensions[i] << " extension!" << std::endl;
        }
    }
#endif

    // Print OpenGL version
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GL Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "GL Renderer: " << glGetString(GL_RENDERER) << std::endl;
#ifdef __EMSCRIPTEN__
    std::cout << "GL Extensions: " << glGetString(GL_EXTENSIONS) << std::endl;
#endif

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
#ifndef __EMSCRIPTEN__
    ImGui_ImplSDL2_InitForOpenGL(window, context);
#else
    SDL_GLContext fake_context = (SDL_GLContext)1; // Dummy non-null pointer
    ImGui_ImplSDL2_InitForOpenGL(window, fake_context);
#endif
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

    std::vector<std::string> JFAScreenPassAttributeLocations(2);
    JFAScreenPassAttributeLocations[0] = vertexPositionAttributeQualifierName;
    JFAScreenPassAttributeLocations[1] = texCoordsPositionAttributeQualiferName;

    std::vector<std::string> fullScreenPassAttributeLocations(2);
    fullScreenPassAttributeLocations[0] = vertexPositionAttributeQualifierName;
    fullScreenPassAttributeLocations[1] = texCoordsPositionAttributeQualiferName;
    
    std::vector<std::string> SDFScreenPassAttributeLocations(2);
    SDFScreenPassAttributeLocations[0] = vertexPositionAttributeQualifierName;
    SDFScreenPassAttributeLocations[1] = texCoordsPositionAttributeQualiferName;
#ifdef __EMSCRIPTEN__
    depthPrePass   = Shader("shaders/DBuffer.vert",      "shaders/DBuffer.frag",      depthPrePassAttributeLocations,   GLSLVersion);
    geometryPass   = Shader("shaders/GBuffer.vert",      "shaders/GBuffer.frag",      geometryPassAttributeLocations,   GLSLVersion);
    deferredPass   = Shader("shaders/DeferredPass.vert", "shaders/DeferredPass.frag", deferredPassAttributeLocations,   GLSLVersion);
    hitPosition    = Shader("shaders/HitPosition.vert",  "shaders/HitPosition.frag",  hitPositionAttributeLocations,    GLSLVersion);
    decalsPass     = Shader("shaders/Decals.vert",       "shaders/Decals.frag",       decalsPassAttributeLocations,     GLSLVersion);
    JFAPass        = Shader("shaders/JFA.vert",          "shaders/JFA.frag",          JFAScreenPassAttributeLocations,  GLSLVersion);
    SDFPass        = Shader("shaders/SDF.vert",          "shaders/SDF.frag",              SDFScreenPassAttributeLocations,  GLSLVersion);
    fullScreenPass = Shader("shaders/FullScreen.vert",   "shaders/FullScreen.frag",   fullScreenPassAttributeLocations, GLSLVersion);
#else
    depthPrePass   = Shader("../shaders/DBuffer.vert",      "../shaders/DBuffer.frag",      depthPrePassAttributeLocations,   GLSLVersion);
    geometryPass   = Shader("../shaders/GBuffer.vert",      "../shaders/GBuffer.frag",      geometryPassAttributeLocations,   GLSLVersion);
    deferredPass   = Shader("../shaders/DeferredPass.vert", "../shaders/DeferredPass.frag", deferredPassAttributeLocations,   GLSLVersion);
    hitPosition    = Shader("../shaders/HitPosition.vert",  "../shaders/HitPosition.frag",  hitPositionAttributeLocations,    GLSLVersion);
    decalsPass     = Shader("../shaders/Decals.vert",       "../shaders/Decals.frag",       decalsPassAttributeLocations,     GLSLVersion);
    JFAPass        = Shader("../shaders/JFA.vert",          "../shaders/JFA.frag",          JFAScreenPassAttributeLocations,  GLSLVersion);
    SDFPass        = Shader("../shaders/SDF.vert",          "../shaders/SDF.frag",              SDFScreenPassAttributeLocations,  GLSLVersion);
    fullScreenPass = Shader("../shaders/FullScreen.vert",   "../shaders/FullScreen.frag",   fullScreenPassAttributeLocations, GLSLVersion);
#endif
}

void createAndAttachDepthPrePassRbo(frameBuffer& depthFramebuffer)
{
    /** Start Depth Buffer **/
    glGenFramebuffers(1, &(depthFramebuffer.framebuffer));
    glBindFramebuffer(GL_FRAMEBUFFER, depthFramebuffer.framebuffer);

    depthPrePass.use();

    // create and attach depth buffer (renderbuffer)
    depthFramebuffer.textures = std::vector<unsigned int>(1);

    glGenTextures(1, &(depthFramebuffer.textures[0]));
    glBindTexture(GL_TEXTURE_2D, depthFramebuffer.textures[0]);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, WIDTH / DEPTH_FACTOR, HEIGHT / DEPTH_FACTOR, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthFramebuffer.textures[0], 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
    {
        std::cerr << "Depth Framebuffer configuration failed" << std::endl;
    }

    unsigned int attachments[1] = {GL_NONE};
    glDrawBuffers(1, attachments);
    std::cout << "Depth frame buffer: framebuffer: " << depthFramebuffer.framebuffer << " texture: " << depthFramebuffer.textures[0] << std::endl;
    /** End Depth Buffer **/
}

void createAndAttachTextureSpaceRbo(const ModelData& modelData, Shader& shader, frameBuffer& textureSpaceFramebuffer, const Shader::TEXTURE_WRAP_PARAMS& wrapParam, const Shader::TEXTURE_SAMPLE_PARAMS& sampleParam)
{
    /** Start Texture Space Buffer **/
    glGenFramebuffers(1, &(textureSpaceFramebuffer.framebuffer));
    glBindFramebuffer(GL_FRAMEBUFFER, textureSpaceFramebuffer.framebuffer);

    shader.use();
#ifdef OPTIMIZE
#else
    std::cout << "Texture Space Width: "  << shader.Width  << std::endl;
    std::cout << "Texture Space Height: " << shader.Height << std::endl;
#endif

    textureSpaceFramebuffer.textures = std::vector<unsigned int>(4);
    unsigned int drawBuffersFBO[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

    int mipLevels = (int)(std::floor(std::log2(std::max(shader.Width, shader.Height)))) + 1;

    for (uint8_t i = 0u; i < (uint8_t)textureSpaceFramebuffer.textures.size(); ++i)
    {
        glGenTextures(1, &(textureSpaceFramebuffer.textures[i]));
        glBindTexture(GL_TEXTURE_2D, textureSpaceFramebuffer.textures[i]);
        
        int mipWidth = shader.Width,
            mipHeight = shader.Height; 
        for (uint16_t j = 0u; j < (uint16_t)mipLevels; ++j)
        {
            glTexImage2D(GL_TEXTURE_2D, j, modelData.material.channels[i], mipWidth, mipHeight, 0, modelData.material.channels[i], GL_UNSIGNED_BYTE, NULL);
            
            mipWidth  = std::max(1, mipWidth  / 2);
            mipHeight = std::max(1, mipHeight / 2);
			std::cout << "Mip level: " << j << " Width: " << mipWidth << " Height: " << mipHeight << std::endl;
        }
        shader.textureWrap(wrapParam);
        shader.textureSample(sampleParam);

        glFramebufferTexture2D(GL_FRAMEBUFFER, drawBuffersFBO[i], GL_TEXTURE_2D, textureSpaceFramebuffer.textures[i], 0);
    }

    // Set the list of draw buffers.
    glDrawBuffers(4, drawBuffersFBO); // "1" is the size of DrawBuffers

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Texture Space Framebuffer is complete!" << std::endl;
    }
    else
    {
        std::cerr << "Texture Space Framebuffer is not complete. Status: " << status << std::endl;
        // Handle incomplete framebuffer (e.g., log more details, exit)
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    deferredPass.use();
    deferredPass.setInt("gAlbedo",    0);
    deferredPass.setInt("gNormal",    1);
    deferredPass.setInt("gMetallic",  2);
    deferredPass.setInt("gRoughness", 3);
    // deferredPass.setInt("gAO",        4);
    /** End Texture Space Buffer **/
    //std::cout << "Texture space frame buffer: framebuffer: " << textureSpaceFramebuffer.framebuffer << " albedo texture: " << textureSpaceFramebuffer.textures[0] << " normal texture: " << textureSpaceFramebuffer.textures[1] << std::endl;
}

void createAndAttachRboJFA(frameBuffer& frameBuffer, const GLsizei& sizeX, const GLsizei& sizeY, const FrameBufferTextureParams& frameBufferTextureParams)
{
    /*
     * Start JFA Render Buffer Object
     */
    glGenFramebuffers(1, &(frameBuffer.framebuffer));
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.framebuffer);

    frameBuffer.textures = std::vector<unsigned int>(1);
    glGenTextures(1, &(frameBuffer.textures[0]));
    glBindTexture(GL_TEXTURE_2D, frameBuffer.textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, frameBufferTextureParams.INTERNAL_FORMAT, sizeX, sizeY, 0, frameBufferTextureParams.FORMAT, frameBufferTextureParams.TYPE, NULL);
    // Texture wrapping params.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Texture filtering params.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameBuffer.textures[0], 0);

    // Set the list of draw buffers.
    unsigned int drawBuffersFBO[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffersFBO);

    int frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (frameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "JFA Framebuffer not complete! Code: " << std::hex << frameBufferStatus << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::cout << "JFA Frame buffer: framebuffer: " << frameBuffer.framebuffer << " texture: " << frameBuffer.textures[0] << std::endl;
    /*
     * End JFA Render Buffer Object
     */
}

void createAndAttachSDFFramebuffer(frameBuffer& frameBuffer, const GLsizei& sizeX, const GLsizei& sizeY, const FrameBufferTextureParams& frameBufferTextureParams)
{
    /*
     * Start SDF Render Buffer Object
     */
    glGenFramebuffers(1, &(frameBuffer.framebuffer));
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.framebuffer);
    
    frameBuffer.textures = std::vector<unsigned int>(1);
    glGenTextures(1, &(frameBuffer.textures[0]));
    glBindTexture(GL_TEXTURE_2D, frameBuffer.textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, frameBufferTextureParams.INTERNAL_FORMAT, sizeX, sizeY, 0, frameBufferTextureParams.FORMAT, frameBufferTextureParams.TYPE, NULL);

    // Texture wrapping params.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Texture filtering params.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameBuffer.textures[0], 0);
    
    // Set the list of draw buffers.
    unsigned int drawBuffersFBO[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffersFBO);
    
    int frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (frameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "SDF Framebuffer not complete! Code: " << std::hex << frameBufferStatus << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    std::cout << "SDF Frame buffer: framebuffer: " << frameBuffer.framebuffer << " texture: " << frameBuffer.textures[0] << std::endl;
    /*
     * End SDF Render Buffer Object
     */
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

struct Light
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::mat4 model    = glm::mat4(1.0f);
    glm::vec3 color    = glm::vec3(1.0f);
    float speed = 0.2f;
    bool toggle = true;
};
Light light;

void readModelsPreload(std::vector<ModelData>& modelData, std::vector<std::vector<ModelData>>& modelsData, 
                       const std::vector<ModelFileNames>& fileNames, const int& pick)
{
    for (uint8_t i = 0u; i < fileNames.size() - 1; ++i)
    {
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

            for (uint16_t j = 0u; j < modelData.size(); ++j);
            {
                ClearModelsVertexData(modelsData[i]);
                loadGLTF(modelGLTF, modelsData[i]);
			}
            
        }
        else if (fileType == "obj")
        {
            std::string fileString = fileToString(fileNames[i].mesh);
            for (uint16_t j = 0u; j < modelsData.size(); ++j)
            {
                ClearModelsVertexData(modelsData[i]);
                ObjLoader(fileString, modelsData[i]);
			}
        }
    }

    // Procedural Sphere.
    size_t lastIndex = fileNames.size() - 1;
    modelsData[lastIndex].resize(1);
    renderSphere(modelsData[lastIndex][0]);
    // Build the acceleration structure for ray tracing.
    for (uint16_t i = 0u; i < modelsData.size(); ++i)
    {
        BuildBVH(modelsData[i]);
    }

    ClearModelsVertexData(modelData);
    modelData = modelsData[pick];
}

enum MaterialTextureType
{
    BASE_COLOR,
    NORMAL,
    METAL,
    ROUGH,
    AO,
    DECAL_BASE_COLOR,
    DECAL_NORMAL
};

void regenerateTexture(Shader& shader, ModelData& modelData, const MaterialTextureType& materialTextureType, const std::string& fileName, frameBuffer& framebuffer)
{
    shader.use();
    int textureTypeInt = (int)materialTextureType;
    bool isFramebuffer = false;

    std::string textureType = "";
    unsigned int* materialType = {};
    isGLTF = false;
    switch (textureTypeInt)
    {
        // BaseColor
    case 0:
    {
        textureType = "iAlbedo";
        materialType = &(modelData.material.baseColor);
        isFramebuffer = true;
        break;
    }
    // Normal
    case 1:
    {
        textureType = "iNormal";
        materialType = &(modelData.material.normal);
        isFramebuffer = true;
        break;
    }
    // Metallic
    case 2:
    {
        textureType = "iMetallic";
        materialType = &(modelData.material.metallic);
        isFramebuffer = true;
        break;
    }
    // Roughness
    case 3:
    {
        textureType = "iRoughness";
        materialType = &(modelData.material.roughness);
        isFramebuffer = true;
        break;
    }
    case 4:
    {
        textureType = "AO";
        materialType = &(modelData.material.ao);
        break;
    }
    case 5:
    {
        textureType = "iDecalAlbedo";
        materialType = &(modelData.material.decalBaseColor);
        isFramebuffer = false;
        break;
    }
    case 6:
    {
        textureType = "iDecalNormal";
        materialType = &(modelData.material.decalNormal);
        isFramebuffer = false;
        break;
    }
    default:
    {
        break;
    }
    }
    //std::cout << "Material: " << textureType << " is FrameBuffer?: " << (isFramebuffer ? "True" : "False") << " Texture type int: " << textureTypeInt << std::endl;
    glDeleteTextures(1, materialType);

    Shader::TEXTURE_WRAP_PARAMS wrapParams = Shader::CLAMP_TO_EDGE;
    Shader::TEXTURE_SAMPLE_PARAMS sampleParams = Shader::LINEAR_MIPS;
    
    if (isFramebuffer)
    {
        shader.createTexture(materialType, fileName, wrapParams, sampleParams, textureType, textureTypeInt, modelData.material.channels[textureTypeInt]);
        std::cout << "Channels: " << modelData.material.channels[textureTypeInt] << std::endl;

        FrameBufferTextureParams fboTextureParams = { modelData.material.channels[textureTypeInt], modelData.material.channels[textureTypeInt], GL_UNSIGNED_BYTE };
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.framebuffer);
        regenerateFramebufferTexture(framebuffer, fboTextureParams, shader, wrapParams, sampleParams, textureTypeInt);
    }
    else
    {
        textureTypeInt -= 1;
        shader.createTexture(materialType, fileName, wrapParams, sampleParams, textureType, textureTypeInt, modelData.material.channels[textureTypeInt]);
    }
}

int main()
{
#ifdef __EMSCRIPTEN__
    fileNames[0].mesh = "Assets/Pilot/source/PilotShirtDraco.glb";
    fileNames[0].baseColor = "Assets/Pilot/textures/T_DefaultMaterial_B_1k.jpg";
    fileNames[0].normal = "Assets/Pilot/textures/T_DefaultMaterial_N_1k.jpg";
    fileNames[0].metallic = "";//Assets/Sphere/rustediron2_metallic.png";
    fileNames[0].roughness = "";//Assets/Sphere/rustediron2_roughness.png";
    fileNames[0].decalBaseColor = "Assets/Textures/Watchmen.png";
    fileNames[0].decalNormal = "Assets/Textures/Watchmen_normal.png";

    fileNames[1].mesh = "Assets/CaterpillarWorkboot/source/sh_catWorkBoot_draco.glb";
    fileNames[1].baseColor = "Assets/CaterpillarWorkboot/textures/sh_catWorkBoot_albedo.jpg";
    fileNames[1].normal = "Assets/CaterpillarWorkboot/textures/sh_catWorkBoot_nrm.jpg";
    fileNames[1].metallic = "Assets/CaterpillarWorkboot/textures/sh_catWorkBoot_specular.jpg";
    fileNames[1].roughness = "Assets/CaterpillarWorkboot/textures/sh_catWorkBoot_rough.jpg";
    fileNames[1].decalBaseColor = "Assets/Textures/Watchmen_2.png";
    fileNames[1].decalNormal = "Assets/Textures/Watchmen_2_normal.png";

    fileNames[2].mesh = "";
    fileNames[2].baseColor = "Assets/Sphere/rustediron2_basecolor.png";
    fileNames[2].normal = "Assets/Sphere/rustediron2_normal.png";
    fileNames[2].metallic = "Assets/Sphere/rustediron2_metallic.png";
    fileNames[2].roughness = "Assets/Sphere/rustediron2_roughness.png";
    fileNames[2].decalBaseColor = "Assets/Textures/Watchmen.png";
    fileNames[2].decalNormal = "Assets/Textures/Watchmen_normal.png";
#else
    fileNames[0].mesh = "../Assets/Pilot/source/PilotShirtDraco.glb";
    fileNames[0].baseColor = "../Assets/Pilot/textures/T_DefaultMaterial_B_1k.jpg";
    fileNames[0].normal = "../Assets/Pilot/textures/T_DefaultMaterial_N_1k.jpg";
    fileNames[0].metallic = "../Assets/Sphere/rustediron2_metallic.png";
    fileNames[0].roughness = "../Assets/Sphere/rustediron2_roughness.png";
    fileNames[0].decalBaseColor = "../Assets/Textures/Watchmen.png";
    fileNames[0].decalNormal = "../Assets/Textures/Watchmen_normal.png";

    fileNames[1].mesh = "../Assets/CaterpillarWorkboot/source/sh_catWorkBoot_draco.glb";
    fileNames[1].baseColor = "../Assets/CaterpillarWorkboot/textures/sh_catWorkBoot_albedo.jpg";
    fileNames[1].normal = "../Assets/CaterpillarWorkboot/textures/sh_catWorkBoot_nrm.jpg";
    fileNames[1].metallic = "../Assets/CaterpillarWorkboot/textures/sh_catWorkBoot_specular.jpg";
    fileNames[1].roughness = "../Assets/CaterpillarWorkboot/textures/sh_catWorkBoot_rough.jpg";
    fileNames[1].decalBaseColor = "../Assets/Textures/Watchmen_2.png";
    fileNames[1].decalNormal = "../Assets/Textures/Watchmen_2_normal.png";

    // https://sketchfab.com/3d-models/real-army-jacket-730e9269cdb647f1898b2b87190d6b4e
    fileNames[2].mesh = "../Assets/Jacket/real_army_jacket.glb";
    fileNames[2].baseColor = "../Assets/Sphere/rustediron2_basecolor.png";
    fileNames[2].normal = "../Assets/Sphere/rustediron2_normal.png";
    fileNames[2].metallic = "../Assets/Sphere/rustediron2_metallic.png";
    fileNames[2].roughness = "../Assets/Sphere/rustediron2_roughness.png";
    fileNames[2].decalBaseColor = "../Assets/Textures/Watchmen.png";
    fileNames[2].decalNormal = "../Assets/Textures/Watchmen_normal.png";

    fileNames[3].mesh = "";
    fileNames[3].baseColor = "../Assets/Sphere/rustediron2_basecolor.png";
    fileNames[3].normal = "../Assets/Sphere/rustediron2_normal.png";
    fileNames[3].metallic = "../Assets/Sphere/rustediron2_metallic.png";
    fileNames[3].roughness = "../Assets/Sphere/rustediron2_roughness.png";
    fileNames[3].decalBaseColor = "../Assets/Textures/Watchmen.png";
    fileNames[3].decalNormal = "../Assets/Textures/Watchmen_normal.png";
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

    int pick = 0;
    if (currentModel == SHIRT)
    {
        pick = 0;
    }
    else if (currentModel == WORKBOOT)
    {
        pick = 1;
    }
    else if (currentModel == JACKET)
    {
        pick = 2;
    }
    else
    {
        pick = 3;
    }

    /**
     * Start Pre-load Models
     */
    readModelsPreload(modelData, modelsData, fileNames, pick);
    /**
     * End Pre-load Models
     */
    
    /**
     *  Start Create Model Textures 
     */
    
    Shader::TEXTURE_WRAP_PARAMS textureWrapParams = Shader::CLAMP_TO_EDGE;// REPEAT;
    Shader::TEXTURE_SAMPLE_PARAMS textureSampleParams = Shader::LINEAR_MIPS;

	geometryPass.use();
    geometryPass.createTexture(&(modelData[0].material.baseColor), fileNames[pick].baseColor, textureWrapParams, textureSampleParams, "iAlbedo", 0, modelData[0].material.channels[0]);
    //std::cout << "Number of channels in iAlbedo: " << modelData.material.channels[0] << std::endl;
    geometryPass.createTexture(&(modelData[0].material.normal), fileNames[pick].normal, textureWrapParams, textureSampleParams, "iNormal", 1, modelData[0].material.channels[1]);
    //std::cout << "Number of channels in iNormal: " << modelData.material.channels[1] << std::endl;
    if (fileNames[pick].metallic != "")
    {
        geometryPass.createTexture(&(modelData[0].material.metallic), fileNames[pick].metallic, textureWrapParams, textureSampleParams, "iMetallic", 2, modelData[0].material.channels[2]);
        iMetal = true;
    }
    else
    {
        iMetal = false;
    }
    //std::cout << "Number of channels in iMetallic: " << modelData.material.channels[2] << std::endl;
    if (fileNames[pick].roughness != "")
    {
        geometryPass.createTexture(&(modelData[0].material.roughness), fileNames[pick].roughness, textureWrapParams, textureSampleParams, "iRoughness", 3, modelData[0].material.channels[3]);
        iRough = true;
    }
    else
    {
		iRough = false;
    }
    //std::cout << "Number of channels in iRoughness: " << modelData.material.channels[3] << std::endl;

    /**
     *  End Create Model Textures
     */

    /**
     *  Start Create Decals Texture 
     */
    decalsPass.use();
    decalsPass.createTexture(&(modelData[0].material.decalBaseColor), fileNames[pick].decalBaseColor, textureWrapParams, textureSampleParams, "iDecalAlbedo", 4, modelData[0].material.channels[5]);
    decalsPass.createTexture(&(modelData[0].material.decalNormal), fileNames[pick].decalNormal, textureWrapParams, textureSampleParams, "iDecalNormal", 5, modelData[0].material.channels[6]);


    decalsPass.setFloat("bias", computeBiasDepthComparison(modelData[0]));
    decalsPass.setInt("iAlbedo",      0);
    decalsPass.setInt("iNormal",      1);
    decalsPass.setInt("iMetallic",    2);
    decalsPass.setInt("iRoughness",   3);
    decalsPass.setInt("iDecalAlbedo", 4);
    decalsPass.setInt("iDecalNormal", 5);
    decalsPass.setInt("iDepth",       6);
    decalsPass.setInt("iSDF",         7);
    //decalsPass.setInt("iDepth", 2);
    //decalsPass.setInt("iSDF", 3);
    /**
     *  Start Create Decals Texture
     */

    /** Start Depth Buffer **/
    createAndAttachDepthPrePassRbo(depthFramebuffer);
    /** End Depth Buffer **/

    /** Start Texture Space Buffer **/
    textureWrapParams = Shader::CLAMP_TO_EDGE;
    textureSampleParams = Shader::LINEAR_MIPS;// NEAREST;
    createAndAttachTextureSpaceRbo(modelData[0], geometryPass, textureSpaceFramebuffer, textureWrapParams, textureSampleParams);
    /** End Texture Space Buffer **/

    glm::ivec4 widthHeightJFA = glm::ivec4(decalsPass.Width, decalsPass.Height, decalsPass.Width / JFA_FACTOR, decalsPass.Height / JFA_FACTOR);
    //std::cout << "widthHeightJFA.x: " << widthHeightJFA.x << " widthHeightJFA.y: " << widthHeightJFA.y << std::endl;
    /** Start JFA Buffer **/
    JFAPass.use();
    JFAPass.setInt("iChannel0", 0);
    JFAPass.setInt("iChannel1", 1);
    for (size_t i = 0; i < jfaFrameBuffer.size(); ++i)
    {
        createAndAttachRboJFA(jfaFrameBuffer[i], widthHeightJFA.z, widthHeightJFA.w, frameBufferTextureParamsJFA);
    }
    /** End JFA Buffer **/
    
    /** Start SDF Buffer * */
    SDFPass.use();
    SDFPass.setInt("iChannel0", 0);
    //SDFPass.setVec2("iResolution", glm::vec2(widthHeightJFA.z, widthHeightJFA.w));
    createAndAttachSDFFramebuffer(sdfFramebuffer, widthHeightJFA.z, widthHeightJFA.w, frameBufferTextureParamsSDF);
    /** End SDF Buffer **/

    // Pass bias from Bbox.
    //float biasDepthComparison = computeBiasDepthComparison(modelData, depthBias);

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

    /**
     * Start Full Screen Quad
     */
    fullScreenPass.use();
    fullScreenPass.setInt("iChannel0", 0);
    fullScreenPass.setInt("iChannel1", 1);
    fullScreenPass.setInt("iChannel2", 2);
    fullScreenPass.setInt("iChannel3", 3);
    fullScreenPass.setInt("iChannel4", 4);
    renderQuad();
    /**
     * End Full Screen Quad
     */
    
	// Create the camera (eye).
	view = glm::mat4(1.0f);
    /*for (uint16_t i = 0u; i < modelData.size(); ++i)
    {
        modelData[i].modelMatrix = glm::mat4(1.0f);
    }*/
    modelData[0].modelMatrix = glm::mat4(1.0f);
    light.position = glm::vec3(0.0f);
    light.model = glm::mat4(0.0f);

    iTime = 0.0f;

    //unsigned int decalTexture = -1;
    // decalsPass.createTexture(&decalTexture, "Assets/Textures/Batman.jpg", "iChannel0", 1);
    // decalsPass.setInt("iChannel1", 0);
    // decalsPass.setInt("iDepth", 2);

    int click = 0;

    modelData[0].bvo.hitPos = glm::vec3(0.0, 0.0, 0.0);
    modelData[0].bvo.hitNor = glm::vec3(1.0, 1.0, 1.0);

    modelNoGuizmo = modelData[0].modelMatrix;

    //mousePositionX = WIDTH / 2;
    //mousePositionY = HEIGHT / 2;

    widthHeight = glm::vec2(WIDTH, HEIGHT);

    projection = glm::perspective(RADIANS_30, widthHeight.x / widthHeight.y, nearPlane, farPlane);
    if (CAMERA == FPS)
    {
        view = glm::lookAt(camPos, camPos + camFront, camUp);
        viewPinnedTop = view;
        viewPinnedBottom = view;
    }
    else if (CAMERA == TRACK_BALL)
    {
        updateViewMatrix();
        viewPinnedTop = glm::lookAt(camPos, camPos + camFront, camUp);
        viewPinnedBottom = viewPinnedTop;
    }

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
    ClearModelsVertexData(modelData);

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
    glm::vec3 eye = modelData[0].Bbox.centroid - glm::vec3(-m_cameraState.position, m_cameraState.zoom);
    translate = glm::translate(translate, -eye);
    view = translate * rotate;
    glm::mat4 invView = glm::inverse(view);
    camPos = glm::vec3(invView[3].x, invView[3].y, invView[3].z);
}

void updateViewMatrixPanels(const glm::vec2& position, const float& zoom, glm::mat4& view)
{
    glm::mat4 translate = glm::mat4(1.0f);
    glm::vec3 eye = modelData[0].Bbox.centroid - glm::vec3(position, zoom);
    translate = glm::translate(translate, -eye);
    view = translate;
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
            for (uint16_t i = 0u; i < modelData.size(); ++i)
            {
                modelData[i].bvo.isTracing = true;
            }
            // Raytrace.
            rayTrace(event.motion.x + (splitScreen ? WIDTH / 4 : 0), event.motion.y, widthHeight, 
                     /* In and Out */modelData, 
                     projection, view, false);
            // Side panels.
            if (splitScreen)
            {
                if (event.motion.x < (WIDTH / 2))
                {
                    /*m_drag.mouseWheelButtonActive = true;
                    m_drag.mouseWheelActionStartMouse.x = mouse.x;
                    m_drag.mouseWheelActionStartMouse.y = mouse.y;
                    m_drag.startCameraState = m_cameraState;*/
                }
                else if (event.motion.y > (HEIGHT / 2))
                {
                    std::cout << "Bottom panel right click." << std::endl;
                    m_drag.activeBottomPanel = true;
                    m_drag.mouseBottomActionStartMouse.x = mouse.x;
                    m_drag.mouseBottomActionStartMouse.y = mouse.y;
                    m_drag.startCameraState = m_cameraState;
                }
                else
                {
                    m_drag.activeTopPanel = true;
                    m_drag.mouseTopActionStartMouse.x = mouse.x;
                    m_drag.mouseTopActionStartMouse.y = mouse.y;
                    m_drag.startCameraState = m_cameraState;
                }
            }
            else
            {
                /*m_drag.mouseWheelButtonActive = true;
                m_drag.mouseWheelActionStartMouse.x = mouse.x;
                m_drag.mouseWheelActionStartMouse.y = mouse.y;
                m_drag.startCameraState = m_cameraState;*/
            }
            break;
        }
        if (CAMERA == TRACK_BALL)
        {
            case SDL_BUTTON_MIDDLE:
            {
                // Side panels.
                if (splitScreen)
                {
                    if (event.motion.x < (WIDTH / 2))
                    {
                        m_drag.mouseWheelButtonActive = true;
                        m_drag.mouseWheelActionStartMouse.x = mouse.x;
                        m_drag.mouseWheelActionStartMouse.y = mouse.y;
                        m_drag.startCameraState = m_cameraState;
                    }
                    else if (event.motion.y > (HEIGHT / 2))
                    {
                        m_drag.activeBottomPanel = true;
                        m_drag.mouseBottomActionStartMouse.x = mouse.x;
                        m_drag.mouseBottomActionStartMouse.y = mouse.y;
                        m_drag.startCameraState = m_cameraState;
                    }
                    else
                    {
                        m_drag.activeTopPanel = true;
                        m_drag.mouseTopActionStartMouse.x = mouse.x;
                        m_drag.mouseTopActionStartMouse.y = mouse.y;
                        m_drag.startCameraState = m_cameraState;
                    }
                }
                else
                {
                    m_drag.mouseWheelButtonActive = true;
                    m_drag.mouseWheelActionStartMouse.x = mouse.x;
                    m_drag.mouseWheelActionStartMouse.y = mouse.y;
                    m_drag.startCameraState = m_cameraState;
                }
                break;
            }
        }
        default:
        {
            break;
        }
    }
}

void mouse_unpressed(SDL_Event& event)
{
    switch (event.button.button)
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
            glm::ivec2 currentMouse = glm::ivec2(event.motion.x, -event.motion.y + HEIGHT);
            mousePositionX = currentMouse.x;
            mousePositionY = currentMouse.y;
            std::cout <<  "Mouse Position X: " << mousePositionX << 
                         " Mouse Position Y: " << mousePositionY << std::endl;
            for (uint16_t i = 0u; i < modelData.size(); ++i)
            {
                modelData[i].bvo.isTracing = false;
            }
            if (splitScreen)
            {
                if (event.motion.x < (WIDTH / 2))
                {
                    //m_drag.mouseWheelButtonActive = false;
                }
                else if (event.motion.y > (HEIGHT / 2))
                {
                    m_drag.activeBottomPanel = false;
                }
                else
                {
                    m_drag.activeTopPanel = false;
                }
            }
            else
            {
                //m_drag.mouseWheelButtonActive = false;
            }
            break;
        }
        if (CAMERA == TRACK_BALL)
        {
            case SDL_BUTTON_MIDDLE:
            {
                if (splitScreen)
                {
                    if (event.motion.x < (WIDTH / 2))
                    {
                        m_drag.mouseWheelButtonActive = false;
                    }
                    else if (event.motion.y > (HEIGHT / 2))
                    {
                        m_drag.activeBottomPanel = false;
                    }
                    else
                    {
                        m_drag.activeTopPanel = false;
                    }
                }
                else
                {
                    m_drag.mouseWheelButtonActive = false;
                }
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
        if (splitScreen)
        {
            if (m_drag.mouseWheelButtonActive)
            {
                glm::vec2 delta = (m_drag.mouseWheelActionStartMouse -
                                   glm::vec2(currentMouse.x, currentMouse.y)) *
                                   m_drag.sensitivity;
                m_cameraState.position = m_drag.startCameraState.position + delta;
                updateViewMatrix();
            }
            // Bottom Right panel
            else if (m_drag.activeBottomPanel)
            {
                glm::vec2 delta = (m_drag.mouseBottomActionStartMouse -
                                   glm::vec2(currentMouse.x, currentMouse.y)) *
                                   m_drag.sensitivity;
                m_cameraState.positionBottom = m_drag.startCameraState.positionBottom + delta;
                updateViewMatrixPanels(m_cameraState.positionBottom, m_cameraState.zoomSide, viewPinnedBottom);
            }
            else if (m_drag.activeTopPanel)
            {
                glm::vec2 delta = (m_drag.mouseTopActionStartMouse -
                                   glm::vec2(currentMouse.x, currentMouse.y)) *
                                   m_drag.sensitivity;
                m_cameraState.positionTop = m_drag.startCameraState.positionTop + delta;
                updateViewMatrixPanels(m_cameraState.positionTop, m_cameraState.zoomTop, viewPinnedTop);
            }
        }
        else if (m_drag.mouseWheelButtonActive)
        {
            glm::vec2 delta = (m_drag.mouseWheelActionStartMouse -
                               glm::vec2(currentMouse.x, currentMouse.y)) *
                               m_drag.sensitivity;
            m_cameraState.position = m_drag.startCameraState.position + delta;
            updateViewMatrix();
        }
    }
    // Raytrace.
    rayTrace(event.motion.x + (splitScreen ? WIDTH / 4 : 0), event.motion.y, widthHeight,
        /* In and Out */modelData,
        projection, view, false);
}

void updateZoom(SDL_MouseWheelEvent* mouseWheel)
{
    m_cameraState.zoom += m_drag.scrollSensitivity * mouseWheel->preciseY;
    updateViewMatrix();
}
void updateZoomSidePanels(SDL_MouseWheelEvent* mouseWheel, float& zoom)
{
    zoom += m_drag.scrollSensitivityPanels * mouseWheel->preciseY;
}

void mouse_wheel(SDL_Event& event)
{
    if (CAMERA == TRACK_BALL)
    {
        SDL_MouseWheelEvent* mouseWheel = (SDL_MouseWheelEvent*)&event;
        if (splitScreen)
        {
            if (mouseWheel->mouseX < (WIDTH / 2))
            {
                updateZoom(mouseWheel);
                std::cout << "Left Screen" << std::endl;
            }
            else if (mouseWheel->mouseY > (HEIGHT / 2))
            {
                updateZoomSidePanels(mouseWheel, m_cameraState.zoomSide);
                std::cout << "Right Botton Screen: Mouse Wheel Precise Y: " << mouseWheel->preciseY << std::endl;
            }
            else
            {
                updateZoomSidePanels(mouseWheel, m_cameraState.zoomTop);
                std::cout << "Right Top Screen Mouse: Wheel Precise Y: " << mouseWheel->preciseY << std::endl;
            }
        }
        else
        {
            updateZoom(mouseWheel);
        }
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

void multiple_touches(SDL_Event& event)
{
    //SDL_MultiGestureEvent* m = (SDL_MultiGestureEvent*)&event;
    if (event.mgesture.numFingers == 2)
    {
        glm::ivec2 mouseDenormalize = glm::ivec2(glm::vec2(event.mgesture.x, event.mgesture.y) * glm::vec2(WIDTH, HEIGHT));
        mousePositionX = mouseDenormalize.x;
        mousePositionY = mouseDenormalize.y;
        rayTrace(mousePositionX + (splitScreen ? WIDTH / 4 : 0), mousePositionY, widthHeight, 
                 /* In and Out */modelData, 
                 projection, view, false);
        m_drag.activeTouchPad = true;
        /*m_drag.mouseWheelButtonActive = false;
        m_drag.mouseWheelButtonActiveSince = 0u;
        isActiveFPS = false;*/
    }
    //std::cout << "Number of fingers: " << event.mgesture.numFingers << std::endl;
}

void regenerateDepthFramebufferTexture(frameBuffer& framebuffer)
{
    glDeleteTextures(1, &(framebuffer.textures[0]));
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.framebuffer);
    
    glGenTextures(1, &(framebuffer.textures[0]));
    glBindTexture(GL_TEXTURE_2D, framebuffer.textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, WIDTH / DEPTH_FACTOR, HEIGHT / DEPTH_FACTOR, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, framebuffer.textures[0], 0);
}

void regenerateModel(std::vector<ModelData>& modelData, std::vector<ModelData>& newModelData, Shader& shader, Shader& decalsShader, frameBuffer& framebuffer, const std::vector<ModelFileNames>& fileNamesVector, const int& pickModel, bool& iMetal, bool& iRough)
{
    ClearModelsVertexData(modelData);
    modelData = newModelData;

	ModelFileNames fileNames = fileNamesVector[pickModel];
    regenerateTexture(shader, modelData[0], BASE_COLOR, fileNames.baseColor, framebuffer);
    regenerateTexture(shader, modelData[0], NORMAL,     fileNames.normal, framebuffer);
    
    if (fileNames.metallic != "")
    {
        regenerateTexture(shader, modelData[0], METAL, fileNames.metallic, framebuffer);
        //iMetal = true;
    }
    else
    {
        std::cout << "No data for metallic texture" << std::endl;
		//std::cout << "Filename metallic: " << fileNamesVector[2].metallic << std::endl;
        regenerateTexture(shader, modelData[0], METAL, fileNamesVector[2].metallic, framebuffer);
        //iMetal = false;
    }
    if (fileNames.roughness != "")
    {
        regenerateTexture(shader, modelData[0], ROUGH, fileNames.roughness, framebuffer);
        //iRough = true;
    }
    else
    {
        std::cout << "No data for roughness texture" << std::endl;
        //std::cout << "Filename roughness: " << fileNamesVector[2].roughness << std::endl;
        regenerateTexture(shader, modelData[0], ROUGH, fileNamesVector[2].roughness, framebuffer);
        //iRough = false;
    }
	std::cout << "iMetal: " << iMetal << " iRough: " << iRough << std::endl;

    frameBuffer fboDummy;
    regenerateTexture(decalsShader, modelData[0], DECAL_BASE_COLOR, fileNames.decalBaseColor, fboDummy);
    regenerateTexture(decalsShader, modelData[0], DECAL_NORMAL, fileNames.decalNormal, fboDummy);
    
    float biasDepthComparison = computeBiasDepthComparison(modelData[0], depthBias);
    decalsPass.setFloat("bias", biasDepthComparison);
    CreateBOs(modelData);
    updateViewMatrix();
    mousePositionX = WIDTH / (splitScreen ? 4 : 2);
    mousePositionY = HEIGHT / 2;
    rayTrace(mousePositionX + (splitScreen ? WIDTH / 4 : 0), mousePositionY, widthHeight, 
             /* In and Out */ modelData, 
             projection, view, false);
}

void activateAndBindTexturesTexSpaceFramebuffer(frameBuffer& frameBuffer, Shader& shader, const Shader::TEXTURE_WRAP_PARAMS wrapParam, const Shader::TEXTURE_SAMPLE_PARAMS sampleParam, const std::array<unsigned int, 6>& textureArray, const bool textureArrayBool)
{
    uint8_t maxIter = frameBuffer.textures.size();
    for (uint8_t i = 0u; i < maxIter; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + ((GLenum)i));
        glBindTexture(GL_TEXTURE_2D, (textureArrayBool ? textureArray[i] : frameBuffer.textures[i]));
        
        // Set sampling and wrapping modes.
        shader.textureWrap(wrapParam);
        shader.textureSample(sampleParam);
    }
}

void main_loop()
{
    widthHeight = glm::vec2(WIDTH, HEIGHT);
    glm::ivec4 widthHeightJFA = glm::ivec4(decalsPass.Width, decalsPass.Height, decalsPass.Width / jfaFactor, decalsPass.Height / jfaFactor);

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

    Shader::TEXTURE_WRAP_PARAMS textureWrapParams = Shader::CLAMP_TO_EDGE;// REPEAT;
    Shader::TEXTURE_SAMPLE_PARAMS textureSampleParams = Shader::LINEAR_MIPS;
    
    if (decalAlbedoImageBuffer.size() > 0)
    {
        recomputeDecalBaseColorTexture(decalsPass, textureWrapParams, textureSampleParams);
    }
    if (decalNormalImageBuffer.size() > 0)
    {
        recomputeDecalNormalTexture(decalsPass, textureWrapParams, textureSampleParams);
    }

    /**
     * Start Light Setup
     */
    float timeSpeed = iTime * light.speed;
    float sinTime = sinf(timeSpeed) * 2.0f;
    float cosTime = cosf(timeSpeed) * 2.0f;
    // Change for all the meshes of the model BBox.
    glm::vec3 lightPosition = (modelData[0].Bbox.centroid +
                               glm::vec3(0.0f, modelData[0].Bbox.bboxMax.y, 0.0f));
    glm::vec3 rotateLight = glm::vec3(sinTime, 1.0f, cosTime);
    rotateLight += lightPosition;
    light.model = glm::mat4(1.0f);
    light.model = glm::translate(light.model, rotateLight);
    light.position = rotateLight;
    //light.position = glm::vec3(light.model * glm::vec4(0.0f, 0.0f, 0.f, 0.0f));
    /**
     * End Light Setup
     */

    /**
     * Start ImGui
     */
    bool shiftIsPressed = false;

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

        insideImGui = io.WantCaptureMouse;
        keyboardImGui = io.WantCaptureKeyboard;

        if (insideImGui || keyboardImGui)
        {
            break;
        }

        // Only do input handling on the perspective viewport.
        bool isInPerspectiveViewport = (event.motion.x < halfWidthHeight.x ? true : false);
        isInPerspectiveViewport = splitScreen ? isInPerspectiveViewport : true;

        if (!isInPerspectiveViewport)
        {
            //break;
        }

        switch (event.type)
        {
            case SDL_MOUSEBUTTONDOWN:
            {
                mouse_press(event);
                break;
            }
            case SDL_MOUSEBUTTONUP:
            {
                mouse_unpressed(event);
                break;
            }
            case SDL_MOUSEMOTION:
            {
                mouse_motion(event);
                break;
            }
            case SDL_MOUSEWHEEL:
            {
                mouse_wheel(event);
                break;
            }
            case SDL_KEYDOWN:
            {
                key_down(event);
                break;
            }
#ifdef __EMSCRIPTEN__
            case SDL_MULTIGESTURE:
            {
                //std::cout << "Multi " << std::endl;
                multiple_touches(event);
                break;
            }
#endif
            case SDL_WINDOWEVENT:
            {
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    WIDTH = event.window.data1;
                    HEIGHT = event.window.data2;
                    depthPrePass.use();
                    regenerateDepthFramebufferTexture(depthFramebuffer);
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

#ifdef __EMSCRIPTEN__
    emscripten_webgl_make_context_current(ctx); // Just in case
#endif
    
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    ImGui::Begin("Graphical User Interface");   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    std::string printTime = std::to_string(deltaTime * 1000.0f) + " ms.\n";
    ImGui::TextUnformatted(printTime.c_str());
    static const char* textureTypes[]{ "3D View", "Albedo", "Normals", "Metallic", "Roughness", "JFA" };
    static const char* selectedTexture = textureTypes[0];
    bool textureSelectionDirty = ImGui::BeginCombo("Display", selectedTexture, 0);
    //std::cout << (cameraSelectionDirty ? "Camera Dirty" : "Camera Not Dirty") << std::endl;
    if (textureSelectionDirty)
    {
        for (int n = 0; n < IM_ARRAYSIZE(textureTypes); n++)
        {
            bool is_selected = (selectedTexture == textureTypes[n]);
            if (ImGui::Selectable(textureTypes[n], is_selected))
            {
                selectedTexture = textureTypes[n];
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
            }
        }
        ImGui::EndCombo();
        //std::cout << "CAMERA: " << CAMERA << std::endl;
        if (strcmp(selectedTexture, "3D View") == 0)
        {
            showTextures = CURRENT_MODEL;
        }
        else if (strcmp(selectedTexture, "Albedo") == 0)
        {
            showTextures = TEXTURE_SPACE;
        }
        else if (strcmp(selectedTexture, "Normals") == 0)
        {
            showTextures = NORMALS;
        }
        else if (strcmp(selectedTexture, "Metallic") == 0)
        {
            showTextures = METALLIC;
        }
        else if (strcmp(selectedTexture, "Roughness") == 0)
        {
            showTextures = ROUGHNESS;
        }
        else
        {
            showTextures = JFA;
        }
    }
    if (showTextures == CURRENT_MODEL)
    {
        static const char* cameraTypes[]{ "FPS", "Trackball" };
        static const char* selectedItem = cameraTypes[1];
        bool cameraSelectionDirty = ImGui::BeginCombo("Camera", selectedItem, 0);
        //std::cout << (cameraSelectionDirty ? "Camera Dirty" : "Camera Not Dirty") << std::endl;
        if (cameraSelectionDirty)
        {
            for (int n = 0; n < IM_ARRAYSIZE(cameraTypes); n++)
            {
                bool is_selected = (selectedItem == cameraTypes[n]);
                if (ImGui::Selectable(cameraTypes[n], is_selected))
                {
                    selectedItem = cameraTypes[n];
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                }
            }
            ImGui::EndCombo();
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
        }
    }
    
    bool regeneratedModel = false;
    static const char* modelsToSelect[]{ "Shirt", "Workboot", "Jacket", "Sphere"};
    static const char* selectedModel = modelsToSelect[0];
    int pickModel = 0;
    if (currentModel == SHIRT)
    {
        selectedModel = modelsToSelect[0];
		pickModel = 0;
    }
    else if (currentModel == WORKBOOT)
    {
        selectedModel = modelsToSelect[1];
		pickModel = 1;
    }
    else if (currentModel == JACKET)
    {
        selectedModel = modelsToSelect[2];
		pickModel = 2;
	}
    else // SPHERE
    {
		selectedModel = modelsToSelect[3];
        pickModel = 3;
    }
    bool selectedModelDirty = ImGui::BeginCombo("Model", selectedModel, 0);
    if (selectedModelDirty)
    {
        for (int n = 0; n < IM_ARRAYSIZE(modelsToSelect); n++)
        {
            bool is_selected = (selectedModel == modelsToSelect[n]);
            if (ImGui::Selectable(modelsToSelect[n], is_selected))
            {
                selectedModel = modelsToSelect[n];
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
            }
        }
        ImGui::EndCombo();
        
        bool recomputeModel = false;
        if (strcmp(selectedModel, "Shirt") == 0 && currentModel != SHIRT)
        {
            currentModel = SHIRT;
            pickModel = 0;
            recomputeModel = true;
            drawingMode = GL_TRIANGLES;
            iMetal = false;
            iRough = false;
            std::cout << "Shirt" << std::endl;
        }
        else if (strcmp(selectedModel, "Workboot") == 0 && currentModel != WORKBOOT)
        {
            currentModel = WORKBOOT;
            pickModel = 1;
            recomputeModel = true;
            drawingMode = GL_TRIANGLES;
            iMetal = true;
            iRough = true;
            std::cout << "Workboot" << std::endl;
        }
        else if (strcmp(selectedModel, "Jacket") == 0 && currentModel != JACKET)
        {
            currentModel = JACKET;
            pickModel = 2;
            recomputeModel = true;
            drawingMode = GL_TRIANGLES;
            iMetal = true;
            iRough = true;
            std::cout << "Jacket" << std::endl;
        }
        else if (strcmp(selectedModel, "Sphere") == 0 && currentModel != SPHERE)
        {
            currentModel = SPHERE;
            pickModel = 3;
            recomputeModel = true;
            drawingMode = GL_TRIANGLE_STRIP;
            iMetal = true;
            iRough = true;
            std::cout << "Sphere" << std::endl;
        }
        if (recomputeModel)
        {
            std::cout << "Regenerating model" << std::endl;

            //regenerateTextureSpaceFramebuffer(textureSpaceFramebuffer, geometryPass, textureWrapParamsDecalOutputs, textureSampleParamsDecalOutputs);
            
			std::cout << "iMetal: " << iMetal << " iRough: " << iRough << std::endl;
            
            regenerateModel(modelData, modelsData[pickModel], geometryPass, decalsPass, textureSpaceFramebuffer, fileNames, pickModel, iMetal, iRough);
            
            frameJFA = 0;
            downloadImage = 1u;
            regenerateAllFramebufferTexturesJFA(jfaFrameBuffer, sdfFramebuffer, widthHeightJFA, frameBufferTextureParamsJFA, frameBufferTextureParamsSDF);

            //regenerateTextureSpaceFramebuffer(textureSpaceFramebuffer, geometryPass, textureWrapParamsDecalOutputs, textureSampleParamsDecalOutputs);
            
            regeneratedModel = true;
        }
    }
    if (showTextures == CURRENT_MODEL)
    {
        if (ImGui::Button("Split Screen"))
        {
            splitScreen = !splitScreen;
        }
        if (ImGui::Button("Enable Normal Map"))
        {
            normalMap = !normalMap;
        }
        if (normalMap && ImGui::Button("Physically Based Lighting"))
        {
            pbr = !pbr;
        }
        if (ImGui::Button("See Light"))
        {
            light.toggle = !light.toggle;
        }
        ImGui::SliderFloat("Light Speed", &(light.speed), 0.0f, 1.0f);
    }
    if (showTextures == TEXTURE_SPACE || showTextures == NORMALS || showTextures == METALLIC || showTextures == ROUGHNESS)
    {
        ImGui::SliderInt("Scale Texture", &fullScreenPassRepeat, 1, 10);
    }
    if (ImGui::SliderInt("Texture Coordinates Scale", &scale, 1, 10))
    {
        frameJFA = 0;
    }
    if (alphaJFA)
    {
        if (ImGui::SliderInt("Scale JFA", &jfaFactor, 2, 8))
        {
            frameJFA = 0;
            downloadImage = 1u;
            regenerateAllFramebufferTexturesJFA(jfaFrameBuffer, sdfFramebuffer, widthHeightJFA, frameBufferTextureParamsJFA, frameBufferTextureParamsSDF);
        }
    }
    if (showTextures == CURRENT_MODEL || showTextures == TEXTURE_SPACE || showTextures == NORMALS || showTextures == METALLIC || showTextures == ROUGHNESS)
    {
        if (ImGui::Button("Show Blend"))
        {
            showSDFBox = !showSDFBox;
        }
        ImGui::SliderFloat("Blend Factor", &blend, 0.0f, 1.0f);
    }
    if (ImGui::Button("Use Alpha for Decal"))
    {
        alphaJFA = !alphaJFA;
        frameJFA = 0u;
    }
    if (alphaJFA)
    {
        if (ImGui::SliderFloat("Alpha Cut", &alphaCut, 0.0f, 1.0f))
        {
            frameJFA = 0u;
        }
        ImGui::SliderFloat("Smoothness", &smoothAlpha, 0.0f, 1.0f);
        ImGui::SliderInt("Pixel Width Alpha", &distanceWidthJFA, 1, 32);
    }
    if (showTextures == CURRENT_MODEL || showTextures == TEXTURE_SPACE || showTextures == NORMALS || showTextures == METALLIC || showTextures == ROUGHNESS)
    {
        glm::vec2 projectorDirty = glm::vec2(projectorSize, projectorRotation);
        ImGui::SliderFloat("Projector Size", &projectorSize, 0.001f, 1.0f);
        ImGui::SliderFloat("Projector Orientation", &projectorRotation, 0.0f, 360.0f);
        if (projectorDirty.x != projectorSize || projectorDirty.y != projectorRotation ||
            frame == 1u)
        {
            rayTrace(mousePositionX + (splitScreen ? WIDTH / 4 : 0), -mousePositionY + HEIGHT, widthHeight,
                     /* In and Out */ modelData,
                     projection, view, false);
        }
    }
    if (ImGui::Button("Flip decals"))
    {
        flipDecal = !flipDecal;
        frameJFA = 0;
    }
    if (showTextures == CURRENT_MODEL)
    {
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
        for (int i = 0; i < 1; ++i)
        {
            ImGuizmo::PushID(i);
            EditTransform(glm::value_ptr(view), glm::value_ptr(projectionHalf), glm::value_ptr(modelData[0].modelMatrix), lastUsing == i, splitScreen, io);
            if (ImGuizmo::IsUsing())
            {
                lastUsing = i;
            }
            ImGuizmo::PopID();
        }
    }
    ImGui::End();
    // If we have a GLTF we need to invert the texture coordinates.
    flipper = isGLTF ? 1 : 0;
    decalsPass.use();
    decalsPass.setInt("iFlipper", flipper);

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

    glViewport(0, 0, WIDTH / DEPTH_FACTOR, HEIGHT / DEPTH_FACTOR);
    glClear(GL_DEPTH_BUFFER_BIT);

    for (uint16_t i = 0u; i < modelData.size(); ++i)
    {
        glBindVertexArray(modelData[i].openGLObject.VAO);
        depthPrePass.setMat4("decalProjector", modelData[i].bvo.decalProjector);
        /*glViewport(0, 0, WIDTH / DEPTH_FACTOR, HEIGHT / DEPTH_FACTOR);*/

        /*glClear(GL_DEPTH_BUFFER_BIT);*/
        glDrawElements(drawingMode, (GLsizei)modelData[i].indexes.size(), GL_UNSIGNED_INT, 0);
    }

    /** End Depth Pre-Pass **/

    float flipFloat = (float)flip;
    float flipAlbedoFloat = (float)flipAlbedo;

    textureWrapParams = Shader::CLAMP_TO_EDGE;// REPEAT;
    textureSampleParams = Shader::NEAREST;
    
    if (alphaJFA)
    {
        /** Start Jump Flooding Algorithm **/
        // JFA
        // Ping pong.
        glBindFramebuffer(GL_FRAMEBUFFER, (frameIsEven ? jfaFrameBuffer[0].framebuffer : jfaFrameBuffer[1].framebuffer));
        glBindVertexArray(quadVAO);
        JFAPass.use();
        JFAPass.setInt("iFrame", (int)frameJFA);
        JFAPass.setVec2("iResolution", glm::vec2(widthHeightJFA.x, widthHeightJFA.y));
        JFAPass.setFloat("iAlphaCut", alphaCut);
        float maxSteps = std::floor(std::log2(std::max(widthHeightJFA.z, widthHeightJFA.w)));
        //std::cout << "Max steps: " << maxSteps << std::endl;
        JFAPass.setFloat("iMaxSteps", maxSteps);
        JFAPass.setInt("iScale", scale);
        JFAPass.setBool("iFlipDecal", flipDecal);
        
        textureWrapParams = Shader::REPEAT;
        textureSampleParams = Shader::NEAREST;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, (frameIsEven ? jfaFrameBuffer[1].textures[0] : jfaFrameBuffer[0].textures[0]));
        JFAPass.textureWrap(textureWrapParams);
        JFAPass.textureSample(textureSampleParams);
        
        textureWrapParams = Shader::REPEAT;
        textureSampleParams = Shader::LINEAR_MIPS;
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, modelData[0].material.decalBaseColor);
        JFAPass.textureWrap(textureWrapParams);
        JFAPass.textureSample(textureSampleParams);
        
        glViewport(0, 0, widthHeightJFA.z, widthHeightJFA.w);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        
        // Reconstruct SDF
        glBindFramebuffer(GL_FRAMEBUFFER, sdfFramebuffer.framebuffer);
        glBindVertexArray(quadVAO);
        SDFPass.use();
        //SDFPass.setInt("iFrame", (int)frame);
        SDFPass.setInt("iChannel0", 0);
        SDFPass.setVec2("iResolution", glm::vec2(widthHeightJFA.x, widthHeightJFA.y));
        SDFPass.setFloat("iDistanceWidth", (float)distanceWidthJFA);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, (frameIsEven ? jfaFrameBuffer[0].textures[0] : jfaFrameBuffer[1].textures[0]));
        textureWrapParams = Shader::REPEAT;
        textureSampleParams = Shader::NEAREST;
        SDFPass.textureWrap(textureWrapParams);
        SDFPass.textureSample(textureSampleParams);
        //glActiveTexture(GL_TEXTURE1);
        //glBindTexture(GL_TEXTURE_2D, modelData.material.decalBaseColor);
        
        glViewport(0, 0, widthHeightJFA.z, widthHeightJFA.w);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        
        /** End Jump Flooding Algorithm **/
    }

    /** Start Decal Pass **/
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, textureSpaceFramebuffer.framebuffer);


    for (uint16_t i = 0u; i < modelData.size(); ++i)
    {
        
        if (modelData[i].bvo.decalProjector != glm::mat4(1.0f))
        {
            modelData[0].bvo.decalProjector = modelData[i].bvo.decalProjector;
        }
    }

    decalsPass.use();
    decalsPass.setMat4("model", modelData[0].modelMatrix);
    decalsPass.setMat4("view", view);
    decalsPass.setMat4("projection", projection);

    decalsPass.setMat4("decalProjector", modelData[0].bvo.decalProjector);

    decalsPass.setVec2("iResolution", widthHeight);
    decalsPass.setInt("iScale", scale);
    decalsPass.setFloat("iFlip", flipFloat);
    decalsPass.setFloat("iBlend", blend);
    decalsPass.setFloat("iSmoothness", smoothAlpha * 0.5);
    decalsPass.setFloat("iFlipAlbedo", flipAlbedoFloat);
    decalsPass.setBool("iAlpha", alphaJFA);
    decalsPass.setBool("iShowSDFBox", showSDFBox);
    decalsPass.setBool("iMetal", iMetal);
    decalsPass.setBool("iRough", iRough);

    std::array<unsigned int, 6> textureArray = { modelData[0].material.baseColor, modelData[0].material.normal, modelData[0].material.metallic, modelData[0].material.roughness,
        modelData[0].material.decalBaseColor, modelData[0].material.decalNormal };
    // Albedo
    textureWrapParams = Shader::CLAMP_TO_EDGE;
    textureSampleParams = Shader::LINEAR_MIPS;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, modelData[0].material.baseColor);
    decalsPass.textureWrap(textureWrapParams);
    decalsPass.textureSample(textureSampleParams);

    // Normal
    //decalsPass.setInt("iNormal", 1);
    //textureWrapParams = Shader::CLAMP_TO_EDGE;
    //textureSampleParams = Shader::LINEAR_MIPS;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, modelData[0].material.normal);

    // Metallic
    //decalsPass.setInt("iMetallic", 2);
    //textureWrapParams = Shader::CLAMP_TO_EDGE;
    //textureSampleParams = Shader::LINEAR_MIPS;
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, modelData[0].material.metallic);

    // Roughness
    //decalsPass.setInt("iRoughness", 3);
    //textureWrapParams = Shader::CLAMP_TO_EDGE;
    //textureSampleParams = Shader::LINEAR_MIPS;
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, modelData[0].material.roughness);
    
    // Decal Base Color
    //decalsPass.setInt("iDecalAlbedo", 4);
    //textureWrapParams = Shader::CLAMP_TO_EDGE;
    //textureSampleParams = Shader::LINEAR_MIPS;
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, modelData[0].material.decalBaseColor);
    //decalsPass.textureWrap(textureWrapParams);
    //decalsPass.textureSample(textureSampleParams);

    // Decal Normal
    //decalsPass.setInt("iDecalNormal", 5);
    //textureWrapParams = Shader::CLAMP_TO_EDGE;
    //textureSampleParams = Shader::LINEAR_MIPS;
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, modelData[0].material.decalNormal);
    //decalsPass.textureWrap(textureWrapParams);
    //decalsPass.textureSample(textureSampleParams);
    
    // Depth
    //decalsPass.setInt("iDepth", 6);
    textureWrapParams = Shader::CLAMP_TO_EDGE;
    textureSampleParams = Shader::NEAREST;
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, depthFramebuffer.textures[0]);
    decalsPass.textureWrap(textureWrapParams);
    decalsPass.textureSample(textureSampleParams);
    
    // SDF
    //textureSampleParams = Shader::LINEAR;
    //decalsPass.textureSample(textureSampleParams);
    //decalsPass.setInt("iSDF", 7);
    //textureWrapParams = Shader::CLAMP_TO_EDGE;
    //textureSampleParams = Shader::NEAREST;
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, sdfFramebuffer.textures[0]);
        
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (uint16_t i = 0u; i < modelData.size(); ++i)
    {
		glBindVertexArray(modelData[i].openGLObject.VAO);
        glViewport(0, 0, geometryPass.Width, geometryPass.Height);

        /*
        glBindVertexArray(modelData[i].openGLObject.VAO);
        depthPrePass.setMat4("decalProjector", modelData[i].bvo.decalProjector);
        glViewport(0, 0, WIDTH / DEPTH_FACTOR, HEIGHT / DEPTH_FACTOR);

        glClear(GL_DEPTH_BUFFER_BIT);
        glDrawElements(drawingMode, (GLsizei)modelData[i].indexes.size(), GL_UNSIGNED_INT, 0);
        */

        glDrawElements(drawingMode, (GLsizei)modelData[i].indexes.size(), GL_UNSIGNED_INT, 0);
    }
	uint16_t mipLevels = (uint16_t)(std::floor(std::log2(std::max(geometryPass.Width, geometryPass.Height)))) + 1u;
    // Generate mipmaps.
    for (int i = 0; i < textureSpaceFramebuffer.textures.size(); ++i)
    {
        bool generateMips = false;
        if (i < 2)
        {
            generateMips = true;
        }
        if (i == 2 && iMetal)
        {
            generateMips = true;
        }
        if (i == 3 && iRough)
        {
            generateMips = true;
        }        
        if (generateMips)
        {
            glBindTexture(GL_TEXTURE_2D, textureSpaceFramebuffer.textures[i]);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }

    if (downloadImage == 1u)
    {
        decalAlbedoResult = uploadImage(0);
        decalNormalResult = uploadImage(1);
        // counter = 0;
        downloadImage = 0u;

        if (frame < 2u)
        {
            // Get the first position for ray picking.
            mousePositionX = WIDTH / 2;
            mousePositionY = HEIGHT / 2;
        }
        std::cout << "Uploading image!" << std::endl;
#ifdef OPTIMIZE
#else
        std::cout << "Uploading image!" << std::endl;
#endif
    }
    /** End Decal Pass **/
    if (showTextures != CURRENT_MODEL)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(quadVAO);
        fullScreenPass.use();
        glm::vec2 fullScreenPassResolution = glm::vec2(1.0f, 1.0f);
        bool drawJFA = alphaJFA && showTextures == JFA;
        if (drawJFA)
        {
            fullScreenPassResolution = glm::vec2(decalsPass.Width, decalsPass.Height);
        }
        else
        {
            fullScreenPassResolution = glm::vec2(geometryPass.Width, geometryPass.Height);
        }
        fullScreenPass.setVec2("iResolution", fullScreenPassResolution);
        
        textureSampleParams = Shader::LINEAR_MIPS;// NEAREST;
        fullScreenPass.textureSample(textureSampleParams);
        
        activateAndBindTexturesTexSpaceFramebuffer(textureSpaceFramebuffer, fullScreenPass, textureWrapParamsDecalOutputs, textureSampleParamsDecalOutputs, textureArray, !true);
        
        textureWrapParams = Shader::CLAMP_TO_EDGE;
        textureSampleParams = Shader::NEAREST;
        glActiveTexture(GL_TEXTURE0 + 4);
        glBindTexture(GL_TEXTURE_2D, sdfFramebuffer.textures[0]);
		fullScreenPass.textureWrap(textureWrapParams);
		fullScreenPass.textureSample(textureSampleParams);
        
        fullScreenPass.setInt("iScale", fullScreenPassRepeat);
        
        fullScreenPass.setBool("iAlpha", drawJFA);
        fullScreenPass.setFloat("iSmoothness", smoothAlpha * 0.5);
        fullScreenPass.setBool("iNormal", (showTextures == NORMALS ? true : false));
        //if (showTextures != CURRENT_MODEL || showTextures != JFA)
        fullScreenPass.setInt("iTexture", (int)showTextures);
         
        glViewport(0, 0, WIDTH, HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }
    else
    {
        /** Start Light Pass **/
        glScissor(0, 0, WIDTH / 2, HEIGHT);
        int enableNormals = normalMap ? 1 : 0;
        if (splitScreen)
        {
            glViewport(0, 0, WIDTH / 2, HEIGHT);
        }
        else
        {
            //std::cout << "Window width: " << WIDTH << " height: " << HEIGHT << " aspect: " << aspect << std::endl;
            glViewport(0, 0, WIDTH, HEIGHT);
        }
        /*std::cout << "Print data before first draw call: " << std::endl;
        printModelData(modelData);*/
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (uint i = 0u; i < modelData.size(); ++i)
        {
            glBindVertexArray(modelData[i].openGLObject.VAO);
            deferredPass.use();
            deferredPass.setMat4("model", modelData[i].modelMatrix);
            deferredPass.setMat4("projection", projectionHalf);
            deferredPass.setMat4("view", view);
            deferredPass.setFloat("iFlipAlbedo", flipAlbedoFloat);
            deferredPass.setBool("iPBR", pbr);

            activateAndBindTexturesTexSpaceFramebuffer(textureSpaceFramebuffer, deferredPass, textureWrapParamsDecalOutputs, textureSampleParamsDecalOutputs, textureArray, !true);

            deferredPass.setVec3("viewPos", camPos);
            deferredPass.setVec3("lightPos", light.position);
            deferredPass.setFloat("iTime", iTime);
            deferredPass.setInt("iFlipper", flipper);
            deferredPass.setInt("iNormals", enableNormals);
            deferredPass.setFloat("iTime", iTime);

            /*glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);*/

            glDrawElements(drawingMode, (GLsizei)modelData[i].indexes.size(), GL_UNSIGNED_INT, 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(0);

        /** End Light Pass **/
    
        if (showBBox)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glEnable(GL_DEPTH_TEST);

            hitPosition.use();
            hitPosition.setMat4("model", modelData[0].modelMatrix);
            hitPosition.setMat4("projection", projectionHalf);
            hitPosition.setMat4("view", view);
            hitPosition.setVec3("color", glm::vec3(0.0f, 0.0f, 1.0f));
            renderLineCube(modelData[0].Bbox.bboxMin, modelData[0].Bbox.bboxMax);
        }

        // Dummy modelData, it will not be modified given that we have already cached.
        ModelData modelDataDummy;
        if (showHitPoint)
        {
            glm::mat4 hitPositionModel = modelData[0].modelMatrix;
            hitPositionModel = glm::translate(hitPositionModel, modelData[0].bvo.hitPos);
            hitPositionModel = glm::scale(hitPositionModel, glm::vec3(0.01f));

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glEnable(GL_DEPTH_TEST);

            hitPosition.use();
            hitPosition.setMat4("model", hitPositionModel);
            hitPosition.setMat4("projection", projectionHalf);
            hitPosition.setMat4("view", view);
            hitPosition.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
            //renderSphere(modelDataDummy);
            renderCube();
        }

        if (showProjector)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glEnable(GL_DEPTH_TEST);

            hitPosition.use();
            hitPosition.setMat4("model", modelData[0].modelMatrix);
            hitPosition.setMat4("projection", projectionHalf);
            hitPosition.setMat4("view", view);
            hitPosition.setVec3("color", glm::vec3(0.0f, 1.0f, 0.0f));
            renderFrustum(modelData[0].bvo.decalProjector);
        }

        if (light.toggle)
        {
            glm::mat4 lightPositionModel = light.model;
            lightPositionModel = glm::scale(lightPositionModel, glm::vec3(0.05f));

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glEnable(GL_DEPTH_TEST);

            hitPosition.use();
            hitPosition.setMat4("model", lightPositionModel);
            hitPosition.setMat4("projection", projectionHalf);
            hitPosition.setMat4("view", view);
            hitPosition.setVec3("color", light.color);
            //renderSphere(modelDataDummy);
            renderCube();
        }

        if (splitScreen)
        {
            for (uint16_t i = 0u; i < modelData.size(); ++i)
            {
                // Side View.
                glViewport(WIDTH / 2, 0, WIDTH / 2, HEIGHT / 2);

                // Scale for the side view.
                glm::mat4 modelSide = glm::scale(modelNoGuizmo, glm::vec3(m_cameraState.zoomSide, m_cameraState.zoomSide, m_cameraState.zoomSide));
                modelSide = glm::rotate(modelSide, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                //modelSide = glm::translate(modelSide, glm::vec3(0.0f, -0.5f, 0.0f) * differenceBboxMaxMin);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glBindVertexArray(modelData[i].openGLObject.VAO);
                deferredPass.use();
                deferredPass.setMat4("model", modelSide);
                deferredPass.setMat4("projection", projectionSide);
                deferredPass.setMat4("view", viewPinnedBottom);/*glm::mat4(-0.113205, -0.187880, 0.975646, 0.000000,
                                                        0.000000, 0.981959, 0.189095, 0.000000,
                                                        -0.993572, 0.021407, -0.111162, 0.000000,
                                                        -0.064962, 0.388481, -4.221102, 1.000000));*/
                deferredPass.setFloat("iFlipAlbedo", flipAlbedoFloat);

                activateAndBindTexturesTexSpaceFramebuffer(textureSpaceFramebuffer, deferredPass, textureWrapParamsDecalOutputs, textureSampleParamsDecalOutputs, textureArray, !true);

                deferredPass.setVec3("viewPos", camPos);
                deferredPass.setVec3("lightPos", light.position);
                deferredPass.setFloat("iTime", iTime);
                deferredPass.setInt("iFlipper", flipper);
                deferredPass.setInt("iNormals", enableNormals);
                deferredPass.setFloat("iTime", iTime);

            
                glDrawElements(drawingMode, (GLsizei)modelData[i].indexes.size(), GL_UNSIGNED_INT, 0);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glBindVertexArray(0);
            }

            for (uint16_t i = 0u; i < modelData.size(); ++i)
            {

                // Front View.
                glViewport(WIDTH / 2, HEIGHT / 2, WIDTH / 2, HEIGHT / 2);

                // Scale for the side view.
                glm::mat4 modelTop = glm::scale(modelNoGuizmo, glm::vec3(m_cameraState.zoomTop, m_cameraState.zoomTop, m_cameraState.zoomTop));
                //modelTop = glm::translate(modelTop, glm::vec3(0.0f, -0.5f, 0.5f) * differenceBboxMaxMin);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glBindVertexArray(modelData[i].openGLObject.VAO);
                deferredPass.use();
                deferredPass.setMat4("model", modelTop);
                deferredPass.setMat4("projection", projectionSide);
                deferredPass.setMat4("view", viewPinnedTop);/*glm::mat4(1.0, 0.0,  0.0, 0.0,
                                                        0.0, 1.0,  0.0, 0.0,
                                                        0.0, 0.0,  1.0, 0.0,
                                                        0.0, 0.0, -1.0, 1.0));*/
                deferredPass.setFloat("iFlipAlbedo", flipAlbedoFloat);

                activateAndBindTexturesTexSpaceFramebuffer(textureSpaceFramebuffer, deferredPass, textureWrapParamsDecalOutputs, textureSampleParamsDecalOutputs, textureArray, !true);

                deferredPass.setVec3("viewPos", camPos);
                deferredPass.setVec3("lightPos", light.position);
                deferredPass.setFloat("iTime", iTime);
                deferredPass.setInt("iFlipper", flipper);
                deferredPass.setInt("iNormals", enableNormals);
                deferredPass.setFloat("iTime", iTime);

                glDrawElements(drawingMode, (GLsizei)modelData[i].indexes.size(), GL_UNSIGNED_INT, 0);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glBindVertexArray(0);
            }
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);

    SDL_PumpEvents();  // make sure we have the lascale mouse state.

    SDL_GL_GetDrawableSize(window, &WIDTH, &HEIGHT);

    iTime += 1.0f / 100.0f;
    frame++;
    frameJFA++;
    counter++;
    frameIsEven = !frameIsEven;
}

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include "common/debug.h"
#include "common/programobject.h"
#include "common/objloader.h"
#include "common/meshbuffer.h"
#include "common/meshobject.h"
#include "common/renderable.h"

using namespace std;
using namespace ogle;

std::string DataDirectory; // ends with a forward slash
int WINDOW_WIDTH = 1024;
int WINDOW_HEIGHT = 1024;

int RENDER_WIDTH = 1024;
int RENDER_HEIGHT = 1024;

GLFWwindow* glfwWindow;

glm::mat4x4 Camera;
glm::mat4x4 SceneBase;
glm::mat4x4 Projection;
glm::mat4x4 ProjectionView;
glm::mat3x3 NormalMatrix;

glm::vec3 CameraTarget;
glm::vec3 CameraPosition;
glm::vec3 CameraUp;

bool MousePositionCapture = false;
glm::vec2 PrevMousePos;

MeshObject VenousModel;
ProgramObject VenousShader;

MeshObject ArtModel;
ProgramObject ArtShader;

MeshObject FustrumModel;
ProgramObject FustrumShader;

MeshObject ValvesModel;

const int LayersCount = 16;

ProgramObject CreateDepthVolume;
GLuint FustrumVolume = 0;
GLuint CavityVolume = 0;
GLuint CounterTexture = 0;
GLuint FustrumFramebuffer = 0;
GLuint RenderFramebuffer = 0;
GLuint RenderSurface = 0;

GLuint ValvesCounterTexture = 0;
GLuint ValvesVolume = 0;

MeshObject Quad;
ProgramObject DisplayFustrumVolume;

ProgramObject CollectDepthsShader;
ProgramObject SortDepthsShader;
ProgramObject ClearImagesShader;

ProgramObject FustrumClipShader;	// the fustrum clips against the rest of the anatomy
ProgramObject CavityClipShader;		// interior models clip against the fustrum

ProgramObject WindowBlit;

const glm::vec4 ColorGradient0(255/255.f, 180/255.f, 50/255.f, 1.f);
const glm::vec4 ColorGradient1(50/255.f, 180/255.f, 255/255.f, 1.f);
const glm::vec4 ColorMinimum(.5f, .5f, .5f, 1.f);
const glm::vec2 ColorDepthRange(320.f, 380.f);

void errorCallback(int error, const char* description)
{
    cerr << description << endl;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE){
        MousePositionCapture = false;
        glfwSetCursorPosCallback(glfwWindow, nullptr);
    }

    if (key == GLFW_KEY_P && action == GLFW_RELEASE){
    }
}

void cursorCallback(GLFWwindow* window, double x, double y)
{
    if (MousePositionCapture){
        glm::vec2 curr = glm::vec2(float(x), float(y));
        glm::vec2 diff = curr - PrevMousePos;
        PrevMousePos = curr;

        glm::vec3 rotation_axis = glm::vec3(-diff.y, -diff.x, 0.f);
        float mag = glm::length(rotation_axis);
        if (mag == 0) return;

        rotation_axis = rotation_axis / mag; // normalize

        glm::vec3 view_vec = CameraPosition - CameraTarget;
        view_vec = glm::rotate(view_vec, mag * 0.001f, rotation_axis);
        
        CameraPosition = CameraTarget + view_vec;

        glm::vec3 view_dir = glm::normalize(view_vec);
        glm::vec3 tmp = glm::cross(CameraUp, view_dir);
        tmp = glm::normalize(tmp);
        CameraUp = glm::normalize(glm::cross(view_dir, tmp));

        Camera = glm::lookAt(CameraPosition, CameraTarget, CameraUp);
    }
}

void mouseCallback(GLFWwindow* window, int btn, int action, int mods)
{
    if (mods == GLFW_MOD_CONTROL){
        if (btn == GLFW_MOUSE_BUTTON_1) {
            if(action == GLFW_PRESS) {
                double x,y;
                glfwGetCursorPos(glfwWindow, &x, &y);
                PrevMousePos = glm::vec2(float(x), float(y));
                MousePositionCapture = true;

                glfwSetCursorPosCallback(glfwWindow, cursorCallback);
            }
            if(action == GLFW_RELEASE) {
                MousePositionCapture = false;
                glfwSetCursorPosCallback(glfwWindow, nullptr);
            }
        }
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    glm::vec3 view_vec = CameraPosition - CameraTarget;

    float dist = glm::length(view_vec);
    view_vec /= dist;

    dist -= 10.f * float(yoffset);
    CameraPosition = CameraTarget + view_vec * dist;

    Camera = glm::lookAt(CameraPosition, CameraTarget, CameraUp);
}

void initGLFW(){
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    glfwWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OIT", NULL, NULL);
    if (!glfwWindow)
    {
        fprintf(stderr, "Failed to create GLFW glfwWindow\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    } 

    glfwMakeContextCurrent(glfwWindow);
    glfwSwapInterval( 0 ); // Turn off vsync for benchmarking.

    glfwSetTime( 0.0 );
    glfwSetKeyCallback(glfwWindow, keyCallback);
    glfwSetErrorCallback(errorCallback);
    glfwSetMouseButtonCallback(glfwWindow, mouseCallback);
    glfwSetScrollCallback(glfwWindow, scrollCallback);
}

void initGLAD(){
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "initGLAD: Failed to initialize OpenGL context" << std::endl;
        exit(EXIT_FAILURE);
    }

    int width,height;
    glfwGetFramebufferSize(glfwWindow, &width, &height);
    glViewport( 0, 0, (GLsizei)width, (GLsizei)height );    
}

void setDataDir(int argc, char *argv[]){
    // get base directory for reading in files
	DataDirectory = "../oit/data/";
}

void initFustrum(){
    // load up mesh
    ogle::ObjLoader loader;
    loader.load(DataDirectory + "fustrum.obj");

    MeshBuffer buffer;
    buffer.setVerts(loader.getVertCount(), loader.getPositions());
    buffer.setNorms(loader.getVertCount(), loader.getNormals());
    buffer.setIndices(loader.getIndexCount(), loader.getIndices());
    buffer.generateFaceNormals();
    FustrumModel.init(buffer);

    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "diffuse.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "diffuse.frag";
    FustrumShader.init(shaders);

    shaders.clear();
    shaders[GL_VERTEX_SHADER] = DataDirectory + "depth.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "thickness.frag";
    CreateDepthVolume.init(shaders);

	shaders.clear();
	shaders[GL_VERTEX_SHADER] = DataDirectory + "diffuseClip.vert";
	shaders[GL_FRAGMENT_SHADER] = DataDirectory + "diffuseClipAgainstCavities.frag";
	FustrumClipShader.init(shaders);
}

void initArt(){
    // load up mesh
    ogle::ObjLoader loader;
    loader.load(DataDirectory + "art.obj");

    MeshBuffer buffer;
    buffer.setVerts(loader.getVertCount(), loader.getPositions());
    buffer.setNorms(loader.getVertCount(), loader.getNormals());
    buffer.setIndices(loader.getIndexCount(), loader.getIndices());
    buffer.generateFaceNormals();
    ArtModel.init(buffer);

    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "diffuse.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "diffuse.frag";
    ArtShader.init(shaders);
}

void initVenous(){
    // load up mesh
    ogle::ObjLoader loader;
    loader.load(DataDirectory + "venous.obj");

    MeshBuffer buffer;
    buffer.setVerts(loader.getVertCount(), loader.getPositions());
    buffer.setNorms(loader.getVertCount(), loader.getNormals());
    buffer.setIndices(loader.getIndexCount(), loader.getIndices());
    buffer.generateFaceNormals();
    VenousModel.init(buffer);

    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "diffuse.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "diffuse.frag";
    VenousShader.init(shaders);
}

void initCollectDepths() {
    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "collectDepths.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "collectDepths.frag";
    CollectDepthsShader.init(shaders);
}

void initSortDepths() {
    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "quad.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "sortDepth.frag";
    SortDepthsShader.init(shaders);
}

void initClipAgainstFustrum() {
    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "diffuseClip.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "diffuseClipAgainstFustrum.frag";
    CavityClipShader.init(shaders);
}

void initView(){
    float fovy = glm::radians(30.f);
    Projection = glm::perspective<float>(fovy, RENDER_WIDTH/(float)RENDER_HEIGHT, 0.1f, 1000.0f );

    float opposite = (VenousModel.AABBMax.y - VenousModel.AABBMin.y);
    float adjacent = opposite / atan(fovy * .5f);

    CameraUp = glm::vec3(0,1,0);
    CameraTarget = VenousModel.PivotPoint;
    glm::vec3 direction(0,0,1);
    CameraPosition = VenousModel.PivotPoint + direction * adjacent * 1.10f;
    Camera = glm::lookAt(CameraPosition, CameraTarget, CameraUp);
}

void createTextures() {
    glGenTextures(1, &RenderSurface);
    glBindTexture(GL_TEXTURE_2D, RenderSurface);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &FustrumVolume);
    glBindTexture(GL_TEXTURE_2D, FustrumVolume);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RG, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &CavityVolume);
    glBindTexture(GL_TEXTURE_2D_ARRAY, CavityVolume);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RG16F, RENDER_WIDTH, RENDER_HEIGHT, LayersCount, 0, GL_RG, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glGenTextures(1, &CounterTexture);
    glBindTexture(GL_TEXTURE_2D, CounterTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &ValvesCounterTexture);
    glBindTexture(GL_TEXTURE_2D, ValvesCounterTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &ValvesVolume);
    glBindTexture(GL_TEXTURE_2D_ARRAY, ValvesVolume);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RG16F, RENDER_WIDTH, RENDER_HEIGHT, LayersCount, 0, GL_RG, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glBindImageTexture(1, CavityVolume, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RG16F);
    glBindImageTexture(2, CounterTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

    glBindImageTexture(3, ValvesVolume, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RG16F);
    glBindImageTexture(4, ValvesCounterTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
}

void createFrambuffer() {
    glGenFramebuffers(1, &FustrumFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, FustrumFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FustrumVolume, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &RenderFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, RenderFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, RenderSurface, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initQuad() {

    std::vector<glm::vec3> positions = {
        glm::vec3(-1, -1, -1),  
        glm::vec3( 1, -1, -1),  
        glm::vec3( 1,  1, -1),  
        glm::vec3(-1,  1, -1),  
    };
    std::vector<glm::vec2> texs = {
        glm::vec2(0, 0),
        glm::vec2(1, 0),
        glm::vec2(1, 1),
        glm::vec2(0, 1),
    };

    std::vector<uint32_t> indices = {
       0,1,2, 2,3,0 
    };

    MeshBuffer buffer;
    buffer.setVerts(4, (const float*)&positions.data()[0]);
    buffer.setTexCoords(0, 4, (const float*)&texs.data()[0]);
    buffer.setIndices(6, indices.data());
    Quad.init(buffer);

    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "quad.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "displayThickness.frag";
    DisplayFustrumVolume.init(shaders);
}

void initClearImagesShader() {
    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "quad.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "clearImages.frag";
    ClearImagesShader.init(shaders);
}

void initValvesModel() {
    // load up mesh
    ogle::ObjLoader loader;
    loader.load(DataDirectory + "valves.obj");

    MeshBuffer buffer;
    buffer.setVerts(loader.getVertCount(), loader.getPositions());
    buffer.setNorms(loader.getVertCount(), loader.getNormals());
    buffer.setIndices(loader.getIndexCount(), loader.getIndices());
    buffer.generateFaceNormals();
    ValvesModel.init(buffer);
}

void initWindowBlit() {
    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "quad.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "blit.frag";
    WindowBlit.init(shaders);
}

void init(int argc, char* argv[]){
    setDataDir(argc, argv);
    initGLFW();
    initGLAD();
    ogle::Debug::init();

    initVenous();
    initArt();
    initFustrum();
    initView();

    createTextures();
    createFrambuffer();

    initQuad();
    initClearImagesShader();
    initCollectDepths();
    initSortDepths();
    initClipAgainstFustrum();
    initValvesModel();

    initWindowBlit();
}

void update(){
    ProjectionView = Projection * Camera;
    // NormalMatrix = glm

    static std::vector<float> deltas;
    static float sum_deltas = 0;

    float deltaTime = (float)glfwGetTime(); // get's the amount of time since last setTime
    deltas.push_back(deltaTime);
    sum_deltas += deltaTime;

    glfwSetTime(0);

    if (sum_deltas > .5f) {
        sum_deltas = 0;

        float ave = 0;
        for (auto& d: deltas){
            ave += d;
        }
        ave /= deltas.size();
        deltas.clear();

    	int fps = int(1 / ave);
    	float milliseconds = ave * 1000.f;
    	string title = "OIT - FPS (" + std::to_string(fps) + ") / " + std::to_string(milliseconds) + "ms";
        glfwSetWindowTitle(glfwWindow, title.c_str());
    }
}

void defaultRenderState() {
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void renderFustrumDiffuse(){
    FustrumShader.bind();
    FustrumShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");
    auto color = glm::vec4(1,0,1,1);
    FustrumShader.setVec4((const float*)&color, "Color");
    FustrumModel.render();
}

void renderArtModelDiffuse(){
    ArtShader.bind();
    ArtShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");
    auto color = glm::vec4(1,1,0,1);
    ArtShader.setVec4((const float*)&color, "Color");
    ArtModel.render();
}

void renderVenousModelDiffuse(){
    VenousShader.bind();
    VenousShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");
    auto color = glm::vec4(1,0,0,1);
    VenousShader.setVec4((const float*)&color, "Color");
    VenousModel.render();
}

void renderFustrumToFrameBuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, FustrumFramebuffer);
    glClearColor( 0, 0, 0, 0 );
    glClear( GL_COLOR_BUFFER_BIT );
    
    glBlendFunc(GL_ONE, GL_ONE);
    
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    glDisable(GL_CULL_FACE);

    CreateDepthVolume.bind();
    CreateDepthVolume.setMatrix44((const float*)&ProjectionView, "ProjectionView");
    FustrumModel.render();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderCavitiesToImages() {    
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    CollectDepthsShader.bind();
    CollectDepthsShader.setInt(1, "CavityVolume");
    CollectDepthsShader.setInt(2, "Counter");
    CollectDepthsShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");

    VenousModel.render();
    ArtModel.render();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

}

void renderValvesToImages() {    
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    CollectDepthsShader.bind();
    CollectDepthsShader.setInt(3, "CavityVolume");
    CollectDepthsShader.setInt(4, "Counter");
    CollectDepthsShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");

    ValvesModel.render();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

}

void debugImages() {
    const uint32_t count = RENDER_WIDTH * RENDER_HEIGHT;
    uint32_t *pixels = new uint32_t[count];
	glBindTexture(GL_TEXTURE_2D, CounterTexture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, (GLvoid*)&pixels[0]);
	glBindTexture(GL_TEXTURE_2D, 0);

    uint32_t loc = 0;
    uint32_t max=0;
    for (uint32_t i=0; i<count; ++i){
        if (pixels[i] > max){
            max = pixels[i];
            loc = i;
        }
    }
    delete [] pixels;

    glm::vec2 *depths = new glm::vec2[count * LayersCount];
    glBindTexture(GL_TEXTURE_2D_ARRAY, CavityVolume);
    glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RG, GL_FLOAT, (GLvoid*)&depths[0]);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    cout << "Layers used: " << max << endl;
    for (int layer=0; layer<max; ++layer){
        int layered_index = (layer * count) + loc;
        cout << "\tLayer " << layer << " val: " << glm::to_string(depths[layered_index]) << endl;
    }

    delete [] depths;
}

void clearImages() {
    ClearImagesShader.bind();
    ClearImagesShader.setInt(1, "CavityVolume");
    ClearImagesShader.setInt(2, "Counter");
    ClearImagesShader.setInt(3, "ValvesVolume");
    ClearImagesShader.setInt(4, "ValvesCounter");
    Quad.render();
}

void sortCavityDepths() {
    SortDepthsShader.bind();
    SortDepthsShader.setInt(1, "CavityVolume");
    SortDepthsShader.setInt(2, "Counter");
    Quad.render();    
}

void sortValveDepths() {
    SortDepthsShader.bind();
    SortDepthsShader.setInt(3, "CavityVolume");
    SortDepthsShader.setInt(4, "Counter");
    Quad.render();    
}

void renderAndClipFustrum() {
    FustrumClipShader.bind();
    FustrumClipShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");

    FustrumClipShader.setVec4((const float*)&ColorGradient0, "ColorGradient0");
    FustrumClipShader.setVec4((const float*)&ColorGradient1, "ColorGradient1");
    FustrumClipShader.setVec4((const float*)&ColorMinimum, "ColorMinimum");
    FustrumClipShader.setVec2((const float*)&ColorDepthRange, "ColorDepthRange");
    FustrumClipShader.setInt(1, "CavityVolume");
    FustrumClipShader.setInt(2, "Counter");
    FustrumClipShader.setInt(3, "ValvesVolume");
    FustrumClipShader.setInt(4, "ValvesCounter");

    FustrumModel.render();
}

void renderFustrumThickness() {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, FustrumVolume);

    DisplayFustrumVolume.bind();
    Quad.render();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderAndClipCavities() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, FustrumVolume);

    CavityClipShader.bind();
    CavityClipShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");

    CavityClipShader.setVec4((const float*)&ColorGradient0, "ColorGradient0");
    CavityClipShader.setVec4((const float*)&ColorGradient1, "ColorGradient1");
    CavityClipShader.setVec4((const float*)&ColorMinimum, "ColorMinimum");
    CavityClipShader.setVec2((const float*)&ColorDepthRange, "ColorDepthRange");
    CavityClipShader.setVec2((const float*)&glm::vec2(RENDER_WIDTH, RENDER_HEIGHT), "Resolution");
    VenousModel.render();
    ArtModel.render();

    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderAndClipValves() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, FustrumVolume);

    CavityClipShader.bind();
    CavityClipShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");

    CavityClipShader.setVec4((const float*)&ColorGradient0, "ColorGradient0");
    CavityClipShader.setVec4((const float*)&ColorGradient1, "ColorGradient1");
    CavityClipShader.setVec4((const float*)&ColorMinimum, "ColorMinimum");
    CavityClipShader.setVec2((const float*)&ColorDepthRange, "ColorDepthRange");
    CavityClipShader.setVec2((const float*)&glm::vec2(RENDER_WIDTH, RENDER_HEIGHT), "Resolution");
    ValvesModel.render();

    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderValvesDiffuse() {
    FustrumShader.bind();
    FustrumShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");
    auto color = glm::vec4(1,0,1,1);
    FustrumShader.setVec4((const float*)&color, "Color");
    ValvesModel.render();
}

void renderToScreen() {
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, RenderSurface);
    WindowBlit.bind();
    Quad.render();
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_BLEND);
}

void render(){
    defaultRenderState();
    renderFustrumToFrameBuffer();

    defaultRenderState();
    glBindFramebuffer(GL_FRAMEBUFFER, RenderFramebuffer);
    glClearColor( 0,0,0,0 );
    glClearDepth( 1 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    clearImages();
    renderCavitiesToImages();
    sortCavityDepths();
    // renderValvesToImages();
    // sortValveDepths();
    // debugImages();

    // renderFustrumThickness();
    renderAndClipCavities();
    renderAndClipFustrum();
    renderAndClipValves();

    // renderVenousModelDiffuse();
    // renderArtModelDiffuse();
    // renderFustrumDiffuse();
    // renderValvesDiffuse();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor( 0,0,0,0 );
    glClearDepth( 1 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    renderToScreen();
}

void runloop(){
    glfwSetTime(0); // init timer
    while (!glfwWindowShouldClose(glfwWindow)){
        glfwPollEvents();
        update();
        render();
        glfwSwapBuffers(glfwWindow);
    }
}

void shutdown(){
    WindowBlit.shutdown();
    FustrumClipShader.shutdown();
    CavityClipShader.shutdown();

    glDeleteTextures(1, &ValvesCounterTexture);
    glDeleteTextures(1, &ValvesVolume);

    ClearImagesShader.shutdown();

    CollectDepthsShader.shutdown();
    SortDepthsShader.shutdown();

    Quad.shutdown();
    DisplayFustrumVolume.shutdown();

    glDeleteTextures(1, &RenderSurface);

    glDeleteFramebuffers(1, &FustrumFramebuffer);
    glDeleteFramebuffers(1, &RenderFramebuffer);

    glDeleteTextures(1, &FustrumVolume);
    glDeleteTextures(1, &CavityVolume);

    FustrumModel.shutdown();
    FustrumShader.shutdown();
    CreateDepthVolume.shutdown();

    ArtModel.shutdown();
    ArtShader.shutdown();

    VenousShader.shutdown();
    VenousModel.shutdown();


    ogle::Debug::shutdown();
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();    
}

int main(int argc, char* argv[]){
    init(argc, argv);
    runloop();
    shutdown();
    exit(EXIT_SUCCESS);
}

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>

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
GLFWwindow* glfwWindow;

glm::mat4 Camera;
glm::mat4 RotationMatrix;
glm::mat4 SceneBase;

glm::mat4 Projection;
glm::mat4 ProjectionView;
glm::mat3 NormalMatrix;

glm::vec3 MoveToOrigin;

glm::vec3 SpinVector;
glm::mat4 SpinRotationMatrix;
glm::mat4 FrustumMatrix;

glm::vec3 CameraTarget;
glm::vec3 CameraPosition;
glm::vec3 CameraUp;

bool MousePositionCapture = false;
glm::vec2 PrevMousePos;

bool FrustumAnimated = true;
float FrustumAnimationValue = 0;
float AnimationModifier = 1.f;
float AnimationModifierStep = .1f;
bool ToggleDebugCavities = false;

MeshObject ArtModel;
ProgramObject ArtShader;

MeshObject FrustumModel;
ProgramObject FrustumShader;

const int LayersCount = 16;

ProgramObject CreateDepthVolume;
GLuint FrustumVolume = 0;
GLuint CavityVolume = 0;
GLuint CounterTexture = 0;
GLuint FrustumFramebuffer = 0;

GLuint ValvesCounterTexture = 0;
GLuint ValvesVolume = 0;

MeshObject Quad;
ProgramObject DisplayFrustumVolume;

ProgramObject CollectDepthsShader;
ProgramObject SortDepthsShader;
ProgramObject ClearImagesShader;

ProgramObject FrustumClipShader;	// the frustum clips against the rest of the anatomy
ProgramObject CavityClipShader;		// interior models clip against the frustum

const int AnatomyFrameCount = 22;
MeshBuffer AnimatedAnatomyFrames[AnatomyFrameCount];
MeshBuffer AnimatedAnatomy;
float AnimatedAnatomyCurrent = 0;
float AnimatedAnatomyDuration = 1;

const int FrustumFrameCount = 2;
MeshBuffer AnimatedFrustumFrames[FrustumFrameCount];
MeshBuffer AnimatedFrustum;

const glm::vec4 ColorGradient0(241/255.f, 219/255.f, 142/255.f, 1.f);
const glm::vec4 ColorGradient1(45/255.f, 135/255.f, 219/255.f, 1.f);
const glm::vec4 ColorMinimum(60/255.f, 14.f/255, 1.f/255.f, 1.f);
glm::vec2 ColorDepthRange(320.f, 380.f);

void errorCallback(int error, const char* description)
{
    cerr << description << endl;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_P && action == GLFW_RELEASE){
    }

    // control frustum
    if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
        FrustumAnimated = !FrustumAnimated;
    }
    if (key == GLFW_KEY_UP){
        FrustumAnimated = false;
        FrustumAnimationValue += AnimationModifierStep;
    }
    if (key == GLFW_KEY_DOWN){
        FrustumAnimated = false;
        FrustumAnimationValue -= AnimationModifierStep;
    }
    // Spin the frustum
    if (key == GLFW_KEY_LEFT){
        glm::mat4 tmpRotation = glm::mat4(1.f);
        tmpRotation = glm::rotate(tmpRotation, -.05f, SpinVector);
        SpinRotationMatrix = tmpRotation * SpinRotationMatrix;
    }
    if (key == GLFW_KEY_RIGHT){
        glm::mat4 tmpRotation = glm::mat4(1.f);
        tmpRotation = glm::rotate(tmpRotation, .05f, SpinVector);
        SpinRotationMatrix = tmpRotation * SpinRotationMatrix;
    }

    // enable debug view of cavities
    if (key == GLFW_KEY_ENTER && action == GLFW_RELEASE) {
        ToggleDebugCavities = !ToggleDebugCavities;
    }
}

void cursorCallback(GLFWwindow* window, double x, double y)
{
    if (MousePositionCapture){
        glm::vec2 curr = glm::vec2(float(x), float(y));
        glm::vec2 diff = curr - PrevMousePos;
        PrevMousePos = curr;

        glm::vec3 rotation_axis = glm::vec3(-diff.y, diff.x, 0.f);
        float mag = glm::length(rotation_axis);
        if (mag == 0) return;

        rotation_axis = rotation_axis / mag; // normalize

        glm::vec3 view_vec = CameraPosition - CameraTarget;
        view_vec = glm::rotate(view_vec, mag * 0.001f, rotation_axis);
        
        CameraPosition = CameraTarget + view_vec;

        glm::vec3 view_dir = glm::normalize(view_vec);
        glm::vec3 tmp = glm::cross(CameraUp, view_dir);
        tmp = glm::normalize(tmp);
        //CameraUp = glm::normalize(glm::cross(view_dir, tmp));

        //Camera = glm::lookAt(CameraPosition, CameraTarget, CameraUp);
        glm::mat4 tmpRotation = glm::mat4(1.f);
        tmpRotation = glm::rotate(tmpRotation, .05f, rotation_axis);
        RotationMatrix = tmpRotation * RotationMatrix;
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
        }
    }

    if (btn == GLFW_MOUSE_BUTTON_1) {
        if (action == GLFW_RELEASE) {
            MousePositionCapture = false;
            glfwSetCursorPosCallback(glfwWindow, nullptr);
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
/*
    if (glewInit() != GLEW_OK)
    {
        std::cout << "initGlew!!: Failed to load opengl functions" << std::endl;
        exit(EXIT_FAILURE);
    }*/

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

void initFrustum(){

    ogle::ObjLoader loaderA;
    std::string fileNameA = DataDirectory + "Heart Interior Simple/3D_TEE_Frustum_MitralPosition.obj";
    loaderA.load(fileNameA);
    AnimatedFrustumFrames[0].setVerts(loaderA.getVertCount(), loaderA.getPositions());
    AnimatedFrustumFrames[0].setIndices(loaderA.getIndexCount(), loaderA.getIndices());

    ogle::ObjLoader loaderB;
    std::string fileNameB = DataDirectory + "Heart Interior Simple/3D_TEE_Frustum_TricuspidPosition.obj";
    loaderB.load(fileNameB);
    AnimatedFrustumFrames[1].setVerts(loaderB.getVertCount(), loaderB.getPositions());
    AnimatedFrustumFrames[1].setIndices(loaderB.getIndexCount(), loaderB.getIndices());

    AnimatedFrustum = AnimatedFrustumFrames[0];
    AnimatedFrustum.generateFaceNormals();
    FrustumModel.init(AnimatedFrustum);

    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "diffuse.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "diffuse.frag";
    FrustumShader.init(shaders);

    shaders.clear();
    shaders[GL_VERTEX_SHADER] = DataDirectory + "depth.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "thickness.frag";
    CreateDepthVolume.init(shaders);

	shaders.clear();
	shaders[GL_VERTEX_SHADER] = DataDirectory + "depth.vert";
	shaders[GL_FRAGMENT_SHADER] = DataDirectory + "diffuseClipAgainstCavities.frag";
	FrustumClipShader.init(shaders);

    MeshObject objectTmp;
    objectTmp.init(AnimatedFrustumFrames[1]);
    glm::vec3 start = FrustumModel.PivotPoint;
    glm::vec3 end = objectTmp.PivotPoint;
    glm::vec3 travelVector = (end - start) * 1.5f;

    SpinVector = glm::cross(travelVector, glm::vec3(0, 1, 0));
}

void initArt(){

    //loader.load(DataDirectory + "art.obj");
    for (int i = 0; i < AnatomyFrameCount; ++i) {
        ogle::ObjLoader loader;

        std::stringstream ss;
        ss << setfill('0') << std::setw(2) << (i + 1);
        std::string fileName = DataDirectory + "Heart Interior Simple/Heart_Interior_withValves_v02_f" + ss.str() + ".obj";
        loader.load(fileName);

        AnimatedAnatomyFrames[i].setVerts(loader.getVertCount(), loader.getPositions());
        AnimatedAnatomyFrames[i].setIndices(loader.getIndexCount(), loader.getIndices());
    }
    AnimatedAnatomy = AnimatedAnatomyFrames[0];
    AnimatedAnatomy.generateFaceNormals();
    ArtModel.init(AnimatedAnatomy);
    //ArtModel.updateBuffers(AnimatedAnatomy);

    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "diffuse.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "diffuse.frag";
    ArtShader.init(shaders);
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

void initClipAgainstFrustum() {
    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "depth.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "diffuseClipAgainstFrustum.frag";
    CavityClipShader.init(shaders);
}

void initView(){
    float fovy = glm::radians(30.f);
    Projection = glm::perspective<float>(fovy, WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 1000.0f );

    auto& model = FrustumModel;

    float opposite = (model.AABBMax.y - model.AABBMin.y);
    float adjacent = opposite / atan(fovy * .5f);

    CameraUp = glm::vec3(0,1,0);
    CameraTarget = model.PivotPoint;
    glm::vec3 direction(0,0,1);
    CameraPosition = model.PivotPoint + -direction * adjacent * 1.10f;
    Camera = glm::lookAt(CameraPosition, CameraTarget, CameraUp);

    MoveToOrigin = glm::vec4(CameraTarget, 1);
}

void createTextures() {
    glGenTextures(1, &FrustumVolume);
    glBindTexture(GL_TEXTURE_2D, FrustumVolume);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RG, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &CavityVolume);
    glBindTexture(GL_TEXTURE_2D_ARRAY, CavityVolume);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RG16F, WINDOW_WIDTH, WINDOW_HEIGHT, LayersCount, 0, GL_RG, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glGenTextures(1, &CounterTexture);
    glBindTexture(GL_TEXTURE_2D, CounterTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &ValvesCounterTexture);
    glBindTexture(GL_TEXTURE_2D, ValvesCounterTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &ValvesVolume);
    glBindTexture(GL_TEXTURE_2D_ARRAY, ValvesVolume);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RG16F, WINDOW_WIDTH, WINDOW_HEIGHT, LayersCount, 0, GL_RG, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glBindImageTexture(1, CavityVolume, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RG16F);
    glBindImageTexture(2, CounterTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

    glBindImageTexture(3, ValvesVolume, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RG16F);
    glBindImageTexture(4, ValvesCounterTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
}

void createFrambuffer() {
    glGenFramebuffers(1, &FrustumFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, FrustumFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FrustumVolume, 0);
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
    DisplayFrustumVolume.init(shaders);
}

void initClearImagesShader() {
    std::map<unsigned int, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "quad.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "clearImages.frag";
    ClearImagesShader.init(shaders);
}

void initSceneFrustumMatrices()
{
	SceneBase = glm::mat4(1.0f);
    RotationMatrix = glm::mat4(1.0f);
    SpinRotationMatrix = glm::mat4(1.0f);
    FrustumMatrix = glm::mat4(1.f);
}

void init(int argc, char* argv[]){
    setDataDir(argc, argv);
    initGLFW();
    initGLAD();
    ogle::Debug::init();

	initSceneFrustumMatrices();

    initArt();
    initFrustum();
    initView();

    createTextures();
    createFrambuffer();

    initQuad();
    initClearImagesShader();
    initCollectDepths();
    initSortDepths();
    initClipAgainstFrustum();
}

void update(){
    ProjectionView = Projection * Camera;

    float colorCenter = glm::length(MoveToOrigin - CameraPosition);
    ColorDepthRange = glm::vec2(colorCenter - 25, colorCenter + 25);

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

    // animate frustum
    {
        float percent = cos(FrustumAnimationValue);
        percent += 1.f; // move from [-1,1] to [0,2]
        percent *= .5;  // move from [0,2] to [0,1]

        if (FrustumAnimated) {
            FrustumAnimationValue += deltaTime * AnimationModifier;
        }

        const std::vector<glm::vec3>& vertsA = AnimatedFrustumFrames[0].getVerts();
        const std::vector<glm::vec3>& vertsB = AnimatedFrustumFrames[1].getVerts();

        size_t count = vertsA.size();
        std::vector<glm::vec3> animatedVerts(count);
        for (size_t i = 0; i < count; ++i) {
            animatedVerts[i] = percent * (vertsB[i] - vertsA[i]) + vertsA[i];
        }
        AnimatedFrustum.setVerts(count, (const float*)animatedVerts.data());
        AnimatedFrustum.generateFaceNormals();
        FrustumModel.updateBuffers(AnimatedFrustum);
    }

    FrustumMatrix = RotationMatrix * SpinRotationMatrix;

    // animate anatomy
    {
        AnimatedAnatomyCurrent += deltaTime;
        AnimatedAnatomyCurrent = std::fmod(AnimatedAnatomyCurrent, AnimatedAnatomyDuration);
        float percent = AnimatedAnatomyCurrent / AnimatedAnatomyDuration;

        float frame = percent * (AnatomyFrameCount - 1);

        float fframeA;
        float tween = modf(frame, &fframeA);

        int frameA = int(fframeA);
        int frameB = (frameA + 1) % int(AnatomyFrameCount);

        MeshBuffer meshA = AnimatedAnatomyFrames[frameA];
        MeshBuffer meshB = AnimatedAnatomyFrames[frameB];

        const std::vector<glm::vec3>& vertsA = meshA.getVerts();
        const std::vector<glm::vec3>& vertsB = meshB.getVerts();
 
        size_t count = vertsA.size();
        std::vector<glm::vec3> animatedVerts(count);
        for (size_t i = 0; i < count; ++i) {
            animatedVerts[i] = tween * (vertsB[i] - vertsA[i]) + vertsA[i];
        }
        AnimatedAnatomy.setVerts(count, (const float*)animatedVerts.data());
        AnimatedAnatomy.generateFaceNormals();
        ArtModel.updateBuffers(AnimatedAnatomy);
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

void renderArtModelDiffuse(){
    ArtShader.bind();
    ArtShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");
    ArtShader.setVec4((const float*)&MoveToOrigin, "MoveToOrigin");
    ArtShader.setMatrix44((const float*)&RotationMatrix, "RotationMatrix");
    
    auto color = glm::vec4(.5f,0,0,.5);
    ArtShader.setVec4((const float*)&color, "Color");
    ArtModel.render();
}

void renderFrustumToFrameBuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, FrustumFramebuffer);
    glClearColor( 0, 0, 0, 0 );
    glClear( GL_COLOR_BUFFER_BIT );
    
    glBlendFunc(GL_ONE, GL_ONE);
    
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    glDisable(GL_CULL_FACE);

	CreateDepthVolume.bind();
    CreateDepthVolume.setVec4((const float*)&MoveToOrigin, "MoveToOrigin");
    CreateDepthVolume.setMatrix44((const float*)&FrustumMatrix, "RotationMatrix");
    CreateDepthVolume.setMatrix44((const float*)&ProjectionView, "ProjectionView");
    FrustumModel.render();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderCavitiesToImages() {    
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    CollectDepthsShader.bind();
    CollectDepthsShader.setInt(1, "CavityVolume");
    CollectDepthsShader.setInt(2, "Counter");
    CollectDepthsShader.setVec4((const float*)&MoveToOrigin, "MoveToOrigin");
    CollectDepthsShader.setMatrix44((const float*)&RotationMatrix, "RotationMatrix");
    CollectDepthsShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");

    //VenousModel.render();
    ArtModel.render();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

}

void debugImages() {
    const uint32_t count = WINDOW_WIDTH * WINDOW_HEIGHT;
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

void renderAndClipFrustum() {
    FrustumClipShader.bind();
    FrustumClipShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");
    FrustumClipShader.setVec4((const float*)&MoveToOrigin, "MoveToOrigin");
    FrustumClipShader.setMatrix44((const float*)&FrustumMatrix, "RotationMatrix");

    FrustumClipShader.setVec4((const float*)&ColorGradient0, "ColorGradient0");
    FrustumClipShader.setVec4((const float*)&ColorGradient1, "ColorGradient1");
    FrustumClipShader.setVec4((const float*)&ColorMinimum, "ColorMinimum");
    FrustumClipShader.setVec2((const float*)&ColorDepthRange, "ColorDepthRange");
    FrustumClipShader.setInt(1, "CavityVolume");
    FrustumClipShader.setInt(2, "Counter");
    FrustumClipShader.setInt(3, "ValvesVolume");
    FrustumClipShader.setInt(4, "ValvesCounter");

    FrustumModel.render();
}

void renderFrustumThickness() {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, FrustumVolume);

    DisplayFrustumVolume.bind();
    Quad.render();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderAndClipCavities() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, FrustumVolume);

    CavityClipShader.bind();
    CavityClipShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");
    CavityClipShader.setVec4((const float*)&MoveToOrigin, "MoveToOrigin");
    CavityClipShader.setMatrix44((const float*)&RotationMatrix, "RotationMatrix");

    CavityClipShader.setVec4((const float*)&ColorGradient0, "ColorGradient0");
    CavityClipShader.setVec4((const float*)&ColorGradient1, "ColorGradient1");
    CavityClipShader.setVec4((const float*)&ColorMinimum, "ColorMinimum");
    CavityClipShader.setVec2((const float*)&ColorDepthRange, "ColorDepthRange");
    glm::vec2 res = glm::vec2(WINDOW_WIDTH, WINDOW_HEIGHT);
    CavityClipShader.setVec2((const float*)&res, "Resolution");
    ArtModel.render();

    glBindTexture(GL_TEXTURE_2D, 0);
}

void render(){
    defaultRenderState();
    renderFrustumToFrameBuffer();

    defaultRenderState();
    glClearColor( 0,0,0,0 );
    glClearDepth( 1 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    clearImages();
    renderCavitiesToImages();
    sortCavityDepths();
    // debugImages();

    // renderFrustumThickness();
    renderAndClipCavities();
    renderAndClipFrustum();

    if (ToggleDebugCavities){
        renderArtModelDiffuse();
    }
    // renderFrustumDiffuse();
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
    FrustumClipShader.shutdown();
    CavityClipShader.shutdown();

    glDeleteTextures(1, &ValvesCounterTexture);
    glDeleteTextures(1, &ValvesVolume);

    ClearImagesShader.shutdown();

    CollectDepthsShader.shutdown();
    SortDepthsShader.shutdown();

    Quad.shutdown();
    DisplayFrustumVolume.shutdown();

    glDeleteFramebuffers(1, &FrustumFramebuffer);

    glDeleteTextures(1, &FrustumVolume);
    glDeleteTextures(1, &CavityVolume);

    FrustumModel.shutdown();
    FrustumShader.shutdown();
    CreateDepthVolume.shutdown();

    ArtModel.shutdown();
    ArtShader.shutdown();

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

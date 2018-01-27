#include <iostream>
#include <algorithm>
#include <iomanip>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/random.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "common/debug.h"
#include "common/programobject.h"
#include "common/objloader.h"
#include "common/meshbuffer.h"
#include "common/meshobject.h"
#include "common/renderable.h"

#include <pnglite.h>

using namespace std;
using namespace ogle;

std::string DataDirectory; // ends with a forward slash
int WINDOW_WIDTH = 1024;
int WINDOW_HEIGHT = 1024;
GLFWwindow* glfwWindow;

glm::mat4x4 Camera;
glm::mat4x4 SceneBase;
glm::mat4x4 Projection;
glm::mat4x4 ProjectionView;

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
        png_t out_img = {0};

        int error = 0;
        error = png_open_file_write(&out_img, "screen_shot.png");

        int width = WINDOW_WIDTH;
        int height = WINDOW_HEIGHT;
        int comp_count = 4;
        int bytes_per = 1;
        unsigned char* data = new unsigned char[width * height * comp_count * bytes_per];
        glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, data);

        int bits_per_pixel = 8;
        error = png_set_data(&out_img, width, height, bits_per_pixel, PNG_TRUECOLOR_ALPHA, data);
        error = png_close_file(&out_img);
        delete [] data; data = 0;
    }
}

void cursorCallback(GLFWwindow* window, double x, double y)
{
    if (MousePositionCapture){
        glm::vec2 curr = glm::vec2(float(x), float(y));
        glm::vec2 diff = curr - PrevMousePos;
        PrevMousePos = curr;

        glm::vec3 rotation_axis = glm::vec3(diff.y, -diff.x, 0.f);
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
    if (mods == GLFW_MOD_ALT){
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    glfwWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Boids", NULL, NULL);
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
    DataDirectory = "./data/";
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

void initView(){
    float fovy = glm::radians(30.f);
    Projection = glm::perspective<float>(fovy, WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 1000.0f );

    float opposite = (VenousModel.AABBMax.y - VenousModel.AABBMin.y);
    float adjacent = opposite / atan(fovy * .5f);

    CameraUp = glm::vec3(0,1,0);
    CameraTarget = VenousModel.PivotPoint;
    glm::vec3 direction(0,0,1);
    CameraPosition = VenousModel.PivotPoint + direction * adjacent;
    Camera = glm::lookAt(CameraPosition, CameraTarget, CameraUp);

    ProjectionView = Projection * Camera;
    cout << "Point " << glm::to_string(VenousModel.PivotPoint) << endl;

    auto mvp = Projection * glm::vec4(VenousModel.PivotPoint, 1);
    cout << "MVP " << glm::to_string(mvp) << " z/-w " << (mvp.z/-mvp.w) << endl;
}

void init(int argc, char* argv[]){
    setDataDir(argc, argv);
    initGLFW();
    initGLAD();
    ogle::Debug::init();
    png_init(0,0);

    initVenous();
    initArt();
    initFustrum();
    initView();
}

void update(){
    float deltaTime = (float)glfwGetTime(); // get's the amount of time since last setTime
    glfwSetTime(0);

    ProjectionView = Projection * Camera;
}

void renderFustrumDiffuse(){
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    FustrumShader.bind();
    FustrumShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");
    FustrumModel.render();
}

void renderArtModelDiffuse(){
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    ArtShader.bind();
    ArtShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");
    ArtModel.render();
}

void renderVenousModelDiffuse(){
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    VenousShader.bind();
    VenousShader.setMatrix44((const float*)&ProjectionView, "ProjectionView");
    VenousModel.render();
}

void render(){
    glClearColor( 0,0,0,0 );
    glClearDepth( 1 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    renderVenousModelDiffuse();
    renderArtModelDiffuse();
    renderFustrumDiffuse();
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
    FustrumModel.shutdown();
    FustrumShader.shutdown();

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

#include <iostream>
#include <fstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Forward Declaration
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
int init(GLFWwindow*& window);
void createShaders();
void createGeometryProgram(GLuint& programID, const char* vertex, const char* fragment, const char* geometry);
void createProgram(GLuint& programID, const char* vertex, const char* fragment);
GLuint loadTexture(const char* path, int comp = 0);
GLuint loadSkyboxTexture();
void loadFile(const char* filename, char*& output);
void renderModel(Model* model, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);
void renderModelNormals(Model* model, GLuint spikeTex);

GLuint simpleProgram, modelProgram;

const int WIDTH = 1280, HEIGHT = 720;

// Camera parameters
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 15.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.05f;
float mouseSensitivity = 0.1f;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

glm::vec3 lightPosition = glm::normalize(glm::vec3(-0.5f, -0.5f, -0.5f));

glm::mat4 view;
glm::mat4 projection = glm::perspective(glm::radians(45.0f), WIDTH / (float)HEIGHT, 0.1f, 5000.0f);

float triangleSize = 5000;

Model* backpack;

int main() {
    GLFWwindow* window;
    int res = init(window);
    if (res != 0) return res;

    createShaders();
    GLuint spikeTex = loadTexture("textures/Spike.png");


    stbi_set_flip_vertically_on_load(true);
    backpack = new Model("models/backpack/backpack.obj");
    stbi_set_flip_vertically_on_load(false);

    glViewport(0, 0, WIDTH, HEIGHT);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderModel(backpack, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
        renderModelNormals(backpack, spikeTex);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    float moveSpeed = 0.2f;
    float rotationSpeed = glm::radians(1.0f);

    //move with WASD
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPosition += moveSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPosition -= moveSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPosition -= glm::normalize(glm::cross(cameraFront, cameraUp)) * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPosition += glm::normalize(glm::cross(cameraFront, cameraUp)) * moveSpeed;

    //move up with space
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPosition += moveSpeed * cameraUp;

    //move down with left shift
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPosition -= moveSpeed * cameraUp;

    //escape to exut window
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static double lastX = WIDTH / 2.0;
    static double lastY = HEIGHT / 2.0;
    static bool firstMouse = true;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    yaw += xOffset;
    pitch += yOffset;

    // Clamp pitch to avoid flipping issues
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Update cameraFront based on yaw and pitch
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

int init(GLFWwindow*& window) {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "OPENGLproject", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    return 0;
}

void createShaders() {
    createProgram(modelProgram, "shaders/modelVertex.shader", "shaders/modelFragment.shader");

    glUseProgram(modelProgram);

    glUniform1i(glGetUniformLocation(modelProgram, "texture_diffuse1"), 0);
    glUniform1i(glGetUniformLocation(modelProgram, "texture_specular1"), 1);
    glUniform1i(glGetUniformLocation(modelProgram, "texture_normal1"), 2);
    glUniform1i(glGetUniformLocation(modelProgram, "texture_roughness1"), 3);
    glUniform1i(glGetUniformLocation(modelProgram, "texture_ao1"), 4);

    createGeometryProgram(simpleProgram, "shaders/normalVisualVertex.shader", "shaders/normalVisualFragment.shader", "shaders/normalVisualGeometry.shader");

    glUseProgram(simpleProgram);

    glUniform1i(glGetUniformLocation(simpleProgram, "spikeTex"), 5);
}

void createGeometryProgram(GLuint& programID, const char* vertex, const char* fragment, const char* geometry) {
    // Create a GL Program with a vertex & fragment shader
    char* vertexSrc;
    char* fragmentSrc;
    char* geometrySrc;

    loadFile(vertex, vertexSrc);
    loadFile(fragment, fragmentSrc);
    loadFile(geometry, geometrySrc);

    GLuint vertexShaderID, fragmentShaderID, geometryShaderID;

    vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderID, 1, &vertexSrc, nullptr);
    glCompileShader(vertexShaderID);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShaderID, 512, nullptr, infoLog);
        std::cout << "ERROR COMPILING VERTEX SHADER\n" << infoLog << std::endl;
    }

    fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderID, 1, &fragmentSrc, nullptr);
    glCompileShader(fragmentShaderID);

    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShaderID, 512, nullptr, infoLog);
        std::cout << "ERROR COMPILING FRAGMENT SHADER\n" << infoLog << std::endl;
    }

    geometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometryShaderID, 1, &geometrySrc, nullptr);
    glCompileShader(geometryShaderID);

    glGetShaderiv(geometryShaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(geometryShaderID, 512, nullptr, infoLog);
        std::cout << "ERROR COMPILING GEOMETRY SHADER\n" << infoLog << std::endl;
    }

    programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glAttachShader(programID, geometryShaderID);
    glLinkProgram(programID);

    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(programID, 512, nullptr, infoLog);
        std::cout << "ERROR LINKING PROGRAM\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
    glDeleteShader(geometryShaderID);

    delete vertexSrc;
    delete fragmentSrc;
    delete geometrySrc;

}

void createProgram(GLuint& programID, const char* vertex, const char* fragment) {
    // Create a GL Program with a vertex & fragment shader
    char* vertexSrc;
    char* fragmentSrc;

    loadFile(vertex, vertexSrc);
    loadFile(fragment, fragmentSrc);

    GLuint vertexShaderID, fragmentShaderID;

    vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderID, 1, &vertexSrc, nullptr);
    glCompileShader(vertexShaderID);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShaderID, 512, nullptr, infoLog);
        std::cout << "ERROR COMPILING VERTEX SHADER\n" << infoLog << std::endl;
    }

    fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderID, 1, &fragmentSrc, nullptr);
    glCompileShader(fragmentShaderID);

    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShaderID, 512, nullptr, infoLog);
        std::cout << "ERROR COMPILING FRAGMENT SHADER\n" << infoLog << std::endl;
    }

    programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);

    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(programID, 512, nullptr, infoLog);
        std::cout << "ERROR LINKING PROGRAM\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    delete vertexSrc;
    delete fragmentSrc;

}

void loadFile(const char* filename, char*& output) {
    std::ifstream file(filename, std::ios::binary);

    if (file.is_open()) {
        file.seekg(0, file.end);
        int length = file.tellg();
        file.seekg(0, file.beg);

        output = new char[length + 1];

        file.read(output, length);

        output[length] = '\0';

        file.close();
    }
    else {
        output = NULL;
    }
}

GLuint loadTexture(const char* path, int comp)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, numChannels;
    unsigned char* data = stbi_load(path, &width, &height, &numChannels, comp);
    if (data) {
        if (comp != 0) numChannels = comp;

        if (numChannels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        else if (numChannels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);


        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Error loading texture: " << path << std::endl;
    }

    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;
}

GLuint loadSkyboxTexture() {
    // Load and configure skybox cubemap
    GLuint skyboxTexture;
    glGenTextures(1, &skyboxTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

    const char* skyboxFaces[] = {
        "textures/skybox/right.jpg",
        "textures/skybox/left.jpg",
        "textures/skybox/top.jpg",
        "textures/skybox/bottom.jpg",
        "textures/skybox/front.jpg",
        "textures/skybox/back.jpg"
    };

    int width, height, numChannels;
    for (GLuint i = 0; i < 6; ++i) {
        unsigned char* data = stbi_load(skyboxFaces[i], &width, &height, &numChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cout << "Failed to load skybox texture: " << skyboxFaces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return skyboxTexture;
}

void renderModel(Model* model, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glUseProgram(modelProgram);

    glm::mat4 world = glm::mat4(1.0f);
    world = glm::translate(world, pos);
    world = glm::scale(world, scale);

    glUniformMatrix4fv(glGetUniformLocation(modelProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(glGetUniformLocation(modelProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(modelProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(glGetUniformLocation(modelProgram, "lightDirection"), 1, glm::value_ptr(lightPosition));
    glUniform3fv(glGetUniformLocation(modelProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    model->Draw(modelProgram);
} 

void renderModelNormals(Model* model, GLuint spikeTex) {
    glUseProgram(simpleProgram);

    glm::mat4 world = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "model"), 1, GL_FALSE, glm::value_ptr(world));

    glUniform1f(glGetUniformLocation(simpleProgram, "trianglesize"), triangleSize);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, spikeTex);

    model->Draw(simpleProgram);
}
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
void createProgram(GLuint& programID, const char* vertex, const char* fragment);
void createGeometry(GLuint& vao, GLuint& EBO, int& size, int& numTriangles);
GLuint loadTexture(const char* path, int comp = 0);
GLuint loadSkyboxTexture();
void createSkyboxGeometry(GLuint& VAO, GLuint& VBO);
void renderSkybox(GLuint skyboxVAO, GLuint skyboxTex, glm::mat4 view, glm::mat4 projection);
void renderBox(GLuint cubeVAO, GLuint boxTex, GLuint boxNormal, glm::mat4 cubeModel, glm::mat4 view, glm::mat4 projection, int& cubeNumIndices);
void loadFile(const char* filename, char*& output);
unsigned int GeneratePlane(const char* heightmap, unsigned char*& data, GLenum format, int comp, float hScale, float xzScale, unsigned int& indexCount, unsigned int& heightmapID);
void renderTerrain();
void renderModel(Model* model, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);

GLuint simpleProgram, skyboxProgram, terrainProgram, modelProgram;

const int WIDTH = 1280, HEIGHT = 720;

// Camera parameters
glm::vec3 cameraPosition = glm::vec3(60.0f, 60.0f, 60.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.05f;
float mouseSensitivity = 0.1f;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

glm::mat4 cubeModel = glm::mat4(1.0f);


GLuint dirt, sand, grass, rock, snow;

glm::vec3 lightPosition = glm::normalize(glm::vec3( - 0.5f, -0.5f, -0.5f));

glm::mat4 view;
glm::mat4 projection = glm::perspective(glm::radians(45.0f), WIDTH / (float)HEIGHT, 0.1f, 5000.0f);

Model* backpack;

//Terrain data
GLuint terrainVAO, terrainIndexCount, heightmapID, heightNormalID;
unsigned char* heightmapTexture;

int main() {
    GLFWwindow* window;
    int res = init(window);
    if (res != 0) return res;
    

    createShaders();

    GLuint skyboxTex = loadSkyboxTexture();
    GLuint skyboxVAO, skyboxVBO;
    createSkyboxGeometry(skyboxVAO, skyboxVBO);

    GLuint cubeVAO, cubeVBO, cubeEBO;
    int cubeSize, cubeNumIndices;

    GLuint boxTex = loadTexture("textures/container2.png");
    GLuint boxNormal = loadTexture("textures/container2_normal.png");
    createGeometry(cubeVAO, cubeEBO, cubeSize, cubeNumIndices);

    terrainVAO = GeneratePlane("textures/heightMap.png", heightmapTexture, GL_RGBA, 4, 100.0f, 5.0f, terrainIndexCount, heightmapID);
    heightNormalID = loadTexture("textures/heightnormal.png");

    dirt = loadTexture("textures/dirt.jpg");
    snow = loadTexture("textures/snow.jpg");
    sand = loadTexture("textures/sand.jpg");
    rock = loadTexture("textures/rock.jpg");
    grass = loadTexture("textures/grass.png", 4);

    stbi_set_flip_vertically_on_load(true);
    backpack = new Model("models/backpack/backpack.obj");
    stbi_set_flip_vertically_on_load(false);

    glViewport(0, 0, WIDTH, HEIGHT);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderSkybox(skyboxVAO, skyboxTex, view, projection);

        renderTerrain();

        renderModel(backpack, glm::vec3(100, 100, 100), glm::vec3(0, 0 , 0), glm::vec3(10, 10 ,10));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    float moveSpeed = 0.8f;
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

    //handle cube rotation with arrow keys
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cubeModel = glm::rotate(cubeModel, rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cubeModel = glm::rotate(cubeModel, -rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        cubeModel = glm::rotate(cubeModel, rotationSpeed, glm::vec3(1.0f, 0.0f, 0.0f));
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        cubeModel = glm::rotate(cubeModel, -rotationSpeed, glm::vec3(1.0f, 0.0f, 0.0f));

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

unsigned int GeneratePlane(const char* heightmap, unsigned char* &data, GLenum format, int comp, float hScale, float xzScale, unsigned int& indexCount, unsigned int& heightmapID) {
    int width, height, channels;
    data = nullptr;
    if (heightmap != nullptr) {
        data = stbi_load(heightmap, &width, &height, &channels, comp);
        if (data) {
            glGenTextures(1, &heightmapID);
            glBindTexture(GL_TEXTURE_2D, heightmapID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    int stride = 8;
    float* vertices = new float[(width * height) * stride];
    unsigned int* indices = new unsigned int[(width - 1) * (height - 1) * 6];

    int index = 0;
    for (int i = 0; i < (width * height); i++) {
        int x = i % width;
        int z = i / width;

        float texHeight = (float)data[i * comp];

        vertices[index++] = x * xzScale;
        vertices[index++] = (texHeight / 255.0f) * hScale;
        vertices[index++] = z * xzScale;

        vertices[index++] = 0;
        vertices[index++] = 1;
        vertices[index++] = 0;

        vertices[index++] = x / (float)width;
        vertices[index++] = z / (float)height;


    }

    index = 0;
    for (int i = 0; i < (width - 1) * (height - 1); i++) {
        int x = i % (width - 1);
        int z = i / (width - 1);

        int vertex = z * width + x;

        indices[index++] = vertex;
        indices[index++] = vertex + width;
        indices[index++] = vertex + width + 1;

        indices[index++] = vertex;
        indices[index++] = vertex + width + 1;
        indices[index++] = vertex + 1;


    }

    unsigned int vertSize = (width * height) * stride * sizeof(float);
    indexCount = ((width - 1) * (height - 1) * 6);

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertSize, vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

    // vertex information!
    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, 0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
    // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * 6));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    delete[] vertices;
    delete[] indices;

    return VAO;
}

void createShaders() {
    createProgram(skyboxProgram, "shaders/skyVertex.shader", "shaders/skyFragment.shader");
    createProgram(simpleProgram, "shaders/simpleVertex.shader", "shaders/simpleFragment.shader");

    glUseProgram(simpleProgram);
    glUniform1i(glGetUniformLocation(simpleProgram, "mainTex"), 0);
    glUniform1i(glGetUniformLocation(simpleProgram, "normalTex"), 1);

    createProgram(terrainProgram, "shaders/terrainVertex.shader", "shaders/terrainFragment.shader");


    glUseProgram(terrainProgram);
    glUniform1i(glGetUniformLocation(terrainProgram, "terrainTex"), 0);
    glUniform1i(glGetUniformLocation(terrainProgram, "normalTex"), 1);

    glUniform1i(glGetUniformLocation(terrainProgram, "dirt"), 2);
    glUniform1i(glGetUniformLocation(terrainProgram, "sand"), 3);
    glUniform1i(glGetUniformLocation(terrainProgram, "grass"), 4);
    glUniform1i(glGetUniformLocation(terrainProgram, "rock"), 5);
    glUniform1i(glGetUniformLocation(terrainProgram, "snow"), 6);

    createProgram(modelProgram, "shaders/model.vs", "shaders/model.fs");

    glUseProgram(modelProgram);

    glUniform1i(glGetUniformLocation(modelProgram, "texture_diffuse1"), 0);
    glUniform1i(glGetUniformLocation(modelProgram, "texture_specular1"), 1);
    glUniform1i(glGetUniformLocation(modelProgram, "texture_normal1"), 2);
    glUniform1i(glGetUniformLocation(modelProgram, "texture_roughness1"), 3);
    glUniform1i(glGetUniformLocation(modelProgram, "texture_ao1"), 4);
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

void createSkyboxGeometry(GLuint& VAO, GLuint& VBO) {
    float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f

    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void createGeometry(GLuint& VAO, GLuint& EBO, int& size, int& numIndices) {
    // need 24 vertices for normal/uv-mapped Cube
    float vertices[] = {
        // positions            //colors            // tex coords   // normals          //tangents      //bitangents
        0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   1.f, 1.f,       0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
        0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
        -0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
        -0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       0.f, -1.f, 0.f,     -1.f, 0.f, 0.f,  0.f, 0.f, 1.f,

        0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,
        0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   1.f, 0.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,

        0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,
        -0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,

        -0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   0.f, 0.f,      -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,
        -0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   0.f, 1.f,      -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,

        -0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   0.f, 1.f,      0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,
        0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,      0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,

        -0.5f, 0.5f, -.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
        -0.5f, 0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f,

        0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   1.f, 1.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,
        -0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       0.f, 0.f, 1.f,     1.f, 0.f, 0.f,  0.f, -1.f, 0.f,

        -0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,   1.f, 0.f,       -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,
        -0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   1.f, 1.f,       -1.f, 0.f, 0.f,     0.f, 1.f, 0.f,  0.f, 0.f, 1.f,

        -0.5f, -0.5f, -.5f,     1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,
        0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   1.f, 0.f,       0.f, 0.f, -1.f,     1.f, 0.f, 0.f,  0.f, 1.f, 0.f,

        0.5f, -0.5f, -0.5f,     1.0f, 1.0f, 1.0f,   0.f, 1.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,
        0.5f, -0.5f, 0.5f,      1.0f, 1.0f, 1.0f,   0.f, 0.f,       1.f, 0.f, 0.f,     0.f, -1.f, 0.f,  0.f, 0.f, 1.f,

        0.5f, 0.5f, -0.5f,      1.0f, 1.0f, 1.0f,   0.f, 1.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f,
        0.5f, 0.5f, 0.5f,       1.0f, 1.0f, 1.0f,   0.f, 0.f,       0.f, 1.f, 0.f,     1.f, 0.f, 0.f,  0.f, 0.f, 1.f
    };

    unsigned int indices[] = {  // note that we start from 0!
        // DOWN
        0, 1, 2,   // first triangle
        0, 2, 3,    // second triangle
        // BACK
        14, 6, 7,   // first triangle
        14, 7, 15,    // second triangle
        // RIGHT
        20, 4, 5,   // first triangle
        20, 5, 21,    // second triangle
        // LEFT
        16, 8, 9,   // first triangle
        16, 9, 17,    // second triangle
        // FRONT
        18, 10, 11,   // first triangle
        18, 11, 19,    // second triangle
        // UP
        22, 12, 13,   // first triangle
        22, 13, 23,    // second triangle
    };

    int stride = (3 + 3 + 2 + 3 + 3 + 3) * sizeof(float);

    size = sizeof(vertices) / stride;
    numIndices = sizeof(indices) / sizeof(int);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_TRUE, stride, (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 3, GL_FLOAT, GL_TRUE, stride, (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 3, GL_FLOAT, GL_TRUE, stride, (void*)(14 * sizeof(float)));
    glEnableVertexAttribArray(5);
}

void renderSkybox(GLuint skyboxVAO, GLuint skyboxTex, glm::mat4 view, glm::mat4 projection) {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH);
    glDisable(GL_DEPTH_TEST);

    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
    glUseProgram(skyboxProgram);
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"), 1, GL_FALSE, glm::value_ptr(skyboxView));
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH);
}

void renderTerrain() {
    glEnable(GL_DEPTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glUseProgram(terrainProgram);

    glm::mat4 world = glm::mat4(1.0f);

    glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "world"), 1, GL_FALSE, glm::value_ptr(world));
    glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(terrainProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(glGetUniformLocation(terrainProgram, "lightDirection"), 1, glm::value_ptr(lightPosition));
    glUniform3fv(glGetUniformLocation(terrainProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightmapID);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, heightNormalID);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, dirt);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, sand);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, grass);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, rock);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, snow);


    glBindVertexArray(terrainVAO);
    glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, 0);


    glDisable(GL_DEPTH);
    glDisable(GL_DEPTH_TEST);
}

void renderBox(GLuint cubeVAO, GLuint boxTex, GLuint boxNormal, glm::mat4 cubeModel,  glm::mat4 view, glm::mat4 projection, int& cubeNumIndices) {
    glEnable(GL_DEPTH);

    glUseProgram(simpleProgram);
    glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "world"), 1, GL_FALSE, glm::value_ptr(cubeModel));
    glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(simpleProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(glGetUniformLocation(simpleProgram, "lightPosition"), 1, glm::value_ptr(lightPosition));
    glUniform3fv(glGetUniformLocation(simpleProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, boxTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, boxNormal);

    glBindVertexArray(cubeVAO);

    glDrawElements(GL_TRIANGLES, cubeNumIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDisable(GL_DEPTH);
    glDisable(GL_DEPTH_TEST);
}

void renderModel(Model* model, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
{
    glEnable(GL_DEPTH);
    glEnable(GL_DEPTH_TEST);
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
#pragma comment(lib, "legacy_stdio_definitions.lib")
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define WIDTH 512
#define HEIGHT 512

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Vec3 { float x, y, z; };

Vec3 operator-(const Vec3& a, const Vec3& b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}
Vec3 operator+(const Vec3& a, const Vec3& b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}
Vec3 operator*(float s, const Vec3& v) {
    return { s * v.x, s * v.y, s * v.z };
}

int gNumVertices;
int gNumTriangles;
int* gIndexBuffer;
std::vector<Vec3> gVertexBuffer;
std::vector<Vec3> gNormals;

void create_scene() {
    int width = 32, height = 16;
    gNumVertices = (height - 2) * width + 2;
    gNumTriangles = ((height - 3) * (width - 1) * 2) + 2 * (width - 1);

    gVertexBuffer.resize(gNumVertices);
    gNormals.resize(gNumVertices, { 0, 0, 0 });
    gIndexBuffer = new int[gNumTriangles * 3];

    int t = 0;
    for (int j = 1; j < height - 1; ++j)
        for (int i = 0; i < width; ++i) {
            float theta = (float)j / (height - 1) * M_PI;
            float phi = (float)i / (width - 1) * M_PI * 2;
            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = -sinf(theta) * sinf(phi);
            gVertexBuffer[t++] = { x, y, z };
        }
    gVertexBuffer[t++] = { 0, 1, 0 };
    gVertexBuffer[t++] = { 0, -1, 0 };

    t = 0;
    for (int j = 0; j < height - 3; ++j)
        for (int i = 0; i < width - 1; ++i) {
            gIndexBuffer[t++] = j * width + i;
            gIndexBuffer[t++] = (j + 1) * width + (i + 1);
            gIndexBuffer[t++] = j * width + (i + 1);
            gIndexBuffer[t++] = j * width + i;
            gIndexBuffer[t++] = (j + 1) * width + i;
            gIndexBuffer[t++] = (j + 1) * width + (i + 1);
        }
    for (int i = 0; i < width - 1; ++i) {
        gIndexBuffer[t++] = (height - 2) * width;
        gIndexBuffer[t++] = i;
        gIndexBuffer[t++] = i + 1;
        gIndexBuffer[t++] = (height - 2) * width + 1;
        gIndexBuffer[t++] = (height - 3) * width + (i + 1);
        gIndexBuffer[t++] = (height - 3) * width + i;
    }

    for (int i = 0; i < gNumTriangles; ++i) {
        int i0 = gIndexBuffer[i * 3];
        int i1 = gIndexBuffer[i * 3 + 1];
        int i2 = gIndexBuffer[i * 3 + 2];
        Vec3 a = gVertexBuffer[i1] - gVertexBuffer[i0];
        Vec3 b = gVertexBuffer[i2] - gVertexBuffer[i0];
        Vec3 n = { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
        float len = sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        n = { n.x / len, n.y / len, n.z / len };
        gNormals[i0] = gNormals[i0] + n;
        gNormals[i1] = gNormals[i1] + n;
        gNormals[i2] = gNormals[i2] + n;
    }
    for (int i = 0; i < gNumVertices; ++i) {
        float len = sqrt(gNormals[i].x * gNormals[i].x + gNormals[i].y * gNormals[i].y + gNormals[i].z * gNormals[i].z);
        gNormals[i] = { gNormals[i].x / len, gNormals[i].y / len, gNormals[i].z / len };
    }
}

std::string readFile(const char* filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int compileShader(unsigned int type, const std::string& source) {
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    return id;
}

unsigned int createShader(const std::string& vertexShader, const std::string& fragmentShader) {
    unsigned int program = glCreateProgram();
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

unsigned int VAO, VBO, NBO, EBO, shaderProgram;

void setupBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &NBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3) * gVertexBuffer.size(), gVertexBuffer.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3) * gNormals.size(), gNormals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * gNumTriangles * 3, gIndexBuffer, GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, gNumTriangles * 3, GL_UNSIGNED_INT, 0);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Phong Shading", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    create_scene();
    std::string vertexCode = readFile("Phong.vert");
    std::string fragmentCode = readFile("Phong.frag");
    shaderProgram = createShader(vertexCode, fragmentCode);
    setupBuffers();

    glUseProgram(shaderProgram);
    float model[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,-3,1
    };
    float view[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    float proj[16] = {
        1.0f,0,0,0,
        0,1.0f,0,0,
        0,0,-1.002,-1,
        0,0,-0.2002,0
    };
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, proj);

    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), -4.0f, 4.0f, -3.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), 0.0f, 0.0f, 0.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "ka"), 0.1f, 0.1f, 0.1f);
    glUniform3f(glGetUniformLocation(shaderProgram, "kd"), 0.2f, 0.6f, 0.3f);
    glUniform3f(glGetUniformLocation(shaderProgram, "ks"), 0.6f, 0.6f, 0.6f);
    glUniform3f(glGetUniformLocation(shaderProgram, "Ia"), 0.2f, 0.2f, 0.2f);
    glUniform3f(glGetUniformLocation(shaderProgram, "Il"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), 32.0f);

    while (!glfwWindowShouldClose(window)) {
        render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

// Phong.vert
/*
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
*/

// Phong.frag
/*
#version 330 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 ka, kd, ks;
uniform vec3 Ia, Il;
uniform float shininess;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float diff = max(dot(norm, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    vec3 ambient = ka * Ia;
    vec3 diffuse = kd * Il * diff;
    vec3 specular = ks * Il * spec;

    vec3 color = ambient + diffuse + specular;
    FragColor = vec4(color, 1.0);
}
*/

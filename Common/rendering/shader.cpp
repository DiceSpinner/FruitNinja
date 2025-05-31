#include <fstream>
#include <iostream>
#include <string>
#include "shader.hpp"
#include "glad/glad.h"
#include "glm/ext.hpp"

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    ID = glCreateProgram();

    std::ifstream vertexFile;
    std::ifstream fragFile;
    vertexFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vertexFile.open(vertexPath);
        fragFile.open(fragmentPath);
    }
    catch (std::ifstream::failure e) {
        std::cout << "Failure to open shader files\n";
        glDeleteProgram(ID);
        ID = 0;
        return;
    }

    std::string vertexShaderString = std::string(std::istreambuf_iterator<char>(vertexFile), std::istreambuf_iterator<char>());
    std::string fragShaderString = std::string(std::istreambuf_iterator<char>(fragFile), std::istreambuf_iterator<char>());
    const char* vertexShaderSource = vertexShaderString.c_str();
    const char* fragShaderSource = fragShaderString.c_str();

    auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glDeleteShader(vertexShader);
        std::cout << "Failed to compile vertex shader\n";
        glDeleteProgram(ID);
        ID = 0;
        return;
    }
    glAttachShader(ID, vertexShader);

    auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glDeleteShader(fragmentShader);
        glDeleteProgram(ID);
        ID = 0;
        std::cout << "Failed to compile fragment shader\n";
        return;
    }
    glAttachShader(ID, fragmentShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "Failed to link program\n";
        std::cout << infoLog << "\n";
        glDeleteProgram(ID);
        ID = 0;
    }
}

Shader::~Shader() {
    if (ID) {
        glDeleteProgram(ID);
    }
}

bool Shader::IsValid() const {
    return ID;
}

void Shader::Use() const {
    if (ID) {
        glUseProgram(ID);
    }
    else {
        std::cout << "The shader compilation failed, cannot use this shader" << std::endl;
    }
}
void Shader::SetBool(const std::string& name, bool value) const {
    if(ID) glUniform1i(glGetUniformLocation(ID, name.c_str()), (GLuint)value);
}

void Shader::SetInt(const std::string& name, int value) const {
    if(ID) glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const {
    if(ID) glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::SetVec4(const std::string& name, glm::vec4 value) const {
    if(ID) glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}
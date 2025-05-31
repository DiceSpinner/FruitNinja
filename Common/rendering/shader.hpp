#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <glm/glm.hpp>

class Shader
{ 
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath);
    Shader(const Shader& shader) = delete;
    Shader(Shader&& shader) = delete;
    Shader& operator = (const Shader& other) = delete;
    Shader& operator = (Shader&& other) = delete;
    ~Shader();

    bool IsValid() const;
    // use/activate the shader
    void Use() const;
    // utility uniform functions
    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec4(const std::string& name, glm::vec4 value) const;
};

#endif
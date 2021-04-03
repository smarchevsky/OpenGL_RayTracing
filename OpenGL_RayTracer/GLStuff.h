#pragma once
#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <iostream>
#include <unordered_map>
#include <vector>

#define LOG(text) std::cout << text << std::endl;
namespace fs = std::experimental::filesystem;

class Shader {
public:
    struct ShaderVariable {
        ShaderVariable(const Shader& parent, const char* name);
        template <class T>
        void set(const T& var) { }
        void set(glm::vec2 var);
        void set(glm::vec3 var);
        void set(glm::vec4 var);
		void set(glm::mat4 var);
        void set(float var);

    private:
        const Shader& parentShader;
        int location;
    };
    Shader(const fs::path& vPath, const fs::path& fPath, const fs::path& gPath = "");
    std::string codeFromPath(const fs::path& path);

    ShaderVariable getVariable(const char* varName) const { return ShaderVariable(*this, varName); }
    int getProgram() const { return m_program; }
    void bind();
    ~Shader();

private:
    int m_program = -1;
    int createShader(const char* shaderSource, int shaderType);
};

class Mesh {
public:
    Mesh(Mesh&& other) noexcept;
    Mesh(const Mesh& other) = delete;
    Mesh()
    {
        float quadSize = 1.f;
        std::vector<glm::vec2> vertices { { quadSize, quadSize }, { quadSize, -quadSize }, { -quadSize, -quadSize }, { -quadSize, quadSize } };
        std::vector<glm::ivec3> indices { { 0, 1, 3 }, { 1, 2, 3 } };
        this->Mesh::Mesh(vertices, indices);
    }

    template <class VertexType, class IndexType>
    Mesh(std::vector<VertexType> vertices, std::vector<IndexType> indices)
    {
        m_indexBatchSize = sizeof(IndexType) / sizeof(int);
        m_elementCount = m_indexBatchSize * indices.size();
        createMesh(vertices.size(), sizeof(VertexType), vertices.data(),
            indices.size(), sizeof(IndexType), indices.data());
    }

    void createMesh(size_t vertexArrayLength, size_t vertexSize, void* vertexPtr,
        size_t indicesArrayLength, size_t indexSize, void* indexPtr);
    void draw();
    ~Mesh();

private:
    unsigned int m_VBO {}, m_VAO {}, m_EBO {};
    size_t m_elementCount {};
    uint8_t m_indexBatchSize {};
};

typedef void* SDL_GLContext;
class Window {
public:
    Window(int width = 1280, int height = 768);
    ~Window();
    bool update();
    float getDeltaTime() { return m_deltaTime; }
    float getAspectRatio() { return (float)m_width / m_height; }
    glm::vec2 getMousePos();

    void bindAction(char symbol, bool press, std::function<void()> f) { m_actionMapping.insert({ combineAction(symbol, press), f }); };
    void clearAction(char symbol) { m_actionMapping.erase(combineAction(symbol, true)), m_actionMapping.erase(combineAction(symbol, false)); }
    uint16_t combineAction(char symbol, bool press) { return (uint16_t(press) << 8) + symbol; }

private:
    struct SDL_Window* window;
    struct SDL_Renderer* renderer;
    SDL_GLContext gl_context;
    int m_width, m_height;

    float m_deltaTime {};
    uint64_t NOW {}, LAST {};

    std::unordered_map<uint16_t, std::function<void()>> m_actionMapping;
};

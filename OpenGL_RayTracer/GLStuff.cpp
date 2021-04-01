#include "GLStuff.h"
#include <iostream>
#include <string>

#include <SDL.h>
#undef main
#include "glad/glad.h"

#pragma region shader
static const char* vertexShaderSource = "#version 330 core\n"
                                        "layout (location = 0) in vec3 aPos;\n"
                                        "uniform vec2 offset;\n"
                                        "void main()\n"
                                        "{\n"
                                        "   gl_Position = vec4(aPos.x + offset.x, aPos.y + offset.y, aPos.z, 1.0);\n"
                                        "}\0";

static const char* fragmentShaderSource = "#version 330 core\n"
                                          "out vec4 FragColor;\n"
                                          "uniform vec3 color = vec3(1.0f, 0.5f, 0.2f);\n"
                                          "void main()\n"
                                          "{\n"
                                          "   FragColor = vec4(color.x, color.y, color.z, 1.0f);\n"
                                          "}\n\0";

Shader::ShaderVariable::ShaderVariable(const Shader& parent, const char* name)
    : parentShader(parent)
    , location(glGetUniformLocation(parentShader.getProgram(), name))
{
}

void Shader::ShaderVariable::set(float var)
{
    glUniform1f(location, var);
}

void Shader::ShaderVariable::set(glm::vec2 var)
{
    glUniform2fv(location, 1, &var[0]);
}

void Shader::ShaderVariable::set(glm::vec3 var)
{
    glUniform3fv(location, 1, &var[0]);
}

void Shader::ShaderVariable::set(glm::vec4 var)
{
    glUniform4fv(location, 1, &var[0]);
}

Shader::Shader()
{
    m_shaderProgram = glCreateProgram();
    int vs = createShader(vertexShaderSource, GL_VERTEX_SHADER);
    int fs = createShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    glAttachShader(m_shaderProgram, vs);
    glAttachShader(m_shaderProgram, fs);
    glLinkProgram(m_shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::Shader(const std::string& path)
{

}

Shader::~Shader()
{
    glDeleteProgram(m_shaderProgram);
}

void Shader::bind()
{
    glUseProgram(m_shaderProgram);
}

int Shader::createShader(const char* shaderSource, int shaderType)
{
    int sh = glCreateShader(shaderType);
    glShaderSource(sh, 1, &shaderSource, NULL);
    glCompileShader(sh);
    int success;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        std::string shaderTypeName = shaderType == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT";
        glGetShaderInfoLog(sh, 512, NULL, infoLog);
        LOG("ERROR::SHADER::" << shaderTypeName << "::COMPILATION_FAILED\n"
                              << infoLog);
    }
    return sh;
}

#pragma endregion

#pragma region window
Window::Window(int width /*= 800*/, int height /*= 600*/)
    : m_width(width)
    , m_height(height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL2 video subsystem couldn't be initialized. Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    window = SDL_CreateWindow("Glad Sample", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_width, m_height, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr) {
        std::cerr << "SDL2 Renderer couldn't be created. Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    gl_context = SDL_GL_CreateContext(window);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize the OpenGL context." << std::endl;
        exit(1);
    }

    std::cout << "OpenGL version loaded: " << GLVersion.major << "."
              << GLVersion.minor << std::endl;

    NOW = SDL_GetPerformanceCounter();
    LAST = 0;
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glClearColor(0.08f, 0.08f, 0.1f, 1.f);
}

bool Window::update()
{
    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    m_deltaTime = (float)((NOW - LAST) / (double)SDL_GetPerformanceFrequency());

    SDL_GL_SwapWindow(window);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_Event event;
    SDL_PollEvent(&event);

    if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
        SDL_Log("Mouse Button 1 (left) is pressed.");
    }
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        uint16_t actionMap = combine(event.key.keysym.sym, event.type == SDL_KEYDOWN);
        auto found = m_actionMapping.find(actionMap);
        if (found != m_actionMapping.end()) {
            found->second();
        }
    }

    switch (event.type) {
    case SDL_QUIT:
        return false;

    case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            return false;
        }
    }
    return true;
}

Window::~Window()
{
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}

glm::vec2 Window::getMousePos()
{
    int x, y;
    auto mb = SDL_GetMouseState(&x, &y);
    glm::vec2 pos((float)x / m_width * 2.f - 1.f, 1.f - (float)y / m_height * 2.f);
    // printf("Mouse position: %f, %f, button: %d\n", pos.x, pos.y, mb);
    return pos;
}
#pragma endregion

#pragma region mesh

void Mesh::createMesh(size_t vertexArrayLength, size_t vertexSize, void* vertexPtr, size_t indicesArrayLength, size_t indexSize, void* indexPtr)
{
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexSize * vertexArrayLength, vertexPtr, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize * indicesArrayLength, indexPtr, GL_STATIC_DRAW);

    glVertexAttribPointer(0, (int)vertexSize / sizeof(float), GL_FLOAT, GL_FALSE, (int)vertexSize, (void*)0);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Mesh::Mesh(Mesh&& other) noexcept
{
    memcpy(this, &other, sizeof(*this));
    other.m_VBO = other.m_VAO = other.m_EBO = 0;
}

Mesh::~Mesh()
{
    if (m_VAO)
        glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO)
        glDeleteBuffers(1, &m_VBO);
    if (m_EBO)
        glDeleteBuffers(1, &m_EBO);
}

void Mesh::draw()
{
    glBindVertexArray(m_VAO);
    switch (m_indexBatchSize) {
    case 1:
        glDrawElements(GL_POINTS, (int)m_elementCount, GL_UNSIGNED_INT, 0);
        break;
    case 2:
        glDrawElements(GL_LINES, (int)m_elementCount, GL_UNSIGNED_INT, 0);
        break;
    case 3:
        glDrawElements(GL_TRIANGLES, (int)m_elementCount, GL_UNSIGNED_INT, 0);
        break;
    default:
        return;
    }
}

#pragma endregion

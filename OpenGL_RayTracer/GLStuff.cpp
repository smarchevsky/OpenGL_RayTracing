#include "GLStuff.h"
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>

#include <SDL.h>
#undef main
#include "glad/glad.h"

#pragma region shader

#pragma region shader_variable
Shader::ShaderVariable::ShaderVariable(const Shader& parent, const char* name)
    : parentShader(parent)
{
    updateLocation(name);
}

void Shader::ShaderVariable::updateLocation(const char* name)
{
    location = glGetUniformLocation(parentShader.getProgram(), name);
}

void Shader::ShaderVariable::set(float var) { glUniform1f(location, var); }
void Shader::ShaderVariable::set(glm::vec2 var) { glUniform2fv(location, 1, &var[0]); }
void Shader::ShaderVariable::set(glm::vec3 var) { glUniform3fv(location, 1, &var[0]); }
void Shader::ShaderVariable::set(glm::vec4 var) { glUniform4fv(location, 1, &var[0]); }
void Shader::ShaderVariable::set(glm::mat4 var) { glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(var)); }
void Shader::ShaderVariable::set(glm::vec3* pos, int num) { glUniform3fv(location, num, glm::value_ptr(pos[0])); }

#pragma endregion

static GLuint shaderFromCode(const std::string& code, GLint type)
{
    GLuint shader = glCreateShader(type);
    char const* VertexSourcePointer = code.c_str();
    glShaderSource(shader, 1, &VertexSourcePointer, nullptr);
    glCompileShader(shader);

    int InfoLogLength;
    GLint Result = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> errorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(shader, InfoLogLength, nullptr, &errorMessage[0]);
        std::cerr << &errorMessage[0] << std::endl;
    }
    return shader;
}

Shader::Shader(const fs::path& vPath, const fs::path& fPath, const fs::path& gPath)
{
    reloadProgram(vPath, fPath, gPath);
}

void Shader::reloadProgram(const fs::path& vPath, const fs::path& fPath, const fs::path& gPath /*= ""*/)
{
    destroyProgram();
    auto vertexShader = shaderFromCode(codeFromPath(vPath), GL_VERTEX_SHADER);
    auto fragmentShader = shaderFromCode(codeFromPath(fPath), GL_FRAGMENT_SHADER);
    auto geometryShader = 0;
    bool geomShaderExists = !gPath.empty();
    if (geomShaderExists)
        geometryShader = shaderFromCode(codeFromPath(gPath), GL_GEOMETRY_SHADER);

    // Link the program
    m_program = glCreateProgram();
    glAttachShader(m_program, vertexShader);
    glAttachShader(m_program, fragmentShader);
    if (geomShaderExists)
        glAttachShader(m_program, geometryShader);

    glLinkProgram(m_program);

    int InfoLogLength;
    GLint Result = GL_FALSE;
    glGetProgramiv(m_program, GL_LINK_STATUS, &Result);
    glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(m_program, InfoLogLength, nullptr, &ProgramErrorMessage[0]);
        throw std::string(&ProgramErrorMessage[0]);
    }

    glDetachShader(m_program, vertexShader);
    glDetachShader(m_program, fragmentShader);

    if (geomShaderExists) {
        glDetachShader(m_program, geometryShader);
        glDeleteShader(geometryShader);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    bind();
}

std::string Shader::codeFromPath(const fs::path& path)
{
    std::string code;
    std::ifstream shaderStream(path, std::ios::in);
    if (shaderStream.is_open()) {
        std::stringstream sstr;
        sstr << shaderStream.rdbuf();
        code = sstr.str();
        shaderStream.close();
        return code;
    }
    throw std::string("Bad shader path: " + path.string());
}

void Shader::bind() { glUseProgram(m_program); }
Shader::~Shader() { destroyProgram(); }

void Shader::destroyProgram()
{
    if (m_program != -1)
        glDeleteProgram(m_program);
}

#pragma endregion

#pragma region window

Window::Window(int width /*= 800*/, int height /*= 600*/)
    : m_width(width)
    , m_height(height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL2 video subsystem couldn't be initialized. Error: " << SDL_GetError() << std::endl;
        throw "Can't initialize SDL";
    }

    SDL_ShowCursor(SDL_FALSE);
    int fullscreen = 0;
#define FULLSCREEN
#ifdef FULLSCREEN
    m_width = 1920, m_height = 1080;
    fullscreen = SDL_WINDOW_FULLSCREEN;
#endif
    window = SDL_CreateWindow("Glad Sample", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_width, m_height,
        fullscreen | SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    SDL_SetWindowResizable(window, SDL_TRUE);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr) {
        std::cerr << "SDL2 Renderer couldn't be created. Error: " << SDL_GetError() << std::endl;
        throw "Can't initialize renderer";
    }

    gl_context = SDL_GL_CreateContext(window);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize the OpenGL context." << std::endl;
        throw "Can't bind OpenGL";
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
        uint16_t actionMap = combineAction(event.key.keysym.sym, event.type == SDL_KEYDOWN);
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
    case SDL_WINDOWEVENT: {
    }
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
            m_width = event.window.data1;
            m_height = event.window.data2;
            glViewport(0, 0, (GLsizei)m_width, (GLsizei)m_height);
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
    return pos;
}
#pragma endregion

#pragma region mesh

void Mesh::createMesh(size_t vertexArrayLength, size_t vertexSize, void* vertexPtr,
    size_t indicesArrayLength, size_t indexSize, void* indexPtr)
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

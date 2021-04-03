#include "GLStuff.h"
#include <iostream>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

int main()
{
    try {

        Window window;
        Mesh mesh;
        Shader shader("Shaders/vert.glsl", "Shaders/frag.glsl");
        auto shaderViewMat = shader.getVariable("viewMat");
        auto shaderAspectRatio = shader.getVariable("aspectRatio");
        auto shaderLightDir = shader.getVariable("lightDir");

        double time = 0.;
        while (window.update()) {
            vec2 rot = window.getMousePos() * 3.f;
            time += window.getDeltaTime();
            vec3 rotateVec = glm::rotateY(vec3(2, 0, 0), -rot.y);
            rotateVec = glm::rotateZ(rotateVec, rot.x);
            mat4 viewMat = glm::inverse(glm::lookAt(rotateVec, vec3(0), vec3(0, 0, 1)));

            shaderAspectRatio.set(window.getAspectRatio());
            shaderLightDir.set(normalize(vec3(sinf(time), cosf(time), 1.f)));
            shaderViewMat.set(viewMat);
            mesh.draw();
        }

    } catch (const std::string& msg) {
        std::cerr << msg << std::endl;
    }
}

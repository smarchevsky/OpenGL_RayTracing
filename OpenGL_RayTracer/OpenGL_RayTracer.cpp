#include "GLStuff.h"
#include <iostream>
#include <memory>
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

        Window window(400, 400);
        Mesh mesh;
        Shader shader("Shaders/Vert.glsl", "Shaders/RayTrace.glsl");
        auto shaderViewMat = shader.getVariable("viewMat");
        auto shaderAspectRatio = shader.getVariable("aspectRatio");
        auto shaderLightDir = shader.getVariable("lightDir");

        auto updateShader = [&]() {
            shader.reloadProgram("Shaders/Vert.glsl", "Shaders/RayTrace.glsl");
            shaderViewMat.updateLocation("viewMat");
            shaderAspectRatio.updateLocation("aspectRatio");
            shaderLightDir.updateLocation("lightDir");
        };

        static int updater = 0;
        double time = 0.;
        while (window.update()) {
            if (updater++ % 60 == 0) {
                updateShader();
            }
            vec2 rot = window.getMousePos() * 3.f;
            time += window.getDeltaTime();
            vec3 rotateVec = glm::rotateY(vec3(5, 0, 0), -rot.y);
            rotateVec = glm::rotateZ(rotateVec, rot.x);
            mat4 viewMat = glm::inverse(glm::lookAt(rotateVec, vec3(0), vec3(0, 0, 1)));

            shaderAspectRatio.set(window.getAspectRatio());
            float ftime = (float)time;
            shaderLightDir.set(normalize(vec3(sin(ftime), cos(ftime), sin(ftime) + 1.4f)));
            shaderViewMat.set(viewMat);
            mesh.draw();
        }

    } catch (const std::string& msg) {
        std::cerr << msg << std::endl;
    }
}

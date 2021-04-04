#include "GLStuff.h"
#include <iostream>
#include <optional>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

struct Sphere {
    static bool collided(const Sphere& a, const Sphere& b, vec3& normal)
    {
        vec3 offset = a.pos - b.pos;
        float dist = glm::length(offset);
        if (dist < (a.r + b.r)) {
            normal = offset / dist;
            return true;
        }
        return false;
    }
    vec3 pos;
    vec3 vel;
    float r = 1.f;
};

static vec3 cubeRand(vec3 min, vec3 max)
{
    return { glm::linearRand(min.x, max.x), glm::linearRand(min.y, max.y), glm::linearRand(min.z, max.z) };
}

template <uint num = 8>
class SphereMotion {
public:
    SphereMotion(vec3 min, vec3 max)
    {
        m_boundMin = min, m_boundMax = max;
        for (auto& sph : m_spheres) {
            sph.pos = cubeRand(min, max);
            sph.vel = sphericalRand(1.4f);
            sph.r = 1.f;
        }
    }
    void update(float dt)
    {
        for (int i = 0; i < num; ++i) {
            auto& sph = m_spheres[i];
            sph.pos += sph.vel * dt;
            if (sph.pos.x > m_boundMax.x)
                sph.pos.x = m_boundMax.x, sph.vel.x *= -1.f;
            if (sph.pos.x < m_boundMin.x)
                sph.pos.x = m_boundMin.x, sph.vel.x *= -1.f;
            if (sph.pos.y > m_boundMax.y)
                sph.pos.y = m_boundMax.y, sph.vel.y *= -1.f;
            if (sph.pos.y < m_boundMin.y)
                sph.pos.y = m_boundMin.y, sph.vel.y *= -1.f;
            if (sph.pos.z > m_boundMax.z)
                sph.pos.z = m_boundMax.z, sph.vel.z *= -1.f;
            if (sph.pos.z < m_boundMin.z)
                sph.pos.z = m_boundMin.z, sph.vel.z *= -1.f;
        }
        for (int i = 0; i < num - 1; ++i) {
            for (int j = i + 1; j < num; ++j) {
                vec3 norm(0);
                if (Sphere::collided(m_spheres[i], m_spheres[j], norm)) {
                    vec3 midpoint = (m_spheres[i].pos + m_spheres[j].pos) * .5f;
                    m_spheres[i].pos = midpoint - norm;
                    m_spheres[j].pos = midpoint + norm;
                    m_spheres[i].vel = -glm::reflect(m_spheres[i].vel, norm);
                    m_spheres[j].vel = -glm::reflect(m_spheres[j].vel, norm);
                }
            }
        }
    }
    Sphere m_spheres[num] {};
    vec3 m_boundMin, m_boundMax;
};

int main()
{
    try {

        Window window(1920, 1080);
        Mesh mesh;
        Shader shader("Shaders/Vert.glsl", "Shaders/RayTrace.glsl");
        auto shaderViewMat = shader.getVariable("viewMat");
        auto shaderAspectRatio = shader.getVariable("aspectRatio");
        auto shaderLightDir = shader.getVariable("lightDir");
        auto shaderSphPositions = shader.getVariable("sphPositions");

        auto updateShader = [&]() {
            shader.reloadProgram("Shaders/Vert.glsl", "Shaders/RayTrace.glsl");
            shaderViewMat.updateLocation("viewMat");
            shaderAspectRatio.updateLocation("aspectRatio");
            shaderLightDir.updateLocation("lightDir");
            shaderSphPositions.updateLocation("sphPositions");
        };

        float boxSize = 3.f;
        SphereMotion<8> sphMotion(vec3(-boxSize), vec3(boxSize));

        static int updater = 0;
        double time = 0.;
        while (window.update()) {
            if (updater++ % 60 == 0) {
                updateShader();
            }
            vec2 rot = window.getMousePos() * 3.f;
			rot.y = -.5;
			rot.x = time * .2;
            float dt = window.getDeltaTime();
            time += dt;
            vec3 rotateVec = glm::rotateY(vec3(8, 0, 0), -rot.y);
            rotateVec = glm::rotateZ(rotateVec, rot.x);
            mat4 viewMat = glm::inverse(glm::lookAt(rotateVec, vec3(0), vec3(0, 0, 1)));

            sphMotion.update(dt);
            vec3 positions[8];
            for (int i = 0; i < 8; ++i) {
                positions[i] = sphMotion.m_spheres[i].pos;
                //std::cout << glm::to_string(positions[i]) << "  ";
            }
            std::cout << std::endl;
            shaderSphPositions.set(positions, 8);

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

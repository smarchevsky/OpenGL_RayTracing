#include "GLStuff.h"
#include <iostream>

int main()
{
    Window window;
    Shader shader;
    Mesh mesh;

    shader.bind();

    while (window.update()) {
        mesh.draw();
    }
}

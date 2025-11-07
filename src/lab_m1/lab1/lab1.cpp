#include "lab_m1/lab1/lab1.h"

#include <vector>
#include <iostream>

using namespace std;
using namespace m1;


/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */


Lab1::Lab1()
{
    // TODO(student): Never forget to initialize class variables!
    color = 0;
	change = false;
	changeForm = false;
	sphereMove = true;
}


Lab1::~Lab1()
{
}


void Lab1::Init()
{
    // Load a mesh from file into GPU memory. We only need to do it once,
    // no matter how many times we want to draw this mesh.
    spherePosition = glm::vec3(3, 0.5f, 0);
    {
        mesh = new Mesh("box");
        mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "box.obj");
        meshes[mesh->GetMeshID()] = mesh;
    }

    // TODO(student): Load some more meshes. The value of RESOURCE_PATH::MODELS
    // is actually a path on disk, go there and you will find more meshes.
	bunny = new Mesh("bunny");
    bunny->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "animals"), "bunny.obj");
    meshes[bunny->GetMeshID()] = bunny;

	sphere = new Mesh("sphere");
    sphere->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "sphere.obj");
	meshes[sphere->GetMeshID()] = sphere;

}


void Lab1::FrameStart()
{
}


void Lab1::Update(float deltaTimeSeconds)
{
    glm::ivec2 resolution = window->props.resolution;

    // Sets the clear color for the color buffer

    // TODO(student): Generalize the arguments of `glClearColor`.
    // You can, for example, declare three variables in the class header,
    // that will store the color components (red, green, blue).
    glClearColor(0, 0, 0, 1);

    // Clears the color buffer (using the previously set color) and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (change == true) {
        color = 3;
        glClearColor(color, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    if (changeForm == true) {
        int changeNum = rand() % 3;
        if (changeNum == 0) {
            bunny->ClearData();
            bunny->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "sphere.obj");
            changeForm = false;
        }
        else if (changeNum == 1) {
            bunny->ClearData();
            bunny->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "box.obj");
            changeForm = false;
        }
        else {
            bunny->ClearData();
            bunny->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "animals"), "bunny.obj");
            changeForm = false;
        }
    }
    // Sets the screen area where to draw
    glViewport(0, 0, resolution.x, resolution.y);

    // Render the object
    RenderMesh(meshes["box"], glm::vec3(1, 0.5f, 0), glm::vec3(0.5f));

    // Render the object again but with different properties
    RenderMesh(meshes["box"], glm::vec3(-1, 0.5f, 0));

    // TODO(student): We need to render (a.k.a. draw) the mesh that
    // was previously loaded. We do this using `RenderMesh`. Check the
    // signature of this function to see the meaning of its parameters.
    // You can draw the same mesh any number of times.
    RenderMesh(meshes["bunny"], glm::vec3(1, 5, 5), glm::vec3(0.3f));
    RenderMesh(meshes["sphere"], spherePosition, glm::vec3(0.8f));

}


void Lab1::FrameEnd()
{
    DrawCoordinateSystem();
}


/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */


void Lab1::OnInputUpdate(float deltaTime, int mods)
{
    // Treat continuous update based on input

    // TODO(student): Add some key hold events that will let you move
    // a mesh instance on all three axes. You will also need to
    // generalize the position used by `RenderMesh`.
    float speed = 2.0f;
    if (ctrlActive == false && sphereMove == true) {
        if (window->KeyHold(GLFW_KEY_W)) {
            spherePosition.z -= deltaTime * speed;
        }
        if (window->KeyHold(GLFW_KEY_S)) {
            spherePosition.z += deltaTime * speed;
        }
        if (window->KeyHold(GLFW_KEY_A)) {
            spherePosition.x -= deltaTime * speed;
        }
        if (window->KeyHold(GLFW_KEY_D)) {
            spherePosition.x += deltaTime * speed;
        }
        if (window->KeyHold(GLFW_KEY_E)) {
            spherePosition.y += deltaTime * speed;
        }
        if (window->KeyHold(GLFW_KEY_Q)) {
            spherePosition.y -= deltaTime * speed;
        }
        
        float otherSpeed = 4.0f;
        
    }
    if (window->KeyHold(GLFW_KEY_LEFT_CONTROL)) {
        ctrlActive = true;
    } else {
		ctrlActive = false;
	}
}


void Lab1::OnKeyPress(int key, int mods)
{
    // Add key press event
    if (key == GLFW_KEY_F) {
        // TODO(student): Change the values of the color components.
        change = !change;

    }
   
    // TODO(student): Add a key press event that will let you cycle
    // through at least two meshes, rendered at the same position.
    // You will also need to generalize the mesh name used by `RenderMesh`.
    if (key == GLFW_KEY_C) {
        changeForm = !changeForm;

    }
    if (window->MouseHold(GLFW_MOUSE_BUTTON_RIGHT)) {
        sphereMove = false;
    }
    else {
		sphereMove = true;
    }
	float otherSpeed = 4.0f;    
    if (ctrlActive && key == GLFW_KEY_W) {
        spherePosition.z -= otherSpeed;
    }
    if (ctrlActive && key == GLFW_KEY_S) {
        spherePosition.z += otherSpeed;
    }
    if (ctrlActive && key == GLFW_KEY_A) {
        spherePosition.x -= otherSpeed;
    }
    if (ctrlActive && key == GLFW_KEY_D) {
        spherePosition.x += otherSpeed;
    }
    if (ctrlActive && key == GLFW_KEY_E) {
        spherePosition.y += otherSpeed;
    }
    if (ctrlActive && key == GLFW_KEY_Q) {
        spherePosition.y -= otherSpeed;
    }
}


void Lab1::OnKeyRelease(int key, int mods)
{
    // Add key release event
}


void Lab1::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
    // Add mouse move event
}


void Lab1::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button press event
}


void Lab1::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
    // Add mouse button release event
}


void Lab1::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
    // Treat mouse scroll event
}


void Lab1::OnWindowResize(int width, int height)
{
    // Treat window resize event
}

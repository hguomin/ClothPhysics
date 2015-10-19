#include <Windows.h>
#include <GL\glew.h>
#include <GL\freeglut.h>
#include "Display.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "Transform.h"
#include "Camera.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Skybox.h"
#include "GridMesh.h"
#include "Cloth2.h"
#include <iostream>

/*https://www.youtube.com/watch?v=RqRxhY6iLto */


int main(int argc, char ** argv[])
{

	Display display(800, 600, "TSBK07 Space");
	Shader shader("./shaders/space");
	// skyShader("./shaders/cube");
	Texture texture("./textures/dirt.tga");
	Camera camera(glm::vec3(0, 0, 0), 70.0f, display.GetAspectRation(), 0.01f, 1000.0f);
	
	Skybox sky;
	sky.SkyboxInit("./textures/skybox/", "back.jpg", "front.jpg", "left.jpg", "right.jpg", "top.jpg", "bottom.jpg");
	Transform transform;
	Keyboard keyboard;
	Mouse mouse;

	Cloth2 cloth(5, 5, 5, 5);

	float counter = 0.0f;
	Mesh monkey("./models/monkey3.obj");
	Mesh box("./models/box.obj");

	std::cout << "init complete" << std::endl;


	while (!display.IsClosed())
	{
		display.Clear(1, 0, 1, 1);

		SDL_Event e;

		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
			{
				display.HandleEvent(e);
			}
			if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
			{
				mouse.HandleEvent(e, camera);
			}
		}

		const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

		keyboard.HandleEvent(currentKeyStates, camera);
		
		sky.Draw(transform, camera);

		shader.Use();
		shader.Update(transform, camera);
		
		//texture.Use();
		//monkey.Draw();
		cloth.Update();
		//cloth.print();

		display.Update();


		counter += 0.001f;
	}
	return 0;
}
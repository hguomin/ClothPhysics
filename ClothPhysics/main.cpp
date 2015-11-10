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
#include <algorithm>
#include <stdlib.h>
#include <time.h>

/*https://www.youtube.com/watch?v=RqRxhY6iLto */

#define DESIRED_FPS 120.0f
#define MAX_PHYSICS_STEP 6
#define MS_PER_SECOND 1000.0f
#define DESIRED_FRAME_TIME (MS_PER_SECOND/ DESIRED_FPS)
#define MAX_DELTA_TIME 0.1f


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

	Cloth2 cloth(10, 10, 10, 10);

	float counter = 0.0f;
	Mesh monkey("./models/monkey3.obj");
	Mesh box("./models/box.obj");

	std::cout << "init complete" << std::endl;

	float previousTicks = SDL_GetTicks();
	srand(time(NULL));

	glm::vec3 wind(0,0,0);
	while (!display.IsClosed())
	{
		//time handling
		float currentTicks = SDL_GetTicks();
		float frameTime = currentTicks - previousTicks;
		previousTicks = currentTicks;
		float totalDeltaTime = frameTime / DESIRED_FRAME_TIME;

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
		float amount = 1.0f;
		if (currentKeyStates[SDL_SCANCODE_O])
		{
			wind.z += amount;
		}
		if (currentKeyStates[SDL_SCANCODE_I])
		{
			wind.z -= amount;
		}
		if (currentKeyStates[SDL_SCANCODE_L])
		{
			wind.x += amount;
		}
		if (currentKeyStates[SDL_SCANCODE_K])
		{
			wind.x -= amount;
		}
		if (currentKeyStates[SDL_SCANCODE_J])
		{
			wind = glm::vec3(0);
		}

		sky.Draw(transform, camera);
		
		shader.Use();
		shader.Update(transform, camera);
		
		//texture.Use();
		//monkey.Draw();

		//Physics handling semi-fixed timestep
		int physicSteps = 0;
		
		while (totalDeltaTime > 0 && physicSteps < MAX_PHYSICS_STEP)
		{
			float dt = std::min(totalDeltaTime, MAX_DELTA_TIME);
			cloth.Update(dt, wind);

			totalDeltaTime -= dt;
			physicSteps++;
		}
		//cloth.Update(0.01f, wind);
		cloth.Draw();

		display.Update();


		counter += 0.001f;
	}
	return 0;
}
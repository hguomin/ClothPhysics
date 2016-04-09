#define GLM_FORCE_RADIANS
#include <Windows.h>
#include <GL\glew.h>
#include <GL\freeglut.h>
//#include <AntTweakBar.h>

#include "Display.h"
//#include "Basic_Shader.h"
//#include "Phong_Shader.h"
//#include "Mesh.h"
#include "Texture.h"
#include "Transform.h"
#include "Camera.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Skybox.h"
//#include "GridMesh.h"
//#include "Cloth2.h"
#include "Cloth_GPU.h"

#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <time.h>

#include "GLError.h"

/*https://www.youtube.com/watch?v=RqRxhY6iLto */

#define DESIRED_FPS 120.0f
#define MAX_PHYSICS_STEP 6
#define MS_PER_SECOND 1000.0f
#define DESIRED_FRAME_TIME (MS_PER_SECOND/ DESIRED_FPS)
#define MAX_DELTA_TIME 0.1f

bool GPU = true;

int main(int argc, char ** argv[])
{
	Display display(800, 600, "TSBK07 Space");
	
	Texture texture("./textures/white.jpg");
	
	glm::vec3 cameraStartPosition = glm::vec3(-1, 6, 8);
	Camera camera(cameraStartPosition, 70.0f, display.GetAspectRation(), 0.01f, 1000.0f);
	camera.SetForward(glm::vec3(0.1f, -0.4f, -0.9f));

	Skybox sky;
	sky.SkyboxInit("./textures/skybox/", "back.jpg", "front.jpg", "left.jpg", "right.jpg", "top.jpg", "bottom.jpg");
	
	Transform transform;
	Keyboard keyboard;
	Mouse mouse;
	//Cloth2 cloth(8, 8, 75, 75);
	Cloth_GPU cloth_gpu;
	
	float counter = 0.0f;

	float previousTicks = (float)SDL_GetTicks();

	srand((unsigned int)time(NULL));

	SDL_Event sdl_event;

	std::cout << "init complete" << std::endl;
	while (!display.IsClosed())
	{
		//time handling
		float currentTicks = (float)SDL_GetTicks();
		float frameTime = currentTicks - previousTicks;
		previousTicks = currentTicks;
		float totalDeltaTime = frameTime / DESIRED_FRAME_TIME;

		display.Clear(0, 0, 0, 1);

		while (SDL_PollEvent(&sdl_event))
		{
			if (sdl_event.type == SDL_QUIT)
			{
				display.HandleEvent(sdl_event);
			}
			if (sdl_event.type == SDL_MOUSEMOTION || sdl_event.type == SDL_MOUSEBUTTONDOWN || sdl_event.type == SDL_MOUSEBUTTONUP)
			{
				mouse.HandleEvent(sdl_event, camera);
			}
		}
		
		const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
		
		keyboard.HandleEvent(currentKeyStates, camera);

		cloth_gpu.Draw(transform, camera);
		display.Update();
		
		counter += 0.001f;
	}
	return 0;
}
#define GLM_FORCE_RADIANS
#include <Windows.h>
#include <GL\glew.h>
#include <GL\freeglut.h>

#include "Display.h"
#include "Texture.h"
#include "Transform.h"
#include "Camera.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Cloth_GPU.h"

#include <iostream>

int main(int argc, char ** argv[])
{
	std::cout << "Init started" << std::endl;
	Display display(800, 600, "TSBK07 Space");
	
	Texture texture("./textures/white.jpg");
	
	glm::vec3 cameraStartPosition = glm::vec3(-1, 6, 8);
	Camera camera(cameraStartPosition, 70.0f, display.GetAspectRation(), 0.01f, 1000.0f);
	camera.SetForward(glm::vec3(0.1f, -0.4f, -0.9f));

	Transform transform;
	Keyboard keyboard;
	Mouse mouse;
	Cloth_GPU cloth_gpu;
	
	SDL_Event sdl_event;

	std::cout << "init complete" << std::endl;
	while (!display.IsClosed())
	{
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
	}
	return 0;
}
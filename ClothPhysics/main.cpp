#define GLM_FORCE_RADIANS
#include <Windows.h>
#include <GL\glew.h>
#include <GL\freeglut.h>
#include <AntTweakBar.h>

#include "Display.h"
#include "Basic_Shader.h"
#include "Phong_Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "Transform.h"
#include "Camera.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Skybox.h"
#include "GridMesh.h"
#include "Cloth2.h"
#include "Cloth_GPU.h"
#include "Cloth_GPU2.h"

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
	
	TwInit(TW_OPENGL, NULL);
	TwWindowSize(800, 600);
	
	Basic_Shader shader("./shaders/space");
	Phong_Shader phong("./shaders/phong");
	
	Texture texture("./textures/white.jpg");
	
	glm::vec3 cameraStartPosition = glm::vec3(-1, 6, 8);
	Camera camera(cameraStartPosition, 70.0f, display.GetAspectRation(), 0.01f, 1000.0f);
	camera.SetForward(glm::vec3(0.1f, -0.4f, -0.9f));
	

	Skybox sky;
	sky.SkyboxInit("./textures/skybox/", "back.jpg", "front.jpg", "left.jpg", "right.jpg", "top.jpg", "bottom.jpg");
	
	Transform transform;
	Keyboard keyboard;
	Mouse mouse;
	check_gl_error();
	Cloth2 cloth(8, 8, 75, 75);
	//Cloth_GPU gpuCloth;
	Cloth_GPU2 cloth2;
	check_gl_error();

	float counter = 0.0f;
	Mesh monkey("./models/monkey3.obj");
	Mesh box("./models/box.obj");
	
	glm::vec3 wind(0, 0, 1);
	float windX = 0.0f;
	float windY = 0.0f;
	float windZ = 0.0f;
	unsigned int iterations = 3;
	bool cutOnce = false;
	TwBar *myBar;
	check_gl_error();
	myBar = TwNewBar("Hello!");
	
	TwAddVarRW(myBar, "Wind X", TW_TYPE_FLOAT, &windX, NULL);
	TwAddVarRW(myBar, "Wind Y", TW_TYPE_FLOAT, &windY, NULL);
	TwAddVarRW(myBar, "Wind Z", TW_TYPE_FLOAT, &windZ, NULL);
	TwAddVarRW(myBar, "Iterations", TW_TYPE_UINT16, &iterations, NULL);
	

	std::cout << "init complete" << std::endl;

	float previousTicks = (float)SDL_GetTicks();
	
	srand((unsigned int)time(NULL));

	SDL_Event sdl_event;
	int handled;
	
	check_gl_error();
	
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
			handled = TwEventSDL(&sdl_event, SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
			if (!handled)
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
		if (currentKeyStates[SDL_SCANCODE_LEFT])
		{
			glm::vec3 temp = cloth.ball_position;
			cloth.ball_position = glm::vec3(temp.x, temp.y, temp.z + 0.1f);
		}
		if (currentKeyStates[SDL_SCANCODE_RIGHT])
		{
			glm::vec3 temp = cloth.ball_position;
			cloth.ball_position = glm::vec3(temp.x, temp.y, temp.z - 0.1f);
		}
		if (currentKeyStates[SDL_SCANCODE_UP])
		{
			glm::vec3 temp = cloth.ball_position;
			cloth.ball_position = glm::vec3(temp.x + 0.1f, temp.y, temp.z);
		}
		if (currentKeyStates[SDL_SCANCODE_DOWN])
		{
			glm::vec3 temp = cloth.ball_position;
			cloth.ball_position = glm::vec3(temp.x - 0.1f, temp.y, temp.z);
		}
		if (currentKeyStates[SDL_SCANCODE_UP] && currentKeyStates[SDL_SCANCODE_LSHIFT])
		{
			glm::vec3 temp = cloth.ball_position;
			cloth.ball_position = glm::vec3(temp.x , temp.y + 0.1f, temp.z);
		}
		if (currentKeyStates[SDL_SCANCODE_DOWN] && currentKeyStates[SDL_SCANCODE_LSHIFT])
		{
			glm::vec3 temp = cloth.ball_position;
			cloth.ball_position = glm::vec3(temp.x, temp.y - 0.1f, temp.z);
		}
		if (currentKeyStates[SDL_SCANCODE_P])
		{
			glm::vec3 pos = camera.GetPosition();
			glm::vec3 view = camera.GetForward();
			std::cout << "position: " << pos.x << ", " << pos.y << ", " << pos.z << "\n";
			std::cout << "view direction" << view.x << ", " << view.y << ", " << view.z << std::endl;
		}
		if (currentKeyStates[SDL_SCANCODE_9])
		{
			cloth.CalculatePerVertexNormals();
			cloth.UpdateTextureCoordinates();
		}
		if (currentKeyStates[SDL_SCANCODE_B])
		{
			GPU = !GPU;
		}
		if (!GPU)
		{
			camera.SetSpeed(3.0f);
			wind = glm::vec3(windX, windY, windZ);

			sky.Draw(transform, camera);

			//shader.Use();
			//shader.UpdateValues(transform, camera);

			phong.Use();
			phong.UpdateValues(transform, camera);
			//phong.UnUse();

			//texture.Use();
			//monkey.Draw();
			//Physics handling semi-fixed timestep
			int physicSteps = 0;

			while (totalDeltaTime > 0 && physicSteps < MAX_PHYSICS_STEP)
			{
				float dt = std::min(totalDeltaTime, MAX_DELTA_TIME);
				cloth.Update(dt, wind, iterations);

				totalDeltaTime -= dt;
				physicSteps++;
			}
			//cloth.Update(0.01f, wind, iterations);

			cloth.Draw();
		}
		else
		{
			camera.SetSpeed(0.5f);
			//gpuCloth.Draw(transform, camera);
			if (cutOnce == false)
			{
				cloth2.Split(3, glm::vec3(0, 0, -1));
				cloth2.Split(4, glm::vec3(0, 0, -1));
				cloth2.Split(5, glm::vec3(0, 0, -1));
				cutOnce = true;
			}
			check_gl_error();
			cloth2.Draw(transform, camera);
			check_gl_error();
			//Current bug. Splitting diagonaly should destroy shearing springs.Not at the moment
		}
		//TwDraw();
		display.Update();
		

		counter += 0.001f;
	}
	TwTerminate();
	return 0;
}
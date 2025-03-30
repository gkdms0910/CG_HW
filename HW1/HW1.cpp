#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>

#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <vector>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

// -------------------------------------------------
// Global Variables
// -------------------------------------------------
int Width = 512;
int Height = 512;
std::vector<float> OutputImage;
// -------------------------------------------------



void render()
{
	OutputImage.clear();
	for (int j = 0; j < Height; ++j)
	{
		for (int i = 0; i < Width; ++i)
		{
			// ---------------------------------------------------
			// --- Implement your code here to generate the image
			// ---------------------------------------------------

			// ------------------------------------------

			vec3 eye(0.0f, 0.0f, 0.0f);
			vec3 u(1, 0, 0), v(0, 1, 0), w(0, 0, 1); // 정해진 좌표계
			float l = -0.1f, r = 0.1f;
			float b = -0.1f, t = 0.1f;
			float d = 0.1f; // 이미지 평면까지 거리

			// 픽셀 중심 좌표를 기준으로 u, v 위치 계산
			float u_coord = l + (r - l) * (i + 0.5f) / Width;
			float v_coord = b + (t - b) * (j + 0.5f) / Height;

			// 이미지 평면상의 픽셀 위치
			vec3 pixel_pos = eye - d * w + u_coord * u + v_coord * v;
			vec3 ray_dir = normalize(pixel_pos - eye);

			// 레이 정의
			vec3 ray_origin = eye;
			vec3 ray_direction = ray_dir;



			// 구 3개 정보
			struct Sphere {
				vec3 center;
				float radius;
			};
			std::vector<Sphere> spheres = {
				{ vec3(-4, 0, -7), 1.0f },
				{ vec3(0, 0, -7), 2.0f },
				{ vec3(4, 0, -7), 1.0f }
			};

			// 평면 정보 (y = -2)
			float plane_y = -2.0f;

			// 레이와 구 교차 검사
			bool hit = false;
			for (const auto& s : spheres) {
				vec3 oc = ray_origin - s.center;
				float a = dot(ray_direction, ray_direction);
				float b = 2.0f * dot(oc, ray_direction);
				float c = dot(oc, oc) - s.radius * s.radius;
				float discriminant = b * b - 4 * a * c;
				if (discriminant > 0.0f) {
					hit = true;
					break;
				}
			}

			// 레이와 평면 교차 검사 (y = -2)
			if (!hit && ray_direction.y != 0.0f) {
				float t = (plane_y - ray_origin.y) / ray_direction.y;
				if (t > 0.0f) hit = true;
			}

			// 픽셀 색상 설정
			vec3 color = hit ? vec3(1.0f) : vec3(0.0f);  // 흰색 or 검정

			OutputImage.push_back(color.x); // R
			OutputImage.push_back(color.y); // G
			OutputImage.push_back(color.z); // B

		}
	}
}


void resize_callback(GLFWwindow*, int nw, int nh)
{
	//This is called in response to the window resizing.
	//The new width and height are passed in so we make 
	//any necessary changes:
	Width = nw;
	Height = nh;
	//Tell the viewport to use all of our screen estate
	glViewport(0, 0, nw, nh);

	//This is not necessary, we're just working in 2d so
	//why not let our spaces reflect it?
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0.0, static_cast<double>(Width)
		, 0.0, static_cast<double>(Height)
		, 1.0, -1.0);

	//Reserve memory for our render so that we don't do 
	//excessive allocations and render the image
	OutputImage.reserve(Width * Height * 3);
	render();
}


int main(int argc, char* argv[])
{
	// -------------------------------------------------
	// Initialize Window
	// -------------------------------------------------

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(Width, Height, "OpenGL Viewer", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//We have an opengl context now. Everything from here on out 
	//is just managing our window or opengl directly.

	//Tell the opengl state machine we don't want it to make 
	//any assumptions about how pixels are aligned in memory 
	//during transfers between host and device (like glDrawPixels(...) )
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	//We call our resize function once to set everything up initially
	//after registering it as a callback with glfw
	glfwSetFramebufferSizeCallback(window, resize_callback);
	resize_callback(NULL, Width, Height);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		//Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// -------------------------------------------------------------
		//Rendering begins!
		glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
		//and ends.
		// -------------------------------------------------------------

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		//Close when the user hits 'q' or escape
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS
			|| glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
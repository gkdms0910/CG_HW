#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>
#include <cmath>
#include <limits>


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
			vec3 eye(0.0f, 0.0f, 0.0f);
			vec3 u(1, 0, 0), v(0, 1, 0), w(0, 0, 1);
			float l = -0.1f, r = 0.1f;
			float b = -0.1f, t = 0.1f;
			float d = 0.1f;

			float u_coord = l + (r - l) * (i + 0.5f) / Width;
			float v_coord = b + (t - b) * (j + 0.5f) / Height;
			vec3 pixel_pos = eye - d * w + u_coord * u + v_coord * v;
			vec3 ray_dir = normalize(pixel_pos - eye);

			vec3 ray_origin = eye;
			vec3 ray_direction = ray_dir;

			struct Sphere {
				vec3 center;
				float radius;
			};

			std::vector<Sphere> spheres = {
				{ vec3(-4, 0, -7), 1.0f },
				{ vec3(0, 0, -7), 2.0f },
				{ vec3(4, 0, -7), 1.0f }
			};

			float plane_y = -2.0f;
			vec3 color(0.0f);

			float closest_t = std::numeric_limits<float>::infinity();
			int hit_sphere_index = -1;
			vec3 hit_point, normal;

			// 구 교차
			for (int k = 0; k < spheres.size(); ++k) {
				vec3 oc = ray_origin - spheres[k].center;
				float a = dot(ray_direction, ray_direction);
				float b = 2.0f * dot(oc, ray_direction);
				float c = dot(oc, oc) - spheres[k].radius * spheres[k].radius;
				float discriminant = b * b - 4 * a * c;
				if (discriminant > 0.0f) {
					float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
					if (t > 0.001f && t < closest_t) {
						closest_t = t;
						hit_sphere_index = k;
						hit_point = ray_origin + t * ray_direction;
						normal = normalize(hit_point - spheres[k].center);
					}
				}
			}

			// 평면 교차
			if (ray_direction.y != 0.0f) {
				float t = (plane_y - ray_origin.y) / ray_direction.y;
				if (t > 0.001f && t < closest_t) {
					closest_t = t;
					hit_sphere_index = -2;
					hit_point = ray_origin + t * ray_direction;
					normal = vec3(0, 1, 0);
				}
			}

			if (closest_t < std::numeric_limits<float>::infinity()) {
				vec3 light_pos(-4, 4, -3);
				vec3 light_color(1, 1, 1);
				vec3 to_light = normalize(light_pos - hit_point);
				vec3 to_camera = normalize(-ray_direction);
				vec3 reflect_dir = reflect(-to_light, normal);

				// 그림자 검사
				vec3 shadow_origin = hit_point + 0.001f * normal;
				vec3 shadow_ray = to_light;
				bool in_shadow = false;
				for (const auto& s : spheres) {
					vec3 oc = shadow_origin - s.center;
					float a = dot(shadow_ray, shadow_ray);
					float b = 2.0f * dot(oc, shadow_ray);
					float c = dot(oc, oc) - s.radius * s.radius;
					float discriminant = b * b - 4 * a * c;
					if (discriminant > 0.0f) {
						float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
						if (t > 0.001f) {
							in_shadow = true;
							break;
						}
					}
				}

				// 재질 설정
				vec3 ka, kd, ks;
				float spec_pow;
				if (hit_sphere_index == 0) {
					ka = vec3(0.2, 0, 0); kd = vec3(1, 0, 0); ks = vec3(0); spec_pow = 0;
				}
				else if (hit_sphere_index == 1) {
					ka = vec3(0, 0.2, 0); kd = vec3(0, 0.5, 0); ks = vec3(0.5); spec_pow = 32;
				}
				else if (hit_sphere_index == 2) {
					ka = vec3(0, 0, 0.2); kd = vec3(0, 0, 1); ks = vec3(0); spec_pow = 0;
				}
				else {
					ka = vec3(0.2); kd = vec3(1); ks = vec3(0); spec_pow = 0;
				}

				// 조명 계산
				color += ka * light_color;
				if (!in_shadow) {
					float diff = max(dot(normal, to_light), 0.0f);
					float spec = pow(max(dot(reflect_dir, to_camera), 0.0f), spec_pow);
					color += kd * light_color * diff;
					color += ks * light_color * spec;
				}

				color = clamp(color, 0.0f, 1.0f);
			}

			// 이미지에 색상 저장
			OutputImage.push_back(color.r);
			OutputImage.push_back(color.g);
			OutputImage.push_back(color.b);
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
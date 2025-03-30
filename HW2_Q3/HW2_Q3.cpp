#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>
#include <cmath>
#include <ctime>
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

int Width = 512;
int Height = 512;
std::vector<float> OutputImage;

void render()
{
	//Create our image. We don't want to do this in 
	//the main loop since this may be too slow and we 
	//want a responsive display of our beautiful image.
	//Instead we draw to another buffer and copy this to the 
	//framebuffer using glDrawPixels(...) every refresh
	OutputImage.clear();

	vec3 eye(0.0f);
	vec3 u(1, 0, 0), v(0, 1, 0), w(0, 0, 1);
	float l = -0.1f, r = 0.1f, b = -0.1f, t = 0.1f, d = 0.1f;

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

	for (int j = 0; j < Height; ++j) {
		for (int i = 0; i < Width; ++i) {
			vec3 color_sum(0.0f);
			int N = 64;

			for (int s = 0; s < N; ++s) {
				// 픽셀 내부에서 무작위 샘플링 좌표 계산
				float ru = static_cast<float>(rand()) / RAND_MAX;
				float rv = static_cast<float>(rand()) / RAND_MAX;

				float u_coord = l + (r - l) * (i + ru) / Width;
				float v_coord = b + (t - b) * (j + rv) / Height;

				// 이미지 평면상의 픽셀 위치 계산
				vec3 pixel_pos = eye - d * w + u_coord * u + v_coord * v;
				vec3 ray_dir = normalize(pixel_pos - eye);
				vec3 ray_origin = eye;
				vec3 ray_direction = ray_dir;

				float closest_t = std::numeric_limits<float>::infinity();
				int hit_sphere_index = -1;
				vec3 hit_point, normal;

				// --- 레이와 구 교차 검사 ---
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

				// --- 레이와 평면 교차 검사 ---
				if (ray_direction.y != 0.0f) {
					float t = (plane_y - ray_origin.y) / ray_direction.y;
					if (t > 0.001f && t < closest_t) {
						closest_t = t;
						hit_sphere_index = -2;
						hit_point = ray_origin + t * ray_direction;
						normal = vec3(0, 1, 0);
					}
				}

				// --- Phong 조명 모델 계산 ---
				vec3 color(0.0f);
				if (closest_t < std::numeric_limits<float>::infinity()) {
					vec3 light_pos(-4, 4, -3);
					vec3 light_color(1);
					vec3 to_light = normalize(light_pos - hit_point);
					vec3 to_camera = normalize(-ray_direction);
					vec3 reflect_dir = reflect(-to_light, normal);

					// --- 그림자 검사 ---
					vec3 shadow_origin = hit_point + 0.001f * normal;
					bool in_shadow = false;
					for (const auto& s : spheres) {
						vec3 oc = shadow_origin - s.center;
						float a = dot(to_light, to_light);
						float b = 2.0f * dot(oc, to_light);
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

					// --- 재질(Material) 파라미터 설정 ---
					vec3 ka, kd, ks; float spec_pow;
					if (hit_sphere_index == 0) { ka = vec3(0.2, 0, 0); kd = vec3(1, 0, 0); ks = vec3(0); spec_pow = 0; }
					else if (hit_sphere_index == 1) { ka = vec3(0, 0.2, 0); kd = vec3(0, 0.5, 0); ks = vec3(0.5); spec_pow = 32; }
					else if (hit_sphere_index == 2) { ka = vec3(0, 0, 0.2); kd = vec3(0, 0, 1); ks = vec3(0); spec_pow = 0; }
					else { ka = vec3(0.2); kd = vec3(1); ks = vec3(0); spec_pow = 0; }

					// --- Phong shading 적용 ---
					color += ka * light_color;
					if (!in_shadow) {
						float diff = max(dot(normal, to_light), 0.0f);
						float spec = pow(max(dot(reflect_dir, to_camera), 0.0f), spec_pow);
						color += kd * light_color * diff;
						color += ks * light_color * spec;
					}
				}

				// --- 누적 색상 합산 ---
				color_sum += clamp(color, 0.0f, 1.0f);
			}

			// --- 평균 및 감마 보정 ---
			vec3 final_color = color_sum / float(N);
			float gamma = 2.2f;
			final_color = pow(final_color, vec3(1.0f / gamma));
			final_color = clamp(final_color, 0.0f, 1.0f);

			// --- 픽셀 색상 출력 ---
			OutputImage.push_back(final_color.r);
			OutputImage.push_back(final_color.g);
			OutputImage.push_back(final_color.b);
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
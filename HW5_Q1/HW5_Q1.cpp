#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <GL/glut.h>

#define WIDTH 512
#define HEIGHT 512

using namespace std;

struct Vec3 { float x, y, z; };
struct Vec4 { float x, y, z, w; };

extern int gNumVertices;
extern int gNumTriangles;
extern int* gIndexBuffer;
void create_scene();

vector<Vec3> gVertexBuffer;
float zBuffer[HEIGHT][WIDTH];
unsigned char framebuffer[HEIGHT][WIDTH][3];

Vec4 modelTransform(Vec3 v) {
    return { v.x * 2, v.y * 2, v.z * 2 - 7, 1 };
}

Vec4 projectionTransform(Vec4 v) {
    float l = -0.1f, r = 0.1f, b = -0.1f, t = 0.1f, n = -0.1f, f = -1000.0f;
    float x = (2 * n * v.x) / ((r - l) * v.z);
    float y = (2 * n * v.y) / ((t - b) * v.z);
    float z = (f + n + 2 * f * n / v.z) / (f - n);
    return { x, y, z, 1 };
}

Vec3 viewportTransform(Vec4 v) {
    return {
        (v.x + 1.0f) * 0.5f * WIDTH,
        (1.0f - v.y) * 0.5f * HEIGHT,
        v.z
    };
}

float edgeFunction(const Vec3& a, const Vec3& b, const Vec3& c) {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

void rasterizeTriangle(Vec3 v0, Vec3 v1, Vec3 v2) {
    int minX = max(0, (int)floor(min({ v0.x, v1.x, v2.x })));
    int maxX = min(WIDTH - 1, (int)ceil(max({ v0.x, v1.x, v2.x })));
    int minY = max(0, (int)floor(min({ v0.y, v1.y, v2.y })));
    int maxY = min(HEIGHT - 1, (int)ceil(max({ v0.y, v1.y, v2.y })));

    float area = edgeFunction(v0, v1, v2);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            Vec3 p = { (float)x + 0.5f, (float)y + 0.5f, 0 };

            float w0 = edgeFunction(v1, v2, p);
            float w1 = edgeFunction(v2, v0, p);
            float w2 = edgeFunction(v0, v1, p);

            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                w0 /= area;
                w1 /= area;
                w2 /= area;

                float depth = w0 * v0.z + w1 * v1.z + w2 * v2.z;

                if (depth < zBuffer[y][x]) {
                    zBuffer[y][x] = depth;
                    framebuffer[y][x][0] = 255;
                    framebuffer[y][x][1] = 255;
                    framebuffer[y][x][2] = 255;
                }
            }
        }
    }
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawPixels(WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, framebuffer);
    glFlush();
}

int main(int argc, char** argv) {
    create_scene();

    int width = 32;
    int height = 16;
    gVertexBuffer.resize(gNumVertices);
    int t = 0;
    for (int j = 1; j < height - 1; ++j) {
        for (int i = 0; i < width; ++i) {
            float theta = (float)j / (height - 1) * M_PI;
            float phi = (float)i / (width - 1) * M_PI * 2;
            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = -sinf(theta) * sinf(phi);
            gVertexBuffer[t++] = { x, y, z };
        }
    }
    gVertexBuffer[t++] = { 0, 1, 0 };
    gVertexBuffer[t++] = { 0, -1, 0 };

    gNumTriangles = ((height - 3) * (width - 1) * 2) + 2 * (width - 1);

    fill(&framebuffer[0][0][0], &framebuffer[0][0][0] + WIDTH * HEIGHT * 3, 0);
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x)
            zBuffer[y][x] = 1e9;

    for (int i = 0; i < gNumTriangles; ++i) {
        int i0 = gIndexBuffer[i * 3 + 0];
        int i1 = gIndexBuffer[i * 3 + 1];
        int i2 = gIndexBuffer[i * 3 + 2];

        Vec3 v[3];
        v[0] = viewportTransform(projectionTransform(modelTransform(gVertexBuffer[i0])));
        v[1] = viewportTransform(projectionTransform(modelTransform(gVertexBuffer[i1])));
        v[2] = viewportTransform(projectionTransform(modelTransform(gVertexBuffer[i2])));

        rasterizeTriangle(v[0], v[1], v[2]);
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("CPU Rasterized Sphere");

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WIDTH, 0.0, HEIGHT);
    glPixelZoom(1.0f, -1.0f);
    glRasterPos2i(0, HEIGHT - 1);

    glutDisplayFunc(renderScene);
    glutMainLoop();

    return 0;
}

#define _USE_MATH_DEFINES
#include <iostream>
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
vector<Vec3> gNormals;
float zBuffer[HEIGHT][WIDTH];
unsigned char framebuffer[HEIGHT][WIDTH][3];

Vec3 normalize(const Vec3& v) {
    float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return { v.x / len, v.y / len, v.z / len };
}
Vec3 cross(const Vec3& a, const Vec3& b) {
	return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}



Vec3 operator+(const Vec3& a, const Vec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vec3 operator-(const Vec3& a, const Vec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
Vec3 operator*(float s, const Vec3& v) { return { s * v.x, s * v.y, s * v.z }; }

Vec3 modelTransform(Vec3 v) { return { v.x * 2, v.y * 2, v.z * 2 - 7 }; }

Vec4 projectionTransform(Vec3 v) {
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

Vec3 computeShading(const Vec3& pos, const Vec3& normal) {
    Vec3 ka = { 0.0f, 1.0f, 0.0f }, kd = { 0.0f, 0.5f, 0.0f }, ks = { 0.5f, 0.5f, 0.5f };
    float p = 32.0f;
    Vec3 lightPos = { -4.0f, 4.0f, -3.0f }, Ia = { 0.2f, 0.2f, 0.2f }, Il = { 1.0f, 1.0f, 1.0f };

    Vec3 l = normalize(lightPos - pos);
    Vec3 v = normalize({ -pos.x, -pos.y, -pos.z });
    Vec3 r = normalize(2 * dot(normal, l) * normal - l);

    float diff = max(dot(normal, l), 0.0f);
    float spec = pow(max(dot(r, v), 0.0f), p);

    Vec3 color;
    color.x = pow(Ia.x * ka.x + Il.x * (kd.x * diff + ks.x * spec), 1.0f / 2.2f);
    color.y = pow(Ia.y * ka.y + Il.y * (kd.y * diff + ks.y * spec), 1.0f / 2.2f);
    color.z = pow(Ia.z * ka.z + Il.z * (kd.z * diff + ks.z * spec), 1.0f / 2.2f);
    return color;
}

void rasterizePhong(Vec3 scr0, Vec3 world0, Vec3 n0,
    Vec3 scr1, Vec3 world1, Vec3 n1,
    Vec3 scr2, Vec3 world2, Vec3 n2) {
    int minX = max(0, (int)floor(min({ scr0.x, scr1.x, scr2.x })));
    int maxX = min(WIDTH - 1, (int)ceil(max({ scr0.x, scr1.x, scr2.x })));
    int minY = max(0, (int)floor(min({ scr0.y, scr1.y, scr2.y })));
    int maxY = min(HEIGHT - 1, (int)ceil(max({ scr0.y, scr1.y, scr2.y })));

    float area = edgeFunction(scr0, scr1, scr2);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            Vec3 p = { (float)x + 0.5f, (float)y + 0.5f, 0 };
            float w0 = edgeFunction(scr1, scr2, p);
            float w1 = edgeFunction(scr2, scr0, p);
            float w2 = edgeFunction(scr0, scr1, p);
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                w0 /= area; w1 /= area; w2 /= area;
                float depth = w0 * scr0.z + w1 * scr1.z + w2 * scr2.z;
                if (depth < zBuffer[y][x]) {
                    zBuffer[y][x] = depth;
                    Vec3 pos = w0 * world0 + w1 * world1 + w2 * world2;
                    Vec3 normal = normalize(w0 * n0 + w1 * n1 + w2 * n2);
                    Vec3 color = computeShading(pos, normal);
                    framebuffer[y][x][0] = (unsigned char)(min(1.0f, color.x) * 255);
                    framebuffer[y][x][1] = (unsigned char)(min(1.0f, color.y) * 255);
                    framebuffer[y][x][2] = (unsigned char)(min(1.0f, color.z) * 255);
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
    int width = 32, height = 16;
    gVertexBuffer.resize(gNumVertices);
    gNormals.resize(gNumVertices, { 0, 0, 0 });

    int t = 0;
    for (int j = 1; j < height - 1; ++j)
        for (int i = 0; i < width; ++i) {
            float theta = (float)j / (height - 1) * M_PI;
            float phi = (float)i / (width - 1) * M_PI * 2;
            gVertexBuffer[t++] = { sinf(theta) * cosf(phi), cosf(theta), -sinf(theta) * sinf(phi) };
        }
    gVertexBuffer[t++] = { 0, 1, 0 };
    gVertexBuffer[t++] = { 0, -1, 0 };

    gNumTriangles = ((height - 3) * (width - 1) * 2) + 2 * (width - 1);

    for (int i = 0; i < gNumTriangles; ++i) {
        int i0 = gIndexBuffer[i * 3], i1 = gIndexBuffer[i * 3 + 1], i2 = gIndexBuffer[i * 3 + 2];
        Vec3 w0 = modelTransform(gVertexBuffer[i0]);
        Vec3 w1 = modelTransform(gVertexBuffer[i1]);
        Vec3 w2 = modelTransform(gVertexBuffer[i2]);
        Vec3 n = normalize(cross(w1 - w0, w2 - w0));
        gNormals[i0] = gNormals[i0] + n;
        gNormals[i1] = gNormals[i1] + n;
        gNormals[i2] = gNormals[i2] + n;
    }
    for (int i = 0; i < gNumVertices; ++i) gNormals[i] = normalize(gNormals[i]);

    fill(&framebuffer[0][0][0], &framebuffer[0][0][0] + WIDTH * HEIGHT * 3, 0);
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x)
            zBuffer[y][x] = 1e9;

    for (int i = 0; i < gNumTriangles; ++i) {
        int i0 = gIndexBuffer[i * 3], i1 = gIndexBuffer[i * 3 + 1], i2 = gIndexBuffer[i * 3 + 2];
        Vec3 w0 = modelTransform(gVertexBuffer[i0]);
        Vec3 w1 = modelTransform(gVertexBuffer[i1]);
        Vec3 w2 = modelTransform(gVertexBuffer[i2]);
        Vec3 v0 = viewportTransform(projectionTransform(w0));
        Vec3 v1 = viewportTransform(projectionTransform(w1));
        Vec3 v2 = viewportTransform(projectionTransform(w2));
        rasterizePhong(v0, w0, gNormals[i0], v1, w1, gNormals[i1], v2, w2, gNormals[i2]);
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Phong Shading Output");
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glPixelZoom(1.0f, -1.0f);
    glRasterPos2i(0, HEIGHT - 1);
    glutDisplayFunc(renderScene);
    glutMainLoop();
    return 0;
}

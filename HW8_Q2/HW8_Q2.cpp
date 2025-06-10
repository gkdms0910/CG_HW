#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <fstream>
#include <float.h>
#include <algorithm> 

#include <GL/glew.h>
#include <GL/glut.h>

// ----------------------------------------------------------------------------
// 구조체 및 전역 변수
// ----------------------------------------------------------------------------
struct Vector3 { float x, y, z; };
struct Triangle { unsigned int indices[3]; };

std::vector<Vector3>  gPositions;
std::vector<Vector3>  gNormals;
std::vector<Triangle> gTriangles;

float  gTotalTimeElapsed = 0.0f;
int    gTotalFrames = 0;
GLuint gTimer;

GLuint gVBO_positions;
GLuint gVBO_normals;
GLuint gEBO;

// ----------------------------------------------------------------------------
// OBJ 로딩 (샘플 코드 그대로 사용)
// ----------------------------------------------------------------------------
void tokenize(char* str, std::vector<std::string>& tok, const char* del) {
    char* t = strtok(str, del);
    while (t) {
        tok.emplace_back(t);
        t = strtok(NULL, del);
    }
}

int face_index(const char* s) {
    int len = strlen(s);
    char* c = new char[len + 1]; memset(c, 0, len + 1);
    strcpy(c, s);
    std::vector<std::string> tok;
    tokenize(c, tok, "/");
    delete[] c;
    if (tok.front().size() > 0 && tok.back().size() > 0
        && atoi(tok.front().c_str()) == atoi(tok.back().c_str()))
        return atoi(tok.front().c_str());
    printf("ERROR: Bad face spec!\n"); exit(0);
}

void load_mesh(const std::string& fn) {
    std::ifstream in(fn);
    if (!in.is_open()) {
        printf("ERROR: cannot open %s\n", fn.c_str());
        exit(0);
    }
    float xmin = FLT_MAX, xmax = -FLT_MAX;
    float ymin = FLT_MAX, ymax = -FLT_MAX;
    float zmin = FLT_MAX, zmax = -FLT_MAX;
    while (!in.eof()) {
        char line[1024] = { 0 };
        in.getline(line, 1024);
        if (strlen(line) <= 1) continue;
        std::vector<std::string> tok;
        tokenize(line, tok, " ");
        if (tok[0] == "v") {
            float x = atof(tok[1].c_str()),
                y = atof(tok[2].c_str()),
                z = atof(tok[3].c_str());
            xmin = std::min(xmin, x); xmax = std::max(xmax, x);
            ymin = std::min(ymin, y); ymax = std::max(ymax, y);
            zmin = std::min(zmin, z); zmax = std::max(zmax, z);
            gPositions.push_back({ x,y,z });
        }
        else if (tok[0] == "vn") {
            float x = atof(tok[1].c_str()),
                y = atof(tok[2].c_str()),
                z = atof(tok[3].c_str());
            gNormals.push_back({ x,y,z });
        }
        else if (tok[0] == "f") {
            unsigned a = face_index(tok[1].c_str()) - 1;
            unsigned b = face_index(tok[2].c_str()) - 1;
            unsigned c = face_index(tok[3].c_str()) - 1;
            gTriangles.push_back({ a,b,c });
        }
    }
    in.close();
    printf("Loaded mesh: %lu verts, %lu norms, %lu tris\n",
        gPositions.size(), gNormals.size(), gTriangles.size());
    printf("BBox: (%.4f,%.4f,%.4f) to (%.4f,%.4f,%.4f)\n",
        xmin, ymin, zmin, xmax, ymax, zmax);
}

// ----------------------------------------------------------------------------
// 타이머
// ----------------------------------------------------------------------------
void init_timer() { glGenQueries(1, &gTimer); }
void start_timing() { glBeginQuery(GL_TIME_ELAPSED, gTimer); }
float stop_timing() {
    glEndQuery(GL_TIME_ELAPSED);
    GLint avail = 0, result = 0;
    while (!avail) glGetQueryObjectiv(gTimer, GL_QUERY_RESULT_AVAILABLE, &avail);
    glGetQueryObjectiv(gTimer, GL_QUERY_RESULT, &result);
    return result * 1e-9f;
}

// ----------------------------------------------------------------------------
// 버퍼 생성 (VBO/EBO)
// ----------------------------------------------------------------------------
void init_buffers() {
    glGenBuffers(1, &gVBO_positions);
    glBindBuffer(GL_ARRAY_BUFFER, gVBO_positions);
    glBufferData(GL_ARRAY_BUFFER,
        gPositions.size() * sizeof(Vector3),
        gPositions.data(),
        GL_STATIC_DRAW);

    glGenBuffers(1, &gVBO_normals);
    glBindBuffer(GL_ARRAY_BUFFER, gVBO_normals);
    glBufferData(GL_ARRAY_BUFFER,
        gNormals.size() * sizeof(Vector3),
        gNormals.data(),
        GL_STATIC_DRAW);

    glGenBuffers(1, &gEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        gTriangles.size() * sizeof(Triangle),
        gTriangles.data(),
        GL_STATIC_DRAW);

    // unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// ----------------------------------------------------------------------------
// OpenGL 초기화 (과제 지정 파라미터에 맞춤)
// ----------------------------------------------------------------------------
void init_gl() {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // 기본 Gouraud shading
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);

    // 고정 파이프라인 조명
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Ia = (0.2,0.2,0.2)
    GLfloat Ia[] = { 0.2f,0.2f,0.2f,1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Ia);

    // Light0: ambient=(0,0,0), diffuse=(1,1,1), specular=(0,0,0), dir=(−1,−1,−1,0)
    GLfloat La[] = { 0,0,0,1 };
    GLfloat Ld[] = { 1,1,1,1 };
    GLfloat Ls[] = { 0,0,0,1 };
    GLfloat Lpos[] = { -1,-1,-1,0 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, La);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, Ld);
    glLightfv(GL_LIGHT0, GL_SPECULAR, Ls);
    glLightfv(GL_LIGHT0, GL_POSITION, Lpos);

    // Material: ka=kd=white, ks=zero, p=0
    GLfloat mA[] = { 1,1,1,1 };
    GLfloat mD[] = { 1,1,1,1 };
    GLfloat mS[] = { 0,0,0,1 };
    glMaterialfv(GL_FRONT, GL_AMBIENT, mA);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mD);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mS);
    glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
}

// ----------------------------------------------------------------------------
// 창 리사이즈 콜백
// ----------------------------------------------------------------------------
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

// ----------------------------------------------------------------------------
// 렌더링 루프
// ----------------------------------------------------------------------------
void display() {
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 투영
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.1, 0.1, -0.1, 0.1, 0.1, 1000.0);

    // 모델뷰
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.1f, -1.0f, -1.5f);
    glScalef(10.0f, 10.0f, 10.0f);

    start_timing();

    // VBO/EBO 바인딩
    glBindBuffer(GL_ARRAY_BUFFER, gVBO_positions);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, gVBO_normals);
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT, 0, (void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBO);
    glDrawElements(GL_TRIANGLES,
        (GLsizei)gTriangles.size() * 3,
        GL_UNSIGNED_INT,
        (void*)0);

    // 정리
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    float t = stop_timing();
    gTotalFrames++;
    gTotalTimeElapsed += t;
    float fps = gTotalTimeElapsed > 0
        ? (float)gTotalFrames / gTotalTimeElapsed
        : 0.0f;

    char buf[128];
    sprintf(buf, "Q2: Vertex Arrays | OpenGL Bunny: %0.2f FPS", fps);
    glutSetWindowTitle(buf);

    glutSwapBuffers();
    glutPostRedisplay();
}

// ----------------------------------------------------------------------------
// 메인
// ----------------------------------------------------------------------------
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(1280, 1280);
    glutCreateWindow("OpenGL Bunny");

    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "GLEW init failed\n");
        return -1;
    }

    load_mesh("bunny.obj");
    init_gl();
    init_buffers();
    init_timer();

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}

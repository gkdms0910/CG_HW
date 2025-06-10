#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm> // For std::min/max

#include <GL/glew.h>
#include <GL/glut.h>

// ----------------------------------------------------------------------------
// 3D 벡터와 삼각형 구조체 정의
// ----------------------------------------------------------------------------
struct Vector3
{
    float x, y, z;
};

struct Triangle
{
    unsigned int indices[3];
};

// ----------------------------------------------------------------------------
// 전역 변수: 메시 데이터 및 타이머 관련
// ----------------------------------------------------------------------------
// load_mesh.cpp 에서 가져온 변수
std::vector<Vector3>    gPositions;
std::vector<Vector3>    gNormals;
std::vector<Triangle>   gTriangles;

// frame_timer.cpp 에서 가져온 변수
float  gTotalTimeElapsed = 0;
int    gTotalFrames = 0;
GLuint gTimer;

// ----------------------------------------------------------------------------
// load_mesh.cpp의 함수들
// ----------------------------------------------------------------------------

// 문자열을 구분자로 나누는 함수
void tokenize(char* string, std::vector<std::string>& tokens, const char* delimiter)
{
    char* token = strtok(string, delimiter);
    while (token != NULL)
    {
        tokens.push_back(std::string(token));
        token = strtok(NULL, delimiter);
    }
}

// "f v/vt/vn" 형식의 obj 파일 face 인덱스를 파싱하는 함수
int face_index(const char* string)
{
    int length = strlen(string);
    char* copy = new char[length + 1];
    memset(copy, 0, length + 1);
    strcpy(copy, string);

    std::vector<std::string> tokens;
    tokenize(copy, tokens, "/");
    delete[] copy;
    if (tokens.front().length() > 0 && tokens.back().length() > 0 && atoi(tokens.front().c_str()) == atoi(tokens.back().c_str()))
    {
        return atoi(tokens.front().c_str());
    }
    else
    {
        printf("ERROR: Bad face specifier!\n");
        exit(0);
    }
}

// .obj 파일에서 메시 데이터를 로드하는 함수
void load_mesh(std::string fileName)
{
    std::ifstream fin(fileName.c_str());
    if (!fin.is_open())
    {
        printf("ERROR: Unable to load mesh from %s!\n", fileName.c_str());
        exit(0);
    }

    float xmin = FLT_MAX;
    float xmax = -FLT_MAX;
    float ymin = FLT_MAX;
    float ymax = -FLT_MAX;
    float zmin = FLT_MAX;
    float zmax = -FLT_MAX;

    while (true)
    {
        char line[1024] = { 0 };
        fin.getline(line, 1024);

        if (fin.eof())
            break;

        if (strlen(line) <= 1)
            continue;

        std::vector<std::string> tokens;
        tokenize(line, tokens, " ");

        if (tokens[0] == "v")
        {
            float x = atof(tokens[1].c_str());
            float y = atof(tokens[2].c_str());
            float z = atof(tokens[3].c_str());

            xmin = std::min(x, xmin);
            xmax = std::max(x, xmax);
            ymin = std::min(y, ymin);
            ymax = std::max(y, ymax);
            zmin = std::min(z, zmin);
            zmax = std::max(z, zmax);

            Vector3 position = { x, y, z };
            gPositions.push_back(position);
        }
        else if (tokens[0] == "vn")
        {
            float x = atof(tokens[1].c_str());
            float y = atof(tokens[2].c_str());
            float z = atof(tokens[3].c_str());
            Vector3 normal = { x, y, z };
            gNormals.push_back(normal);
        }
        else if (tokens[0] == "f")
        {
            unsigned int a = face_index(tokens[1].c_str());
            unsigned int b = face_index(tokens[2].c_str());
            unsigned int c = face_index(tokens[3].c_str());
            Triangle triangle;
            triangle.indices[0] = a - 1;
            triangle.indices[1] = b - 1;
            triangle.indices[2] = c - 1;
            gTriangles.push_back(triangle);
        }
    }

    fin.close();

    printf("Loaded mesh from %s. (%lu vertices, %lu normals, %lu triangles)\n", fileName.c_str(), gPositions.size(), gNormals.size(), gTriangles.size());
    printf("Mesh bounding box is: (%0.4f, %0.4f, %0.4f) to (%0.4f, %0.4f, %0.4f)\n", xmin, ymin, zmin, xmax, ymax, zmax);
}


// ----------------------------------------------------------------------------
// frame_timer.cpp의 함수들
// ----------------------------------------------------------------------------
void init_timer()
{
    glGenQueries(1, &gTimer);
}

void start_timing()
{
    glBeginQuery(GL_TIME_ELAPSED, gTimer);
}

float stop_timing()
{
    glEndQuery(GL_TIME_ELAPSED);

    GLint available = GL_FALSE;
    while (available == GL_FALSE)
        glGetQueryObjectiv(gTimer, GL_QUERY_RESULT_AVAILABLE, &available);

    GLint result;
    glGetQueryObjectiv(gTimer, GL_QUERY_RESULT, &result);

    float timeElapsed = result / 1000000000.0f; // 나노초를 초로 변환
    return timeElapsed;
}

// ----------------------------------------------------------------------------
// OpenGL 초기화 함수
// ----------------------------------------------------------------------------
void init_gl()
{
    // Shading Parameters
    glEnable(GL_DEPTH_TEST); // 깊이 버퍼 활성화 
    glDisable(GL_CULL_FACE); // 후면 제거 비활성화 

    glEnable(GL_LIGHTING);   // 조명 활성화
    glEnable(GL_LIGHT0);     // 0번 광원 활성화

    // 전역 주변광 설정 (Ia)
    GLfloat ambientLight[] = { 0.2f, 0.2f, 0.2f, 1.0f }; // 
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight);

    // 재질 속성 설정
    GLfloat materialAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // ka 
    GLfloat materialDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // kd 
    GLfloat materialSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // ks 
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, 0.0f); // p 

    // 방향성 광원(LIGHT0) 설정
    GLfloat lightAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };  // 
    GLfloat lightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };  // 
    GLfloat lightSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 
    GLfloat lightPosition[] = { -1.0f, -1.0f, -1.0f, 0.0f }; // 방향 (w=0) 
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glEnable(GL_NORMALIZE); // 법선 벡터 자동 정규화 활성화
}

// ----------------------------------------------------------------------------
// 렌더링을 위한 디스플레이 콜백 함수
// ----------------------------------------------------------------------------
void display()
{
    // 화면과 깊이 버퍼 초기화
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Projection Transform 설정
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // l=-0.1, r=0.1, b=-0.1, t=0.1, n=0.1, f=1000 
    // PDF의 n, f 값은 음수지만 OpenGL의 glFrustum은 양수 값을 사용함 
    glFrustum(-0.1, 0.1, -0.1, 0.1, 0.1, 1000.0);

    // ModelView Transform 설정
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // 카메라는 (0,0,0)에 위치하며 기본 축을 사용하므로 gluLookAt은 불필요 

    // Bunny Transform
    glTranslatef(0.1f, -1.0f, -1.5f); // 
    glScalef(10.0f, 10.0f, 10.0f);    // 

    // --- 렌더링 시간 측정 시작 ---
    start_timing();

    // Q1: Immediate Mode로 토끼 렌더링 
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < gTriangles.size(); ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            unsigned int vertexIndex = gTriangles[i].indices[j];

            const Vector3& normal = gNormals[vertexIndex];
            glNormal3f(normal.x, normal.y, normal.z);

            const Vector3& position = gPositions[vertexIndex];
            glVertex3f(position.x, position.y, position.z);
        }
    }
    glEnd();

    // --- 렌더링 시간 측정 종료 및 FPS 계산 ---
    float timeElapsed = stop_timing();
    gTotalFrames++;
    gTotalTimeElapsed += timeElapsed;
    if (gTotalTimeElapsed > 0) // 0으로 나누는 것을 방지
    {
        float fps = gTotalFrames / gTotalTimeElapsed;
        char string[1024] = { 0 };
        sprintf(string, "Q1: Immediate Mode | OpenGL Bunny: %.2f FPS", fps);
        glutSetWindowTitle(string);
    }

    glutSwapBuffers();
    glutPostRedisplay();
}


// ----------------------------------------------------------------------------
// 메인 함수
// ----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(1280, 1280); // Viewport nx=ny=1280 
    glutCreateWindow("OpenGL Bunny");

    glewInit();

    // 메시 로딩 및 OpenGL 초기화
    load_mesh("bunny.obj");
    init_gl();
    init_timer();

    // 콜백 함수 등록
    glutDisplayFunc(display);

    // 메인 루프 시작
    glutMainLoop();

    return 0;
}
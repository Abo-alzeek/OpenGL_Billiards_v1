#include <GL/glut.h>
#include <cmath>

// Function to draw a rectangle
void drawRectangle(float x, float y, float z, float width, float depth, float height, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);

    // Top face
    glVertex3f(x, y, z);
    glVertex3f(x + width, y, z);
    glVertex3f(x + width, y, z + depth);
    glVertex3f(x, y, z + depth);

    // Bottom face
    glVertex3f(x, y - height, z);
    glVertex3f(x + width, y - height, z);
    glVertex3f(x + width, y - height, z + depth);
    glVertex3f(x, y - height, z + depth);

    // Front face
    glVertex3f(x, y - height, z);
    glVertex3f(x + width, y - height, z);
    glVertex3f(x + width, y, z);
    glVertex3f(x, y, z);

    // Back face
    glVertex3f(x, y - height, z + depth);
    glVertex3f(x + width, y - height, z + depth);
    glVertex3f(x + width, y, z + depth);
    glVertex3f(x, y, z + depth);

    // Left face
    glVertex3f(x, y - height, z);
    glVertex3f(x, y - height, z + depth);
    glVertex3f(x, y, z + depth);
    glVertex3f(x, y, z);

    // Right face
    glVertex3f(x + width, y - height, z);
    glVertex3f(x + width, y - height, z + depth);
    glVertex3f(x + width, y, z + depth);
    glVertex3f(x + width, y, z);

    glEnd();
}

// Function to draw a cylinder (for legs)
void drawCylinder(float x, float y, float z, float radius, float height, float r, float g, float b) {
    glColor3f(r, g, b);
    int slices = 32;

    // Draw cylinder sides
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; i++) {
        float angle = 2.0f * M_PI * i / slices;
        float dx = radius * cos(angle);
        float dz = radius * sin(angle);
        glVertex3f(x + dx, y, z + dz);
        glVertex3f(x + dx, y - height, z + dz);
    }
    glEnd();

    // Draw top and bottom circles
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(x, y, z); // Center of top circle
    for (int i = 0; i <= slices; i++) {
        float angle = 2.0f * M_PI * i / slices;
        float dx = radius * cos(angle);
        float dz = radius * sin(angle);
        glVertex3f(x + dx, y, z + dz);
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(x, y - height, z); // Center of bottom circle
    for (int i = 0; i <= slices; i++) {
        float angle = 2.0f * M_PI * i / slices;
        float dx = radius * cos(angle);
        float dz = radius * sin(angle);
        glVertex3f(x + dx, y - height, z + dz);
    }
    glEnd();
}

// Function to draw the billiards table
void drawTable() {
    // Table Base
    drawRectangle(-1.4, 0.1, -0.7, 2.8, 1.4, 0.1, 0.0, 0.5, 0.0);

    // Rails
    drawRectangle(-1.5, 0.2, -0.8, 3.0, 0.1, 0.2, 0.4, 0.2, 0.0); // Top Rail
    drawRectangle(-1.5, 0.2, 0.7, 3.0, 0.1, 0.2, 0.4, 0.2, 0.0);  // Bottom Rail
    drawRectangle(-1.5, 0.2, -0.7, 0.1, 1.4, 0.2, 0.4, 0.2, 0.0); // Left Rail
    drawRectangle(1.4, 0.2, -0.7, 0.1, 1.4, 0.2, 0.4, 0.2, 0.0);  // Right Rail

    // Legs
    drawCylinder(-1.3, 0.1, -0.6, 0.1, 0.5, 0.3, 0.2, 0.1);
    drawCylinder(1.3, 0.1, -0.6, 0.1, 0.5, 0.3, 0.2, 0.1);
    drawCylinder(-1.3, 0.1, 0.6, 0.1, 0.5, 0.3, 0.2, 0.1);
    drawCylinder(1.3, 0.1, 0.6, 0.1, 0.5, 0.3, 0.2, 0.1);

    // Pockets (can add more detail later if needed)
}

// OpenGL Display Function
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    gluLookAt(0.0, 1.5, 4.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    drawTable();

    glutSwapBuffers();
}

// OpenGL Initialization
void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

// Main Function
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Billiards Table");

    init();
    glutDisplayFunc(display);
    glutMainLoop();

    return 0;
}


// g++ addingTable.cpp -o table -lGL -lGLU -lglut -lX11 && ./table

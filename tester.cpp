#include <GL/glut.h>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>       // Core GLM functions
#include <glm/gtc/matrix_transform.hpp>  // For matrix transformations like lookAt
#include <glm/gtc/type_ptr.hpp> // For converting glm types to OpenGL types (e.g., mat4 to float*)
#include <X11/Xlib.h>

// screen resolutions
const float EPS = 1e-6;
const int screen_width = 1200;
const int screen_height = 800;

bool keys[256];  // Array to keep track of key presses

class Camera {
public:
    glm::vec3 position = glm::vec3(0, 5, 15);
    glm::vec3 look = glm::vec3(0, 0, -1);
    glm::vec3 up = glm::vec3(0, 1, 0);

    float cameraMovementSpeed = 0.025;
    float cameraSensitivity = 0.001;
} camera;

class Ball {
public:
    float radius;
    glm::vec3 position, color;

    Ball() { }
    
    Ball(float r) {
        radius = r;
    }
    
    Ball(float rad, float r, float g, float b) {
        radius = rad;
        color = {r, g, b};
    }

    Ball(float rad, float r, float g, float b, float x, float y, float z) {
        radius = rad;
        color = glm::vec3(r, g, b);
        position = glm::vec3(x, y, z);
    }

    // Function to draw a colored sphere
    void drawColoredSphere() {
        glColor3f(color.x, color.y, color.z); // Set the color (RGB)
        glPushMatrix(); // Save the current transformation matrix
        glTranslatef(position.x, position.y, position.z); // Move the sphere to the desired position
        glutSolidSphere(radius, 50, 50); // Draw the sphere
        glPopMatrix(); // Restore the previous transformation matrix
    }
};

Ball balls[16]; // zero is the cue ball

void rotatVector(glm::vec3 &vector, glm::vec3 axis, float angle) {
    // Create the rotation matrix
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, axis);

    // Rotate the vector (multiply it by the rotation matrix)
    glm::vec4 rotatedVector = rotationMatrix * glm::vec4(vector, 1.0f);

    vector.x = rotatedVector.x;
    vector.y = rotatedVector.y;
    vector.z = rotatedVector.z;    
}

// Function to draw a circle (for a pocket)
void drawCircle(float x, float y, float z, float radius, float depth, float r, float g, float b) {
    glColor3f(r, g, b);
    int segments = 32;

    // Draw pocket's top opening as a circle
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(x, y, z); // Center of the circle
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float dx = radius * cos(angle);
        float dz = radius * sin(angle);
        glVertex3f(x + dx, y, z + dz);
    }
    glEnd();

    // Optional: Draw pocket depth as a vertical cylinder
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float dx = radius * cos(angle);
        float dz = radius * sin(angle);
        glVertex3f(x + dx, y, z + dz);         // Top edge
        glVertex3f(x + dx, y - depth, z + dz); // Bottom edge
    }
    glEnd();
}

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
    drawRectangle(-1.4, 0.5, -0.7, 2.8, 1.4, 0.1, 0.0, 0.5, 0.0);

    // Rails
    drawRectangle(-1.5, 0.6, -0.8, 3.0, 0.1, 0.2, 0.4, 0.2, 0.0); // Top Rail
    drawRectangle(-1.5, 0.6, 0.7, 3.0, 0.1, 0.2, 0.4, 0.2, 0.0);  // Bottom Rail
    drawRectangle(-1.5, 0.6, -0.7, 0.1, 1.4, 0.2, 0.4, 0.2, 0.0); // Left Rail
    drawRectangle(1.4, 0.6, -0.7, 0.1, 1.4, 0.2, 0.4, 0.2, 0.0);  // Right Rail

    // Legs
    drawCylinder(-1.3, 0.49, -0.6, 0.1, 0.5, 0.3, 0.2, 0.1);
    drawCylinder(1.3, 0.49, -0.6, 0.1, 0.5, 0.3, 0.2, 0.1);
    drawCylinder(-1.3, 0.49, 0.6, 0.1, 0.5, 0.3, 0.2, 0.1);
    drawCylinder(1.3, 0.49, 0.6, 0.1, 0.5, 0.3, 0.2, 0.1);

    float pocketRadius = 0.1;
    float pocketDepth = 0.2;

    // Corner pockets
    drawCircle(-1.4, 0.52, -0.7, pocketRadius, pocketDepth, 0.0, 0.0, 0.0); // Bottom-left
    drawCircle(1.4, 0.52, -0.7, pocketRadius, pocketDepth, 0.0, 0.0, 0.0);  // Bottom-right
    drawCircle(-1.4, 0.52, 0.7, pocketRadius, pocketDepth, 0.0, 0.0, 0.0);  // Top-left
    drawCircle(1.4, 0.52, 0.7, pocketRadius, pocketDepth, 0.0, 0.0, 0.0);   // Top-right

    // Side pockets
    drawCircle(0.0, 0.52, -0.7, pocketRadius, pocketDepth, 0.0, 0.0, 0.0);  // Bottom-center
    drawCircle(0.0, 0.52, 0.7, pocketRadius, pocketDepth, 0.0, 0.0, 0.0);   // Top-center
}

// Function to handle mouse movement
void mouseMotion(int x, int y) {
    if(x == screen_width / 2) {
        // std::cout << "RETURNING" << std::endl;
        return;
    }

    float deltaX = (screen_width/2 - x) * camera.cameraSensitivity; // Calculate the change in mouse position

    // Vector to rotate
    glm::vec3 vector(camera.look.x, camera.look.y, camera.look.z);
    rotatVector(vector, glm::vec3(camera.up.x, camera.up.y, camera.up.z), asin(deltaX / sqrt(deltaX * deltaX + 1 * 1)));

    camera.look.x = vector.x;
    camera.look.y = vector.y;
    camera.look.z = vector.z;
    
    // glm::vec3 tempVec = glm::normalize(glm::vec3(cameraLookAtX + deltaX * sensitivity - cameraX, cameraLookAtY - cameraY, cameraLookAtZ - cameraZ));
    // std::cout << "CAMERA LOCATION: " << cameraX << " " << cameraY << " " << cameraZ << ", CAMERA LOOK AT LOCATION: " << cameraLookAtX << " " << cameraLookAtY << " " << cameraLookAtZ << std::endl;
}

// Function to handle the camera movement based on input
void handleKeys() {
    if (keys[27]) { // Escape key to exit
        exit(0);
    }

    // Camera movement based on key press
    if (keys['w']) {  // Move forward
        camera.position.x += camera.look.x * camera.cameraMovementSpeed;
        camera.position.z += camera.look.z * camera.cameraMovementSpeed;
    }
    if (keys['s']) {  // Move backward
        glm::vec3 lookVec = glm::vec3(camera.look.x, camera.look.y, camera.look.z);
        rotatVector(lookVec, glm::vec3(camera.up.x, camera.up.y, camera.up.z), acos(-1));
        
        camera.position.x += lookVec.x * camera.cameraMovementSpeed;
        camera.position.z += lookVec.z * camera.cameraMovementSpeed;
    }
    if (keys['a']) {  // Strafe left
        glm::vec3 lookVec = glm::vec3(camera.look.x, camera.look.y, camera.look.z);
        rotatVector(lookVec, glm::vec3(camera.up.x, camera.up.y, camera.up.z), acos(-1)/2.0);
        
        camera.position.x += lookVec.x * camera.cameraMovementSpeed;
        camera.position.z += lookVec.z * camera.cameraMovementSpeed;
    }
    if (keys['d']) {  // Strafe right
        glm::vec3 lookVec = glm::vec3(camera.look.x, camera.look.y, camera.look.z);
        rotatVector(lookVec, glm::vec3(camera.up.x, camera.up.y, camera.up.z), -acos(-1)/2.0);
        
        camera.position.x += lookVec.x * camera.cameraMovementSpeed;
        camera.position.z += lookVec.z * camera.cameraMovementSpeed;
    }
    if (keys['z']) {
        camera.position.y += camera.cameraMovementSpeed;
    }
    if(keys['x']) {
        camera.position.y -= camera.cameraMovementSpeed;
    }
}

// Function to reposition the mouse to the center of the screen
void recenterMouse(Display* display, Window root) {
    // Get screen dimensions
    Screen* screen = DefaultScreenOfDisplay(display);
    int screenWidth = screen->width;
    int screenHeight = screen->height;

    // Center coordinates
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;

    // Move the mouse cursor to the center of the screen
    XWarpPointer(display, None, root, 0, 0, 0, 0, centerX, centerY);

    // Ensure the command is executed
    XFlush(display);
}

void timer(int value) {
    // Open a connection to the X server
    Display* display = XOpenDisplay(NULL);
    if (!display) {
        std::cerr << "Unable to open X display" << std::endl;
        exit(1);
    }

    // Get the root window
    Window root = DefaultRootWindow(display);

    // Recenter the mouse
    recenterMouse(display, root);

    // Close the display connection
    XCloseDisplay(display);

    // Set the timer again for periodic updates (optional)
    glutTimerFunc(1000 / 60, timer, 0); // Call every 1000 ms (1 second)
}

// Function to render the platform
void renderPlatform() {
    glBegin(GL_QUADS); // Start drawing a quadrilateral (platform)
    
    // Define the four corners of the platform
    glColor3f(0.6f, 0.6f, 0.6f); // Set platform color to gray
    glVertex3f(-20.0f, 0.0f, -20.0f);  // Bottom-left
    glVertex3f(20.0f, 0.0f, -20.0f);   // Bottom-right
    glVertex3f(20.0f, 0.0f, 20.0f);    // Top-right
    glVertex3f(-20.0f, 0.0f, 20.0f);   // Top-left

    glEnd();  // Finish drawing
}

// Display callback function
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the screen

    glLoadIdentity(); // Reset transformations
    
    // Set the camera position using gluLookAt (eye position, center, up direction)
    gluLookAt(camera.position.x, camera.position.y, camera.position.z, // Camera position (x, y, z)
              camera.position.x + camera.look.x, camera.position.y + camera.look.y, camera.position.z + camera.look.z,  // Look at a point 1 unit in front of the camera
              camera.up.x, camera.up.y, camera.up.z); // Up direction (y-axis)

    // Render the platform
    renderPlatform();

    // Draw the table
    drawTable();

    for(int i = 0;i < 16;i++) {
        balls[i].drawColoredSphere();
    }

    glutSwapBuffers(); // Swap buffers to display the rendered scene
}

// Function to initialize OpenGL settings
void init() {
    glEnable(GL_DEPTH_TEST);  // Enable depth testing for proper 3D rendering
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color to black

    balls[0] = Ball(0.05, 1.0, 1.0, 1.0, 2.0 + (-1.15), 0.55, 1.0 + (-1.0));

    balls[1] = Ball(0.05, 0.0, 1.0, 0.0, 1.0 + (-1.7), 0.55, 1.0 + (-1.0));
    balls[2] = Ball(0.05, 0.0, 0.0, 1.0, 1.0 + (-1.7), 0.55, 1.05 + (-1.0));
    balls[3] = Ball(0.05, 1.0, 1.0, 0.0, 1.0 + (-1.7), 0.55, 0.95 + (-1.0));
    balls[4] = Ball(0.05, 1.0, 0.0, 1.0, 1.0 + (-1.7), 0.55, 1.1 + (-1.0));
    balls[5] = Ball(0.05, 0.0, 1.0, 1.0, 1.0 + (-1.7), 0.55, 0.9 + (-1.0));

    balls[6] = Ball(0.05, 0.7, 1.0, 0.2, 1.05 + (-1.7), 0.55, 1.025 + (-1.0));
    balls[7] = Ball(0.05, 1.0, 0.0, 0.0, 1.05 + (-1.7), 0.55, 1.075 + (-1.0));
    
    balls[8] =  Ball(0.05, 0.0, 0.0, 0.0, 1.1 + (-1.7), 0.55, 1.0 + (-1.0));

    balls[9] =  Ball(0.05, 1.0, 0.0, 0.0, 1.05 + (-1.7), 0.55, 0.975 + (-1.0));
    balls[10] = Ball(0.05, 1.0, 0.0, 0.0, 1.0 + (-1.7), 0.55, 0.925 + (-1.0));

    balls[11] = Ball(0.05, 1.0, 0.0, 0.0, 1.1 + (-1.7), 0.55, 1.05 + (-1.0));
    balls[12] = Ball(0.05, 1.0, 0.0, 0.0, 1.1 + (-1.7), 0.55, 0.95 + (-1.0));

    balls[13] = Ball(0.05, 1.0, 0.0, 0.0, 1.15 + (-1.7), 0.55, 1.025 + (-1.0));
    balls[14] = Ball(0.05, 1.0, 0.0, 0.0, 1.15 + (-1.7), 0.55, 0.975 + (-1.0));
    balls[15] = Ball(0.05, 1.0, 0.0, 0.0, 1.2 + (-1.7), 0.55, 1.0 + (-1.0));
}

// Function to handle window resizing
void reshape(int w, int h) {
    glViewport(0, 0, w, h); // Set the viewport size
    glMatrixMode(GL_PROJECTION); // Switch to projection matrix
    glLoadIdentity(); // Reset projection matrix
    gluPerspective(45.0f, (double)w / (double)h, 1.0f, 100.0f); // Set the perspective view
    glMatrixMode(GL_MODELVIEW); // Switch back to modelview matrix
}

// Keyboard input callback (key press)
void keyboard(unsigned char key, int x, int y) {
    keys[key] = true;
}

// Keyboard input callback (key release)
void keyboardUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

// Idle callback to handle continuous movement
void idle() {
    // updateCamera();
    handleKeys();  // Update the camera position based on input
    glutPostRedisplay();  // Request a redraw to update the scene
}

// Main function
int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(screen_width, screen_height);  // Set initial window size
    glutCreateWindow("8 Ball Pool");

    // Initialize OpenGL settings
    init();

    // Hide the mouse cursor
    glutSetCursor(GLUT_CURSOR_NONE);  // Hide the cursor in the window

    // Set callback functions
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutIdleFunc(idle);
    glutMotionFunc(mouseMotion);  // Register mouse motion callback
    glutPassiveMotionFunc(mouseMotion);  // Also capture passive mouse movement

    glutTimerFunc(0, timer, 0); // Set a timer to periodically reposition the cursor

    // Start the GLUT main loop
    glutMainLoop();

    return 0;
}


// g++ -I/usr/include/glm -I/usr/include/GL tester.cpp -o test -Lls /usr/lib/libglut.so -lGL -lGLU -lglut -lX11
// g++ tester.cpp -o test -lGL -lGLU -lglut -lX11 && ./test

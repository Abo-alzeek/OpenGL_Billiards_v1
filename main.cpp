#include <GL/glut.h>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>                  // Core GLM functions
#include <glm/gtc/matrix_transform.hpp> // For matrix transformations like lookAt
#include <glm/gtc/type_ptr.hpp>         // For converting glm types to OpenGL types (e.g., mat4 to float*)
#include <X11/Xlib.h>

// screen resolutions
const float EPS = 1e-6;
const int screen_width = 1200;
const int screen_height = 800;

const float pocketRadius = 0.1;
const float pocketDepth = 0.2;
const float table_width = 2.8f;
const float table_height = 1.4;

const int balls_count = 16;
bool isMousePressed = false;
float strength = 0.0;

bool keys[256];  // Array to keep track of key presses

float distance(glm::vec3 p1, glm::vec3 p2 = {0, 0 ,0}) {
    return sqrt( (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y) + (p1.z - p2.z) * (p1.z - p2.z) );
}

class Camera {
public:
    glm::vec3 position = glm::vec3(0, 1.3, 5);
    glm::vec3 look = glm::vec3(0, 0, -1);
    glm::vec3 up = glm::vec3(0, 1, 0);

    float cameraMovementSpeed = 0.01;
    float cameraSensitivity = 0.001;

    glm::vec3 getPropperVector() {
        glm::vec3 ret = look;
        ret.y = position.y;
        ret /= distance(ret);
        return ret;
    }

} camera;

class Ball {
public:
    bool active = true;
    float radius, mass = 1.0;
    glm::vec3 position, color, velocity = {0, 0, 0};

    Ball() {
        active = true;
    }
    
    Ball(float r) {
        radius = r;
        active = true;
    }
    
    Ball(float rad, float r, float g, float b) {
        radius = rad;
        color = {r, g, b};
        active = true;
    }

    Ball(float rad, float r, float g, float b, float x, float y, float z) {
        radius = rad;
        color = glm::vec3(r, g, b);
        position = glm::vec3(x, y, z);
        active = true;
    }

    // Function to draw a colored sphere
    void drawColoredSphere() {
        glColor3f(color.x, color.y, color.z); // Set the color (RGB)
        glPushMatrix(); // Save the current transformation matrix
        glTranslatef(position.x, position.y, position.z); // Move the sphere to the desired position
        glutSolidSphere(radius, 50, 50); // Draw the sphere
        glPopMatrix(); // Restore the previous transformation matrix
    }

    void setMovement(glm::vec3 v) {
        velocity = v;
        velocity.y = 0;
    }

    void move() {
        float dec = 0.0000625;
        
        position += velocity;
        float len = distance(velocity);
        float ratio = ((len - dec) / len);
        velocity *= ratio;

        if(len - dec < 0) {
            velocity = {0, 0, 0};
        }

    }

} balls[balls_count]; // zero is the cue ball

float dot(glm::vec3 v1, glm::vec3 v2) {
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

void print(glm::vec3 v) {
    std::cout << v.x << " " << v.y << " " << v.z;
}

// Function to compute the intersection point
glm::vec3 projectPointOntoLine(const glm::vec3& point, const glm::vec3& lineOrigin, const glm::vec3& lineDirection) {
    // Vector from line origin to point
    glm::vec3 L = point - lineOrigin;

    // Compute t
    float t = dot(L, lineDirection) / dot(lineDirection, lineDirection);

    // Compute the projection point Q
    glm::vec3 Q = lineOrigin + lineDirection * t;

    return Q;
}

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

void drawLightSphere() {
    glPushMatrix();
    glTranslatef(0.0f, 1.4f, 0.0f);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.0f);
    glutSolidSphere(0.025, 20, 20);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// Function to draw the billiards table
void drawTable() {
    // lights
    drawRectangle(-1.12, 1.6, -0.2, 2.24, 0.5, 0.2, 0.4, 0.2, 0.0); // Top Rail

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
    float deltaX = (screen_width/2 - x) * camera.cameraSensitivity; // Calculate the change in mouse position
    float deltaY = (screen_height/2 - y) * camera.cameraSensitivity + 0.004;

    // Vector to rotate
    glm::vec3 vector(camera.look.x, camera.look.y, camera.look.z);
    rotatVector(vector, glm::vec3(camera.up.x, camera.up.y, camera.up.z), asin(deltaX / sqrt(deltaX * deltaX + 1 * 1)));
    rotatVector(vector, glm::vec3(glm::cross(glm::vec3(camera.up.x, camera.up.y, camera.up.z), vector)), -asin(deltaY / sqrt(deltaY * deltaY + 1 * 1)));


    camera.look.x = vector.x;
    camera.look.y = vector.y;
    camera.look.z = vector.z;
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
    float centerX = screenWidth / 2.0;
    float centerY = screenHeight / 2.0;

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

void setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = {3.0f, 3.0f, 3.0f, 3.0f};
    GLfloat ambientLight[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat diffuseLight[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat specularLight[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
}

// Function to draw the aim dot
void drawAimDot() {
    // Disable depth testing to ensure the dot is always visible
    glDisable(GL_DEPTH_TEST);

    // Switch to orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();             // Save the current projection matrix
    glLoadIdentity();           // Reset the projection matrix
    gluOrtho2D(0.0, 1.0, 0.0, 1.0); // Orthographic projection for screen-space

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();             // Save the current modelview matrix
    glLoadIdentity();           // Reset the modelview matrix

    // Draw the dot in the center of the screen
    glPointSize(5.0f);         // Set point size
    glColor3f(1.0f, 0.0f, 0.0f); // Red color for the aim dot
    glBegin(GL_POINTS);
    glVertex2f(0.5f, 0.5f);     // Center of the screen in normalized device coordinates
    glEnd();

    // Restore the previous matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW); // Switch back to modelview
    glEnable(GL_DEPTH_TEST);    // Re-enable depth testing
}

void swap(glm::vec3 &s1, glm::vec3 &s2) {
    glm::vec3 temp = s1;
    s1 = s2;
    s2 = temp;
}

void checkWallCollisions(Ball& ball) {
    if (ball.position.x - ball.radius < -table_width / 2 || ball.position.x + ball.radius > table_width / 2) {
        ball.velocity.x *= -1;
    }

    if (ball.position.z - ball.radius < -table_height / 2 || ball.position.z + ball.radius > table_height / 2) {
        ball.velocity.z *= -1;
    }
}

void checkBallCollisions(Ball& b1, Ball& b2) {
    glm::vec3 delta = b1.position - b2.position;
    float dist = glm::length(delta);

    if (dist < b1.radius + b2.radius) {
        glm::vec3 collisionNormal = glm::normalize(delta);
        glm::vec3 relativeVelocity = b1.velocity - b2.velocity;

        float velocityAlongNormal = glm::dot(relativeVelocity, collisionNormal);
        if (velocityAlongNormal > 0) return;

        float restitution = 1.0f;
        float impulse = (1 + restitution) * velocityAlongNormal;
        impulse /= 1 / b1.radius + 1 / b2.radius;

        glm::vec3 impulseVector = impulse * collisionNormal;
        b1.velocity -= impulseVector / b1.radius;
        b2.velocity += impulseVector / b2.radius;
    }
}

void checkPocketCollisions(Ball& ball) {
    if(distance(glm::vec2(ball.position.x, ball.position.z), glm::vec2(1.4, 0.7)) < (pocketRadius/2.0 + ball.radius)) {
        ball.active = false;
    }
    if(distance(glm::vec2(ball.position.x, ball.position.z), glm::vec2(-1.4, 0.7)) < (pocketRadius/2.0 + ball.radius)) {
        ball.active = false;
    }
    if(distance(glm::vec2(ball.position.x, ball.position.z), glm::vec2(1.4, -0.7)) < (pocketRadius/2.0 + ball.radius)) {
        ball.active = false;
    }
    if(distance(glm::vec2(ball.position.x, ball.position.z), glm::vec2(-1.4, -0.7)) < (pocketRadius/2.0 + ball.radius)) {
        ball.active = false;
    }

    if(distance(glm::vec2(ball.position.x, ball.position.z), glm::vec2(0.0, -0.7)) < (pocketRadius/2.0 + ball.radius)) {
        ball.active = false;
    }
    if(distance(glm::vec2(ball.position.x, ball.position.z), glm::vec2(0.0, 0.7)) < (pocketRadius/2.0 + ball.radius)) {
        ball.active = false;
    }

}

void update() {
    for(int i = 0;i < balls_count;i++) {
        if(!balls[i].active) continue;

        checkWallCollisions(balls[i]);
        checkPocketCollisions(balls[i]);
        for(int j = i + 1;j < balls_count;j++) {
            if(!balls[j].active) continue;

            checkBallCollisions(balls[i], balls[j]);
        }
    }
}

// Display callback function
void display() {
    update();
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the screen

    glLoadIdentity(); // Reset transformations
    
    // Set the camera position using gluLookAt (eye position, center, up direction)
    gluLookAt(camera.position.x, camera.position.y, camera.position.z, // Camera position (x, y, z)
              camera.position.x + camera.look.x, camera.position.y + camera.look.y, camera.position.z + camera.look.z,  // Look at a point 1 unit in front of the camera
              camera.up.x, camera.up.y, camera.up.z); // Up direction (y-axis)

    // drawLightSphere();
    
    // Render the platform
    renderPlatform();

    // Draw the table
    drawTable();

    for(int i = 0;i < balls_count;i++) {
        if(!balls[i].active) continue;

        balls[i].drawColoredSphere();
    }

    drawAimDot();

    glutSwapBuffers(); // Swap buffers to display the rendered scene
}

// Function to initialize OpenGL settings
void init() {
    glEnable(GL_DEPTH_TEST);  // Enable depth testing for proper 3D rendering
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color to black
    // setupLighting();

    balls[0] = Ball(0.05, 1.0, 1.0, 1.0, 2.0 + (-1.15), 0.55, 1.0 + (-1.0));

    balls[1] = Ball(0.05, 1.0, 0.0, 0.0, 1.0 + (-1.7), 0.55, 1.0 + (-1.0));
    balls[2] = Ball(0.05, 1.0, 0.0, 0.0, 1.0 + (-1.7), 0.55, 1.1 + (-1.0));
    balls[3] = Ball(0.05, 1.0, 0.0, 0.0, 1.0 + (-1.7), 0.55, 0.9 + (-1.0));
    balls[4] = Ball(0.05, 1.0, 0.0, 0.0, 1.0 + (-1.7), 0.55, 1.2 + (-1.0));
    balls[5] = Ball(0.05, 1.0, 0.0, 0.0, 1.0 + (-1.7), 0.55, 0.8 + (-1.0));

    balls[6] = Ball(0.05, 1.0, 0.0, 0.0, 1.1 + (-1.7) - (0.0134 * 1), 0.55, 1.05 + (-1.0));
    balls[7] = Ball(0.05, 1.0, 0.0, 0.0, 1.1 + (-1.7) - (0.0134 * 1), 0.55, 1.15 + (-1.0));
    balls[9] =  Ball(0.05, 1.0, 0.0, 0.0, 1.1 + (-1.7) - (0.0134 * 1), 0.55, 0.95 + (-1.0));
    balls[10] = Ball(0.05, 1.0, 0.0, 0.0, 1.1 + (-1.7) - (0.0134 * 1), 0.55, 0.85 + (-1.0));
    
    balls[8] =  Ball(0.05, 0.0, 0.0, 0.0, 1.2 + (-1.7) - (0.0134 * 2), 0.55, 1.0 + (-1.0));
    balls[11] = Ball(0.05, 1.0, 0.0, 0.0, 1.2 + (-1.7) - (0.0134 * 2), 0.55, 1.1 + (-1.0));
    balls[12] = Ball(0.05, 1.0, 0.0, 0.0, 1.2 + (-1.7) - (0.0134 * 2), 0.55, 0.9 + (-1.0));

    balls[13] = Ball(0.05, 1.0, 0.0, 0.0, 1.3 + (-1.7) - (0.0134 * 3), 0.55, 1.05 + (-1.0));
    balls[14] = Ball(0.05, 1.0, 0.0, 0.0, 1.3 + (-1.7) - (0.0134 * 3), 0.55, 0.95 + (-1.0));
    
    balls[15] = Ball(0.05, 1.0, 0.0, 0.0, 1.4 + (-1.7) - (0.0134 * 4), 0.55, 1.00 + (-1.0));
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

    bool noMovement = true;
    for(int i = 0;i < 16;i++) {
        if(distance(balls[i].velocity) > 1e-6) {
            noMovement = false;
        }

        balls[i].move();
    }

    float limit = 0.1, inc = 0.00025;
    if(noMovement && isMousePressed) {
        strength += inc;
        if(strength > limit) {
            strength = limit;
        }
    }
    else strength = 0;

}

void mouse(int button, int state, int x, int y) {
    // Detect left mouse button press/release
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            isMousePressed = true; // Mouse is pressed
        } else if (state == GLUT_UP) {
            isMousePressed = false; // Mouse is released
            
            float dist = distance(balls[0].position, projectPointOntoLine(balls[0].position, camera.position, camera.look));
            if(dist < balls[0].radius) {
                balls[0].setMovement(camera.getPropperVector() * strength);
            }
        }
    }
}

// Main function
int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(screen_width, screen_height);  // Set initial window size
    glutCreateWindow("8 Ball Pool");
    // glutFullScreen();

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
    glutMouseFunc(mouse);

    glutTimerFunc(0, timer, 0); // Set a timer to periodically reposition the cursor

    // Start the GLUT main loop
    glutMainLoop();

    return 0;
}


// g++ -I/usr/include/glm -I/usr/include/GL tester.cpp -o test -Lls /usr/lib/libglut.so -lGL -lGLU -lglut -lX11
// g++ main.cpp -o main -lGL -lGLU -lglut -lX11 && ./main




//Simple Paint Program

#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

// Canvas dimensions
const int INITIAL_WIDTH = 800;
const int INITIAL_HEIGHT = 600;
const int UI_HEIGHT = 80;  // Increased UI height for better display

int CANVAS_WIDTH = INITIAL_WIDTH;
int CANVAS_HEIGHT = INITIAL_HEIGHT;

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Drawing modes
enum DrawingMode {
    MODE_POINT,
    MODE_LINE,
    MODE_LINE_LOOP,
    MODE_TRIANGLE_WIRE,
    MODE_TRIANGLE_FILLED,
    MODE_RECTANGLE_WIRE,
    MODE_RECTANGLE_FILLED,
    MODE_CIRCLE
};

// Menu constants
#define POINT 1
#define LINE 2
#define LINE_LOOP 3
#define TRIANGLE_WIRE 4
#define TRIANGLE_FILLED 5
#define RECTANGLE_WIRE 6
#define RECTANGLE_FILLED 7
#define CIRCLE_MENU 8
#define CLEAR 9

// Colors
#define COLOR_BLACK 10 
#define COLOR_RED 11
#define COLOR_GREEN 12
#define COLOR_BLUE 13
#define COLOR_YELLOW 14
#define COLOR_MAGENTA 15
#define COLOR_CYAN 16
#define COLOR_GRAY 17

// Point sizes
#define POINT_SIZE_1 20
#define POINT_SIZE_3 21
#define POINT_SIZE_5 22
#define POINT_SIZE_7 23
#define POINT_SIZE_10 24
#define POINT_SIZE_15 25

// Line widths
#define LINE_WIDTH_1 30
#define LINE_WIDTH_2 31
#define LINE_WIDTH_3 32
#define LINE_WIDTH_5 33
#define LINE_WIDTH_7 34
#define LINE_WIDTH_10 35

#define EXIT 40

// Shape structure to store all drawn shapes
struct Shape {
    int type;
    vector<pair<float, float>> vertices;
    float color[3];
    float pointSize;
    float lineWidth;
    bool isFilled;

    Shape() : type(0), pointSize(5.0f), lineWidth(2.0f), isFilled(false) {
        color[0] = 0.0f; color[1] = 0.0f; color[2] = 0.0f; // Default black
    }
};

// Global variables
vector<Shape> shapes;
vector<Shape> undoStack;
vector<Shape> redoStack;
vector<pair<float, float>> currentVertices;
DrawingMode currentMode = MODE_POINT;
float currentColor[3] = { 0.0f, 0.0f, 0.0f }; // Black
float currentPointSize = 5.0f;
float currentLineWidth = 2.0f;
string currentStatus = "Drawing a Point";
bool needsRedraw = true;

// Auto-close detection variables
const float AUTO_CLOSE_DISTANCE = 15.0f; // pixels

// Function declarations
void drawCircle(float cx, float cy, float radius, int segments = 36);
void drawAllShapes();
void drawCurrentShape();
void completeShape();
void drawUI();
void processMenuEvents(int option);
void mouse(int btn, int state, int x, int y);
void createMenus();
void keyboard(unsigned char key, int x, int y);
void display();
void reshape(int w, int h);
void initialize();
void drawAutoCloseIndicator();

// Initialize OpenGL settings
void initialize() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // White background 
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

// Display callback function
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Set viewport for drawing area (below UI)
    glViewport(0, UI_HEIGHT, CANVAS_WIDTH, CANVAS_HEIGHT - UI_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, CANVAS_WIDTH, 0, CANVAS_HEIGHT - UI_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw all stored shapes and current shape
    drawAllShapes();
    drawCurrentShape();

    // Draw auto-close indicator if applicable
    drawAutoCloseIndicator();

    // Set viewport for UI area (top section)
    glViewport(0, 0, CANVAS_WIDTH, UI_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, CANVAS_WIDTH, 0, UI_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw user interface
    drawUI();

    glutSwapBuffers();
    needsRedraw = false;
}

// Reshape callback function - handles window resizing
void reshape(int w, int h)
{
    // Prevent divide by zero
    if (CANVAS_WIDTH == 0 || CANVAS_HEIGHT == 0) {
        CANVAS_WIDTH = w;
        CANVAS_HEIGHT = h;
        return;
    }

    float oldW = CANVAS_WIDTH;
    float oldH = CANVAS_HEIGHT;

    float scaleX = (float)w / oldW;
    float scaleY = (float)h / oldH;

    // Scale all previously drawn shapes
    for (auto& shape : shapes) {
        for (auto& v : shape.vertices) {
            v.first *= scaleX;
            v.second *= scaleY;
        }
    }

    // Scale in-progress drawing
    for (auto& v : currentVertices) {
        v.first *= scaleX;
        v.second *= scaleY;
    }

    CANVAS_WIDTH = w;
    CANVAS_HEIGHT = h;

    glutPostRedisplay();
}

void undo() {
    if (!shapes.empty()) {
        // Move last shape to redo stack
        redoStack.push_back(shapes.back());
        shapes.pop_back();
        needsRedraw = true;
        glutPostRedisplay();
    }
}

void redo() {
    if (!redoStack.empty()) {
        // Move last redo shape back to main shapes
        shapes.push_back(redoStack.back());
        redoStack.pop_back();
        needsRedraw = true;
        glutPostRedisplay();
    }
}

// Draw user interface with GLUT bitmap characters
void drawUI() {
    // UI background
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f((float)CANVAS_WIDTH, 0.0f);
    glVertex2f((float)CANVAS_WIDTH, (float)UI_HEIGHT);
    glVertex2f(0.0f, (float)UI_HEIGHT);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f); // White text

    // Current mode display
    glRasterPos2f(10.0f, (float)(UI_HEIGHT - 20));
    for (char c : currentStatus) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    // Instructions display
    string instructions = "Press 'C' to Clear | 'Q' to Exit | 'Z' to Undo | 'Y' to Redo | Right-click for Menu";
    if (currentMode == MODE_LINE_LOOP && currentVertices.size() >= 3) {
        instructions += " | Click near first point to Complete";
    }
    glRasterPos2f(10.0f, (float)(UI_HEIGHT - 40));
    for (char c : instructions) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    // Current settings display
    stringstream settings;
    settings << "Point Size: " << (int)currentPointSize << " | Line Width: " << (int)currentLineWidth;
    string settingsStr = settings.str();
    glRasterPos2f(10.0f, (float)(UI_HEIGHT - 60));
    for (char c : settingsStr) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    // Color indicator
    glColor3f(currentColor[0], currentColor[1], currentColor[2]);
    glBegin(GL_QUADS);
    glVertex2f((float)(CANVAS_WIDTH - 100), 10.0f);
    glVertex2f((float)(CANVAS_WIDTH - 60), 10.0f);
    glVertex2f((float)(CANVAS_WIDTH - 60), 30.0f);
    glVertex2f((float)(CANVAS_WIDTH - 100), 30.0f);
    glEnd();

    // Color label
    glColor3f(1.0f, 1.0f, 1.0f);
    string colorLabel = "Current Color:";
    glRasterPos2f((float)(CANVAS_WIDTH - 200), 20.0f);
    for (char c : colorLabel) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }
}

// Draw circle using line loop (wireframe only as required)
void drawCircle(float cx, float cy, float radius, int segments) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; i++) {
        float theta = 2.0f * (float)M_PI * (float)i / (float)segments;
        float x = radius * cosf(theta);
        float y = radius * sinf(theta);
        glVertex2f(x + cx, y + cy);
    }
    glEnd();
}

// Draw all stored shapes
void drawAllShapes() {
    for (const auto& shape : shapes) {
        glColor3f(shape.color[0], shape.color[1], shape.color[2]);

        if (shape.type == CIRCLE_MENU) {
            // Circle (wireframe only as required)
            glLineWidth(shape.lineWidth);
            float cx = shape.vertices[0].first;
            float cy = shape.vertices[0].second;
            float radius = shape.vertices[1].first; // Radius stored in first vertex
            drawCircle(cx, cy, radius);
        }
        else if (shape.type == GL_POINTS) {
            // Points
            glPointSize(shape.pointSize);
            glBegin(GL_POINTS);
            for (const auto& vertex : shape.vertices) {
                glVertex2f(vertex.first, vertex.second);
            }
            glEnd();
        }
        else {
            // Lines, triangles, rectangles
            if (shape.type == GL_LINES || shape.type == GL_LINE_LOOP) {
                glLineWidth(shape.lineWidth);
            }
            glBegin(shape.type);
            for (const auto& vertex : shape.vertices) {
                glVertex2f(vertex.first, vertex.second);
            }
            glEnd();
        }
    }
}

// Draw auto-close indicator (highlight first point when close)
void drawAutoCloseIndicator() {
    if (currentMode == MODE_LINE_LOOP && currentVertices.size() >= 3) {
        // Draw a highlighted circle around the first point
        glColor3f(1.0f, 0.0f, 0.0f); // Red indicator
        glLineWidth(2.0f);

        // Draw circle around first point to indicate auto-close zone
        float firstX = currentVertices[0].first;
        float firstY = currentVertices[0].second;

        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 20; i++) {
            float theta = 2.0f * (float)M_PI * (float)i / 20.0f;
            float x = AUTO_CLOSE_DISTANCE * cosf(theta);
            float y = AUTO_CLOSE_DISTANCE * sinf(theta);
            glVertex2f(x + firstX, y + firstY);
        }
        glEnd();

        // Draw a crosshair at the first point
        glBegin(GL_LINES);
        glVertex2f(firstX - 8.0f, firstY);
        glVertex2f(firstX + 8.0f, firstY);
        glVertex2f(firstX, firstY - 8.0f);
        glVertex2f(firstX, firstY + 8.0f);
        glEnd();
    }
}

// Draw current incomplete shape (preview)
void drawCurrentShape() {
    if (!currentVertices.empty()) {
        glColor3f(currentColor[0], currentColor[1], currentColor[2]);

        // Draw points for placed vertices
        glPointSize(currentPointSize);
        glBegin(GL_POINTS);
        for (const auto& vertex : currentVertices) {
            glVertex2f(vertex.first, vertex.second);
        }
        glEnd();

        // Draw preview based on current mode
        switch (currentMode) {
        case MODE_LINE:
            if (currentVertices.size() == 2) {
                glLineWidth(currentLineWidth);
                glBegin(GL_LINES);
                glVertex2f(currentVertices[0].first, currentVertices[0].second);
                glVertex2f(currentVertices[1].first, currentVertices[1].second);
                glEnd();
            }
            break;

        case MODE_CIRCLE:
            // Improved circle preview with better visualization
            if (currentVertices.size() == 1) {
                // Show only the center point
                glPointSize(currentPointSize);
                glBegin(GL_POINTS);
                glVertex2f(currentVertices[0].first, currentVertices[0].second);
                glEnd();
            }
            else if (currentVertices.size() == 2) {
                // Show circle preview with center and radius
                float cx = currentVertices[0].first;
                float cy = currentVertices[0].second;
                float dx = currentVertices[1].first - cx;
                float dy = currentVertices[1].second - cy;
                float radius = sqrtf(dx * dx + dy * dy);

                // Draw the circle
                glLineWidth(currentLineWidth);
                drawCircle(cx, cy, radius);

                // Draw radius line for better visualization
                glBegin(GL_LINES);
                glVertex2f(cx, cy);
                glVertex2f(currentVertices[1].first, currentVertices[1].second);
                glEnd();
            }
            break;

        case MODE_TRIANGLE_WIRE:
            if (currentVertices.size() >= 2) {
                glLineWidth(currentLineWidth);
                glBegin(GL_LINE_STRIP);
                for (const auto& vertex : currentVertices) {
                    glVertex2f(vertex.first, vertex.second);
                }
                if (currentVertices.size() == 3) {
                    glVertex2f(currentVertices[0].first, currentVertices[0].second);
                }
                glEnd();
            }
            break;

        case MODE_RECTANGLE_WIRE:
            if (currentVertices.size() >= 2) {
                glLineWidth(currentLineWidth);
                glBegin(GL_LINE_STRIP);
                for (const auto& vertex : currentVertices) {
                    glVertex2f(vertex.first, vertex.second);
                }
                if (currentVertices.size() == 4) {
                    glVertex2f(currentVertices[0].first, currentVertices[0].second);
                }
                glEnd();
            }
            break;

        case MODE_LINE_LOOP:
            if (currentVertices.size() >= 2) {
                glLineWidth(currentLineWidth);
                glBegin(GL_LINE_STRIP);
                for (const auto& vertex : currentVertices) {
                    glVertex2f(vertex.first, vertex.second);
                }
                glEnd();

                // Draw a dotted line from last point to first point for better visualization
                if (currentVertices.size() >= 3) {
                    glEnable(GL_LINE_STIPPLE);
                    glLineStipple(1, 0x00FF); // Dotted pattern
                    glBegin(GL_LINES);
                    glVertex2f(currentVertices.back().first, currentVertices.back().second);
                    glVertex2f(currentVertices[0].first, currentVertices[0].second);
                    glEnd();
                    glDisable(GL_LINE_STIPPLE);
                }
            }
            break;
        }
    }
}

// Complete the current shape and add to shapes list
void completeShape() {
    if (currentVertices.empty()) return;

    Shape newShape;
    newShape.color[0] = currentColor[0];
    newShape.color[1] = currentColor[1];
    newShape.color[2] = currentColor[2];
    newShape.pointSize = currentPointSize;
    newShape.lineWidth = currentLineWidth;
    newShape.vertices = currentVertices;

    // Set appropriate OpenGL primitive type
    switch (currentMode) {
    case MODE_POINT:
        newShape.type = GL_POINTS;
        newShape.isFilled = true;
        break;
    case MODE_LINE:
        newShape.type = GL_LINES;
        newShape.isFilled = false;
        break;
    case MODE_LINE_LOOP:
        newShape.type = GL_LINE_LOOP;
        newShape.isFilled = false;
        currentStatus = "Line Loop Completed";
        break;
    case MODE_TRIANGLE_WIRE:
        newShape.type = GL_LINE_LOOP;
        newShape.isFilled = false;
        break;
    case MODE_TRIANGLE_FILLED:
        newShape.type = GL_TRIANGLES;
        newShape.isFilled = true;
        break;
    case MODE_RECTANGLE_WIRE:
    case MODE_RECTANGLE_FILLED:
        if (currentVertices.size() == 2) {

            // First two clicks
            float x1 = currentVertices[0].first;
            float y1 = currentVertices[0].second;
            float x2 = currentVertices[1].first;
            float y2 = currentVertices[1].second;

            // Auto-generated corners
            pair<float, float> p1 = make_pair(x1, y1);
            pair<float, float> p2 = make_pair(x1, y2);
            pair<float, float> p3 = make_pair(x2, y2);
            pair<float, float> p4 = make_pair(x2, y1);

            // Clear and replace with generated rectangle
            newShape.vertices.clear();
            newShape.vertices.push_back(p1);
            newShape.vertices.push_back(p2);
            newShape.vertices.push_back(p3);
            newShape.vertices.push_back(p4);

            // Set type
            newShape.type = (currentMode == MODE_RECTANGLE_FILLED) ? GL_QUADS : GL_LINE_LOOP;
            newShape.isFilled = (currentMode == MODE_RECTANGLE_FILLED);

            currentStatus = "Rectangle Completed";
        }
        else {
            currentStatus = "Rectangle requires 2 points.";
            currentVertices.clear();
            needsRedraw = true;
            return;
        }
        break;
    case MODE_CIRCLE:
        if (currentVertices.size() == 2) {
            // Store circle data: first point is center, second point's x stores radius
            float cx = currentVertices[0].first;
            float cy = currentVertices[0].second;
            float dx = currentVertices[1].first - cx;
            float dy = currentVertices[1].second - cy;
            float radius = sqrtf(dx * dx + dy * dy);

            newShape.type = CIRCLE_MENU; // Custom type for circle
            newShape.vertices.clear();
            newShape.vertices.push_back(make_pair(cx, cy)); // Center
            newShape.vertices.push_back(make_pair(radius, 0.0f)); // Radius stored in first vertex
            newShape.isFilled = false;
            currentStatus = "Circle Completed";
        }
        else {
            // Not enough points for circle
            currentVertices.clear();
            currentStatus = "Circle requires 2 points (center + radius)";
            needsRedraw = true;
            return;
        }
        break;
    }

    shapes.push_back(newShape);
    // --- Undo support ---
    undoStack.push_back(newShape);
    redoStack.clear();

    currentVertices.clear();
    needsRedraw = true;
}

// Mouse callback function with auto-close detection
void mouse(int btn, int state, int x, int y) {
    // Convert coordinates to drawing area coordinate system
    float canvasX = (float)x;
    float canvasY = (float)(CANVAS_HEIGHT - y - UI_HEIGHT);

    // Check if click is in drawing area (not in UI area)
    if (canvasY < 0) return;

    if (btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // Check for auto-close when in Line Loop mode with enough points
        if (currentMode == MODE_LINE_LOOP && currentVertices.size() >= 3) {
            if (!currentVertices.empty()) {
                float firstX = currentVertices[0].first;
                float firstY = currentVertices[0].second;

                // Calculate distance from current click to first point
                float distance = sqrt(pow(canvasX - firstX, 2) + pow(canvasY - firstY, 2));

                // Auto-close if within threshold distance
                if (distance <= AUTO_CLOSE_DISTANCE) {
                    completeShape();
                    return; // Don't add another point
                }
            }
        }

        // Normal left-click behavior - add new point
        currentVertices.push_back(make_pair(canvasX, canvasY));

        bool shapeComplete = false;

        // Check if shape is complete based on current mode
        switch (currentMode) {
        case MODE_POINT:
            shapeComplete = true;  // 1 click for points
            break;
        case MODE_LINE:
            shapeComplete = (currentVertices.size() == 2);  // 2 clicks for lines
            break;
        case MODE_TRIANGLE_WIRE:
        case MODE_TRIANGLE_FILLED:
            shapeComplete = (currentVertices.size() == 3);  // 3 clicks for triangles
            break;
        case MODE_RECTANGLE_WIRE:
        case MODE_RECTANGLE_FILLED:
            shapeComplete = (currentVertices.size() == 2);  // 2 clicks for rectangles
            break;
        case MODE_CIRCLE:
            shapeComplete = (currentVertices.size() == 2);  // 2 clicks for circles
            break;
        case MODE_LINE_LOOP:
            // Line loop doesn't auto-complete by click count, uses auto-close or manual completion
            shapeComplete = false;
            break;
        }

        if (shapeComplete) {
            completeShape();
        }

        needsRedraw = true;
        glutPostRedisplay();
    }

    // Right click to complete line loop (alternative method)
    if (btn == GLUT_RIGHT_BUTTON && state == GLUT_DOWN &&
        currentMode == MODE_LINE_LOOP && currentVertices.size() >= 3) {
        completeShape();
    }
}

// Menu callback function
void processMenuEvents(int option) {
    string modeText = "";

    switch (option) {
        // Drawing modes
    case POINT:
        currentMode = MODE_POINT;
        modeText = "Drawing a Point";
        break;
    case LINE:
        currentMode = MODE_LINE;
        modeText = "Drawing a Line";
        break;
    case LINE_LOOP:
        currentMode = MODE_LINE_LOOP;
        modeText = "Drawing a Line Loop (Click near first point to complete)";
        break;
    case TRIANGLE_WIRE:
        currentMode = MODE_TRIANGLE_WIRE;
        modeText = "Drawing Triangle (Wireframe)";
        break;
    case TRIANGLE_FILLED:
        currentMode = MODE_TRIANGLE_FILLED;
        modeText = "Drawing Triangle (Filled)";
        break;
    case RECTANGLE_WIRE:
        currentMode = MODE_RECTANGLE_WIRE;
        modeText = "Drawing Rectangle (Wireframe)";
        break;
    case RECTANGLE_FILLED:
        currentMode = MODE_RECTANGLE_FILLED;
        modeText = "Drawing Rectangle (Filled)";
        break;
    case CIRCLE_MENU:
        currentMode = MODE_CIRCLE;
        modeText = "Drawing a Circle (Click center then radius point)";
        break;

    case CLEAR:
        shapes.clear();
        currentVertices.clear();
        modeText = "Canvas Cleared";
        break;

        // Colors (8 different colors as required)
    case COLOR_BLACK:  // Changed from COLOR_WHITE to COLOR_BLACK
        currentColor[0] = 0.0f; currentColor[1] = 0.0f; currentColor[2] = 0.0f;
        break;
    case COLOR_RED:
        currentColor[0] = 1.0f; currentColor[1] = 0.0f; currentColor[2] = 0.0f;
        break;
    case COLOR_GREEN:
        currentColor[0] = 0.0f; currentColor[1] = 1.0f; currentColor[2] = 0.0f;
        break;
    case COLOR_BLUE:
        currentColor[0] = 0.0f; currentColor[1] = 0.0f; currentColor[2] = 1.0f;
        break;
    case COLOR_YELLOW:
        currentColor[0] = 1.0f; currentColor[1] = 1.0f; currentColor[2] = 0.0f;
        break;
    case COLOR_MAGENTA:
        currentColor[0] = 1.0f; currentColor[1] = 0.0f; currentColor[2] = 1.0f;
        break;
    case COLOR_CYAN:
        currentColor[0] = 0.0f; currentColor[1] = 1.0f; currentColor[2] = 1.0f;
        break;
    case COLOR_GRAY:
        currentColor[0] = 0.5f; currentColor[1] = 0.5f; currentColor[2] = 0.5f;
        break;

        // Point sizes (6 sizes as required)
    case POINT_SIZE_1:
        currentPointSize = 1.0f;
        break;
    case POINT_SIZE_3:
        currentPointSize = 3.0f;
        break;
    case POINT_SIZE_5:
        currentPointSize = 5.0f;
        break;
    case POINT_SIZE_7:
        currentPointSize = 7.0f;
        break;
    case POINT_SIZE_10:
        currentPointSize = 10.0f;
        break;
    case POINT_SIZE_15:
        currentPointSize = 15.0f;
        break;

        // Line widths (6 widths as required)
    case LINE_WIDTH_1:
        currentLineWidth = 1.0f;
        break;
    case LINE_WIDTH_2:
        currentLineWidth = 2.0f;
        break;
    case LINE_WIDTH_3:
        currentLineWidth = 3.0f;
        break;
    case LINE_WIDTH_5:
        currentLineWidth = 5.0f;
        break;
    case LINE_WIDTH_7:
        currentLineWidth = 7.0f;
        break;
    case LINE_WIDTH_10:
        currentLineWidth = 10.0f;
        break;

    case EXIT:
        exit(0);
        break;
    }

    if (option != CLEAR && option != EXIT) {
        currentStatus = modeText;
    }

    currentVertices.clear();
    needsRedraw = true;
    glutPostRedisplay();
}

// Create GLUT menus and submenus
void createMenus() {
    // Color submenu (8 colors as required)
    int colorMenu = glutCreateMenu(processMenuEvents);
    glutAddMenuEntry("Black", COLOR_BLACK);  // Changed from "White" to "Black"
    glutAddMenuEntry("Red", COLOR_RED);
    glutAddMenuEntry("Green", COLOR_GREEN);
    glutAddMenuEntry("Blue", COLOR_BLUE);
    glutAddMenuEntry("Yellow", COLOR_YELLOW);
    glutAddMenuEntry("Magenta", COLOR_MAGENTA);
    glutAddMenuEntry("Cyan", COLOR_CYAN);
    glutAddMenuEntry("Gray", COLOR_GRAY);

    // Point size submenu (6 sizes as required)
    int pointSizeMenu = glutCreateMenu(processMenuEvents);
    glutAddMenuEntry("1px", POINT_SIZE_1);
    glutAddMenuEntry("3px", POINT_SIZE_3);
    glutAddMenuEntry("5px", POINT_SIZE_5);
    glutAddMenuEntry("7px", POINT_SIZE_7);
    glutAddMenuEntry("10px", POINT_SIZE_10);
    glutAddMenuEntry("15px", POINT_SIZE_15);

    // Line width submenu (6 widths as required)
    int lineWidthMenu = glutCreateMenu(processMenuEvents);
    glutAddMenuEntry("1px", LINE_WIDTH_1);
    glutAddMenuEntry("2px", LINE_WIDTH_2);
    glutAddMenuEntry("3px", LINE_WIDTH_3);
    glutAddMenuEntry("5px", LINE_WIDTH_5);
    glutAddMenuEntry("7px", LINE_WIDTH_7);
    glutAddMenuEntry("10px", LINE_WIDTH_10);

    // Drawing mode submenu
    int drawMenu = glutCreateMenu(processMenuEvents);
    glutAddMenuEntry("Point", POINT);
    glutAddMenuEntry("Line", LINE);
    glutAddMenuEntry("Line Loop", LINE_LOOP);
    glutAddMenuEntry("Triangle (Wireframe)", TRIANGLE_WIRE);
    glutAddMenuEntry("Triangle (Filled)", TRIANGLE_FILLED);
    glutAddMenuEntry("Rectangle (Wireframe)", RECTANGLE_WIRE);
    glutAddMenuEntry("Rectangle (Filled)", RECTANGLE_FILLED);
    glutAddMenuEntry("Circle", CIRCLE_MENU);

    // Main menu
    int mainMenu = glutCreateMenu(processMenuEvents);
    glutAddSubMenu("Drawing Mode", drawMenu);
    glutAddSubMenu("Color", colorMenu);
    glutAddSubMenu("Point Size", pointSizeMenu);
    glutAddSubMenu("Line Width", lineWidthMenu);
    glutAddMenuEntry("Clear Screen", CLEAR);
    glutAddMenuEntry("Exit", EXIT);

    // Attach menu to right button as required
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// Keyboard callback function
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'c':
    case 'C':
        shapes.clear();
        currentVertices.clear();
        currentStatus = "Canvas Cleared";
        needsRedraw = true;
        glutPostRedisplay();
        break;

    case 'q':
    case 'Q':
    case 27: // ESC key
        exit(0);
        break;

    case ' ':  // Space bar to complete Line Loop
    case 13:   // Enter key to complete Line Loop
        if (currentMode == MODE_LINE_LOOP && currentVertices.size() >= 3) {
            completeShape();
        }
        break;
    case 'Z': // z for undo 
    case 'z':
        undo();
        break;
    case 'Y': // y for re-do
    case 'y':
        redo();
        break;
    }

}

// Main function
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(INITIAL_WIDTH, INITIAL_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("2D Drawing Program");

    initialize();
    createMenus();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);

    cout << "2D Drawing Program Instructions:" << endl;
    cout << "- Left-click to place vertices" << endl;
    cout << "- Right-click for menu options" << endl;
    cout << "- Press 'C' to clear screen" << endl;
    cout << "- Press 'Q' or ESC to exit" << endl;
    cout << "- Press 'Z' to undo, 'Y' to redo" << endl;  // Added undo/redo instructions
    cout << endl;
    cout << "Drawing Modes:" << endl;
    cout << "- Point: 1 click" << endl;
    cout << "- Line: 2 clicks" << endl;
    cout << "- Line Loop: 3+ clicks (click near first point to complete)" << endl;
    cout << "- Triangle: 3 clicks" << endl;
    cout << "- Rectangle: 2 clicks (two opposite corners)" << endl;
    cout << "- Circle: 2 clicks (center + radius)" << endl;

    glutMainLoop();
    return 0;
}
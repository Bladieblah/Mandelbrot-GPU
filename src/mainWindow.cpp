#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <stack>

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#include "coordinates.hpp"
#include "mainWindow.hpp"
#include "opencl.hpp"

int windowIdMain;
uint32_t *pixelsMain;

WindowSettings settingsMain;
MouseState mouseMain;
ViewSettings viewMain, defaultView;
std::stack<ViewSettings> viewStackMain;
bool selecting = true;

void drawGrid() {
    glBegin(GL_LINES);
        glVertex2f(-1,0); glVertex2f(1,0);
        glVertex2f(-1,0.5); glVertex2f(1,0.5);
        glVertex2f(-1,-0.5); glVertex2f(1,-0.5);
        glVertex2f(0,-1); glVertex2f(0,1);
        glVertex2f(0.5,-1); glVertex2f(0.5,1);
        glVertex2f(-0.5,-1); glVertex2f(-0.5,1);
    glEnd();
}

void drawBox() {
    float aspectRatio = (float)settingsMain.windowW / (float)settingsMain.windowH;
    float xc = 2 * mouseMain.xDown / (float)settingsMain.windowW - 1;
    float yc = 1 - 2 * mouseMain.yDown / (float)settingsMain.windowH;

    float dx1 = 2 * (mouseMain.x - mouseMain.xDown) / (float)settingsMain.windowW;
    float dy1 = -2 * (mouseMain.y - mouseMain.yDown) / (float)settingsMain.windowH;

    float dx2 = 2 * (mouseMain.y - mouseMain.yDown) * aspectRatio / settingsMain.windowW;
    float dy2 = 2 * (mouseMain.x - mouseMain.xDown) * aspectRatio / settingsMain.windowH;

    glColor4f(1,1,1,1);

    glBegin(GL_LINE_STRIP);
        glVertex2f(xc + dx1 + dx2, yc + dy1 + dy2);
        glVertex2f(xc + dx1 - dx2, yc + dy1 - dy2);
        glVertex2f(xc - dx1 - dx2, yc - dy1 - dy2);
        glVertex2f(xc - dx1 + dx2, yc - dy1 + dy2);
        glVertex2f(xc + dx1 + dx2, yc + dy1 + dy2);
    glEnd();
}

void displayMain() {
    glutSetWindow(windowIdMain);

    glClearColor( 0, 0, 0, 1 );
    glColor3f(1, 1, 1);
    glClear( GL_COLOR_BUFFER_BIT );

    glEnable (GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D (
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        settingsMain.width,
        settingsMain.height,
        0,
        GL_RGB,
        GL_UNSIGNED_INT,
        &(pixelsMain[0])
    );

    glPushMatrix();
    glScalef(settingsMain.zoom, settingsMain.zoom, 1.);
    glTranslatef(-settingsMain.centerX, -settingsMain.centerY, 0.);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0, -1.0);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0, -1.0);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0,  1.0);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0,  1.0);
    glEnd();

    glDisable (GL_TEXTURE_2D);

    glPointSize(2);
    glColor3f(1, 1, 1);
    glEnable(GL_POINT_SMOOTH);
    
    glBegin(GL_POINTS);

    glEnd();

    glPopMatrix();

    if (settingsMain.grid) {
        drawGrid();
    }

    if (mouseMain.state == GLUT_DOWN && selecting) {
        drawBox();
    }

    glFlush();
    glutSwapBuffers();
}

void updateView() {
    fprintf(stderr, "\n\n\n\n\n\nSetting region to:\n");
    fprintf(stderr, "scale = %.3f\n", viewMain.scaleY);
    fprintf(stderr, "center = (%.3f, %.3f)\n", viewMain.centerX, viewMain.centerY);
    fprintf(stderr, "theta = %.3f\n", viewMain.theta);

    viewMain.scaleX = viewMain.scaleY / config->height * config->width;
    viewMain.cosTheta = cos(viewMain.theta);
    viewMain.sinTheta = sin(viewMain.theta);

    opencl->setKernelArg("initParticles", 1, sizeof(ViewSettings), &(viewMain));
    opencl->step("initParticles");
}

void selectRegion() {
    if (mouseMain.state != GLUT_DOWN) {
        return;
    }

    ScreenCoordinate downP({mouseMain.xDown, mouseMain.yDown});
    ScreenCoordinate upP({mouseMain.x, mouseMain.y});

    WorldCoordinate downF = downP.toPixel(settingsMain).toWorld(viewMain);
    WorldCoordinate upF   = upP.toPixel(settingsMain).toWorld(viewMain);

    viewStackMain.push(ViewSettings(viewMain));

    viewMain.scaleY = sqrt(pow(upF.x - downF.x, 2) + pow(upF.y - downF.y, 2));
    viewMain.centerX = downF.x;
    viewMain.centerY = downF.y;
    viewMain.theta = atan2(upF.x - downF.x, upF.y - downF.y);

    updateView();
}

void keyPressedMain(unsigned char key, int x, int y) {
    switch (key) {
        case 'a':
            selectRegion();
            break;
        case 'e':
            glutSetWindow(windowIdMain);
            glutPostRedisplay();
            break;
        
        case 'g':
            settingsMain.grid = ! settingsMain.grid;
            break;
        case 'b':
            selecting = ! selecting;
            break;
        case 'r':
            settingsMain.zoom = 1.;
            settingsMain.centerX = 0.;
            settingsMain.centerY = 0.;
            break;
        case 'z':
            if (!viewStackMain.empty()) {
                viewMain = viewStackMain.top();
                viewStackMain.pop();
                opencl->setKernelArg("initParticles", 1, sizeof(ViewSettings), &(viewMain));
                opencl->step("initParticles");
            }
            break;

        case 'w':
            settingsMain.zoom *= 1.5;
            break;
        case 's':
            settingsMain.zoom /= 1.5;
            break;

        case 'q':
            exit(0);
            break;
        default:
            break;
    }
}

void specialKeyPressedMain(int key, int x, int y) {
    viewStackMain.push(ViewSettings(viewMain));

    switch (key) {
        case GLUT_KEY_RIGHT:
            viewMain.centerX += 0.1 * viewMain.scaleY;
            break;
        case GLUT_KEY_LEFT:
            viewMain.centerX -= 0.1 * viewMain.scaleY;
            break;
        case GLUT_KEY_UP:
            viewMain.centerY += 0.1 * viewMain.scaleY;
            break;
        case GLUT_KEY_DOWN:
            viewMain.centerY -= 0.1 * viewMain.scaleY;
            break;
        default:
            break;
    }
    updateView();
}

void translateCamera(ScreenCoordinate coords) {
    settingsMain.centerX += 2. / settingsMain.zoom * (coords.x / (float)settingsMain.windowW - 0.5);
    settingsMain.centerY += 2. / settingsMain.zoom * (0.5 - coords.y / (float)settingsMain.windowH);
}

void mousePressedMain(int button, int state, int x, int y) {
    if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) {
            translateCamera((ScreenCoordinate){x, y});
        }

        return;
    }

    mouseMain.state = state;

    if (state == GLUT_DOWN) {
        mouseMain.xDown = x;
        mouseMain.yDown = y;
    }
}

void mouseMovedMain(int x, int y) {
    mouseMain.x = x;
    mouseMain.y = y;
}

void onReshapeMain(int w, int h) {
    settingsMain.windowW = w;
    settingsMain.windowH = h;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    glMatrixMode(GL_MODELVIEW);
}

void createMainWindow(char *name, uint32_t width, uint32_t height) {
    settingsMain.width = width;
    settingsMain.height = height;

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(width, height);
    windowIdMain = glutCreateWindow(name);

    pixelsMain = (uint32_t *)malloc(3 * width * height * sizeof(uint32_t));

    for (int i = 0; i < 3 * width * height; i++) {
        pixelsMain[i] = 0;
    }
    
    glutKeyboardFunc(&keyPressedMain);
    glutSpecialFunc(&specialKeyPressedMain);
    glutMouseFunc(&mousePressedMain);
    glutMotionFunc(&mouseMovedMain);
    glutReshapeFunc(&onReshapeMain);
}

void destroyMainWindow() {
    free(pixelsMain);
}

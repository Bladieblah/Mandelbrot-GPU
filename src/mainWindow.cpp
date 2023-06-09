#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <stack>

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glut.h"
#include "../imgui/backends/imgui_impl_opengl2.h"

#include "colourMap.hpp"
#include "config.hpp"
#include "coordinates.hpp"
#include "mainWindow.hpp"
#include "opencl.hpp"

int windowIdMain;
uint32_t *pixelsMain;

size_t test_steps = 100000;
WorldCoordinate *zArray, *dzxArray, *dzyArray;
size_t escape_time = 0;
size_t escape_dx = 0;
size_t escape_dy = 0;

WindowSettings settingsMain;
MouseState mouseMain;
ViewSettings viewMain, defaultView;
std::stack<ViewSettings> viewStackMain;
bool selecting = true;
bool drawGradient = true;
bool drawDerivatives = true;
unsigned int count0 = 0;
char colourmap_filename[60];

#ifdef MANDEL_GPU_USE_DOUBLE
ViewSettingsCL viewMainCL;
#endif

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

void fillTestArray() {
    int i;
    ScreenCoordinate screen({mouseMain.x, mouseMain.y});
    WorldCoordinate fractal = screen.toPixel(settingsMain).toWorld(viewMain);
    WorldCoordinate dzx({1, 0});
    WorldCoordinate dzy({0, 1});
    WorldCoordinate offset(fractal);

    zArray[0] = fractal;
    dzxArray[0] = dzx;
    dzyArray[0] = dzy;

    escape_dx = 0;
    escape_dy = 0;

    for (i = 1; i < test_steps; i++) {
        dzx = 2 * complex_mul(fractal, dzx) + WorldCoordinate({1, 0});
        dzy = 2 * complex_mul(fractal, dzy) + WorldCoordinate({0, 1});
        fractal = complex_square(fractal) + offset;

        zArray[i] = fractal;
        dzxArray[i] = dzx;
        dzyArray[i] = dzy;

        if ((dzx.x * dzx.x + dzx.y * dzx.y) > 10 && escape_dx == 0) {
            escape_dx = i;
        }
        if ((dzy.x * dzy.x + dzy.y * dzy.y) > 10 && escape_dy == 0) {
            escape_dy = i;
        }

        if ((fractal.x * fractal.x + fractal.y * fractal.y) > 16) {
            break;
        }
    }
    
    escape_time = i + 1;
}

void drawPath() {
    glPointSize(2);
    glEnable(GL_POINT_SMOOTH);
    
    glBegin(GL_POINTS);
    glColor3f(1, 0, 1);

    for (int i = 0; i < escape_time; i++) {
        glVertex2f(
            2 * zArray[i].toPixel(defaultView).x / (float)viewMain.sizeX - 1,
            2 * zArray[i].toPixel(defaultView).y / (float)viewMain.sizeY - 1
        );
    }

    if (drawDerivatives) {
        for (int i = 0; i < escape_time; i++) {
            glColor3f(1, 0, 0);
            glVertex2f(
                2 * (zArray[i] + 0.1 * dzxArray[i]).toPixel(defaultView).x / (float)viewMain.sizeX - 1,
                2 * (zArray[i] + 0.1 * dzxArray[i]).toPixel(defaultView).y / (float)viewMain.sizeY - 1
            );

            glColor3f(0, 1, 1);
            glVertex2f(
                2 * (zArray[i] + 0.1 * dzyArray[i]).toPixel(defaultView).x / (float)viewMain.sizeX - 1,
                2 * (zArray[i] + 0.1 * dzyArray[i]).toPixel(defaultView).y / (float)viewMain.sizeY - 1
            );
        }
    }

    glEnd();
}

void showInfo() {
    ImGui::SeparatorText("Info");
    ImGui::Text("x = %.16f", viewMain.centerX);
    ImGui::Text("y = %.16f", viewMain.centerY);
    ImGui::Text("scale = %.3g", viewMain.scaleY); ImGui::SameLine();
    ImGui::Text("theta = %.3f", viewMain.theta);
    ImGui::Text("Frametime = %.3f", frameTime);
    ImGui::Text("Iterations = %d", iterCount);

    ScreenCoordinate screen({mouseMain.x, mouseMain.y});
    WorldCoordinate fractal = screen.toPixel(settingsMain).toWorld(viewMain);

    ImGui::SeparatorText("Cursor");
    ImGui::Text("x = %.16f", fractal.x);
    ImGui::Text("y = %.16f", fractal.y);
    
    ImGui::Text("Escape Time = %zu", escape_time);
    ImGui::Text("dx = %zu", escape_dx); ImGui::SameLine();
    ImGui::Text("dy = %zu", escape_dy);


    ImGui::Checkbox("Draw Box", &selecting);
    ImGui::Checkbox("Draw Derivatives", &drawDerivatives);
}

void showColourmapControls() {
    bool changed = false;
    char label[100];
    for (size_t i = 0; i < cm->getColorCount(); i++) {
        sprintf(label, "Color %zu", i);
        if (ImGui::TreeNode(label)) {
            sprintf(label, "##picker %zu", i);
            changed |= ImGui::ColorPicker3(label, cm->m_y[i].data(), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel);
            sprintf(label, "##slider %zu", i);
            changed |= ImGui::SliderFloat(label, &(cm->m_x.data()[i]), 0., 1.);
            ImGui::TreePop();
        }
    }

    ImGui::InputText("##Filename", colourmap_filename, 60); ImGui::SameLine();
    if (ImGui::Button("Save")) {
        char fn2[80];
        sprintf(fn2, "colourmaps/%s", colourmap_filename);
        cm->save(fn2);
    }

    if (changed) {
        cm->generate();
        cm->apply(cmap);
        opencl->writeBuffer("colourMap", cmap);
    }
}

void displayMain() {
    // --------------------------- RESET ---------------------------
    glutSetWindow(windowIdMain);

    ImGuiIO& io = ImGui::GetIO();

    glClearColor(0, 0, 0, 1);
    glColor3f(1, 1, 1);
    glClear( GL_COLOR_BUFFER_BIT );

    glEnable (GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    // --------------------------- FRACTAL ---------------------------

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

    if (!io.WantCaptureMouse && mouseMain.state == GLUT_DOWN && !selecting) {
        drawPath();
    }

    glPopMatrix();

    if (settingsMain.grid) {
        drawGrid();
    }

    if (!io.WantCaptureMouse && mouseMain.state == GLUT_DOWN && selecting) {
        drawBox();
    }
    
    // --------------------------- GRADIENT ---------------------------

    if (drawGradient) {
        glColor3f(1, 1, 1);
        glEnable (GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glTexImage2D (
            GL_TEXTURE_2D,
            0,
            GL_RGB,
            config->num_colours,
            1,
            0,
            GL_RGB,
            GL_UNSIGNED_INT,
            &(cmap[0])
        );

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0, -1.0);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(-1.0,  1.0);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(-0.9,  1.0);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.9, -1.0);
        glEnd();

        glDisable (GL_TEXTURE_2D);
    }

    // --------------------------- IMGUI ---------------------------

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowSize(ImVec2(220, 0));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 200, 0));

    ImGui::Begin("Gradient colors");
    ImGui::PushItemWidth(140);

    showInfo();

    if (ImGui::CollapsingHeader("Colour Map")) {
        showColourmapControls();
    }

    

    ImGui::End();

    // --------------------------- DRAW ---------------------------
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glFlush();
    glutSwapBuffers();
}

void updateView() {
    fprintf(stderr, "\n\n\n\n\n\nSetting region to:\n");
    fprintf(stderr, "scale = %.25g\n", viewMain.scaleY);
    fprintf(stderr, "center_x = %.25g\ncenter_y = %.25f\n", viewMain.centerX, viewMain.centerY);
    fprintf(stderr, "theta = %.6f\n", viewMain.theta);

    viewMain.scaleX = viewMain.scaleY / config->height * config->width;
    viewMain.cosTheta = cos(viewMain.theta);
    viewMain.sinTheta = sin(viewMain.theta);
    
#ifdef MANDEL_GPU_USE_DOUBLE
    transformView();
    opencl->setKernelArg("initParticles", 1, sizeof(ViewSettingsCL), &(viewMainCL));
#else
    opencl->setKernelArg("initParticles", 1, sizeof(ViewSettings), &(viewMain));
#endif
    opencl->step("initParticles");
    iterCount = 0;
}

void selectRegion() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || mouseMain.state != GLUT_DOWN) {
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

void writeData() {
    int i, j, k;
    FILE *outFile;
    char filename[200];

#ifdef MANDEL_GPU_USE_DOUBLE
    sprintf(filename, "raw_images/weighted_%lu_%lu_%.6f_%.6f_%.6f_%.6f_%d.csv", 
        viewMain.sizeX, viewMain.sizeY, viewMain.scaleY, viewMain.theta, viewMain.centerX, viewMain.centerY, count0);
#else
    sprintf(filename, "raw_images/weighted_%d_%d_%.6f_%.6f_%.6f_%.6f_%d.csv", 
        viewMain.sizeX, viewMain.sizeY, viewMain.scaleY, viewMain.theta, viewMain.centerX, viewMain.centerY, count0);
#endif
    
    outFile = fopen(filename, "w");

    if (outFile == NULL) {
        fprintf(stderr, "\nError opening file %s\n", filename);
        return;
    }

    k = 0;
    for (j = 0; j < viewMain.sizeY; j++) {
        fprintf(outFile, "%u", pixelsMain[k]);
        k++;

        for (i = 1; i < 3 * viewMain.sizeX; i++) {
            fprintf(outFile, ",%u", pixelsMain[k]);
            k++;
        }
        fprintf(outFile, "\n");
    }

    fclose(outFile);
}

void keyPressedMain(unsigned char key, int x, int y) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGLUT_KeyboardFunc(key, x, y);
    if (io.WantCaptureKeyboard) {
        return;
    }
    

    switch (key) {
        case 'a':
            selectRegion();
            break;
        case 'e':
            glutSetWindow(windowIdMain);
            glutPostRedisplay();
            break;
        
        case 'W':
            writeData();
            break;
        
        case 'g':
            settingsMain.grid = ! settingsMain.grid;
            break;
        case 'h':
            drawGradient = ! drawGradient;
            break;

        case 'b':
            selecting = ! selecting;
            break;
        case 'n':
            drawDerivatives = ! drawDerivatives;
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
#ifdef MANDEL_GPU_USE_DOUBLE
                transformView();
                opencl->setKernelArg("initParticles", 1, sizeof(ViewSettingsCL), &(viewMainCL));
#else
                opencl->setKernelArg("initParticles", 1, sizeof(ViewSettings), &(viewMain));
#endif
                opencl->step("initParticles");
                iterCount = 0;
            }
            break;

        case 'w':
            settingsMain.zoom *= 1.5;
            break;
        case 's':
            settingsMain.zoom /= 1.5;
            break;

        case '[':
            viewMain.scaleY *= 1.2;
            updateView();
            break;
        case ']':
            viewMain.scaleY /= 1.2;
            updateView();
            break;
        
        case ',':
            if (count0 > 0) {
                count0 -= 4;
                opencl->setKernelArg("renderImage", 4, sizeof(unsigned int), &count0);
            }
            break;
        case '.':
            count0 += 4;
            opencl->setKernelArg("renderImage", 4, sizeof(unsigned int), &count0);
            break;

        case 'R':
            opencl->step("initParticles");
            iterCount = 0;
            break;

        case 'q':
            exit(0);
            break;
        default:
            break;
    }
}

void specialKeyPressedMain(int key, int x, int y) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGLUT_SpecialFunc(key, x, y);
    if (io.WantCaptureKeyboard) {
        return;
    }

    viewStackMain.push(ViewSettings(viewMain));

    switch (key) {
        case GLUT_KEY_RIGHT:
            viewMain.centerX += 0.2 * viewMain.scaleY * viewMain.cosTheta;
            viewMain.centerY -= 0.2 * viewMain.scaleY * viewMain.sinTheta;
            break;
        case GLUT_KEY_LEFT:
            viewMain.centerX -= 0.2 * viewMain.scaleY * viewMain.cosTheta;
            viewMain.centerY += 0.2 * viewMain.scaleY * viewMain.sinTheta;
            break;
        case GLUT_KEY_UP:
            viewMain.centerX += 0.2 * viewMain.scaleY * viewMain.sinTheta;
            viewMain.centerY += 0.2 * viewMain.scaleY * viewMain.cosTheta;
            break;
        case GLUT_KEY_DOWN:
            viewMain.centerX -= 0.2 * viewMain.scaleY * viewMain.sinTheta;
            viewMain.centerY -= 0.2 * viewMain.scaleY * viewMain.cosTheta;
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
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGLUT_MouseFunc(button, state, x, y);

    if (!io.WantCaptureMouse && button == GLUT_RIGHT_BUTTON) {
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
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGLUT_MotionFunc(x, y);

    mouseMain.x = x;
    mouseMain.y = y;

    if (!io.WantCaptureMouse) {
        fillTestArray();
    }
}

void onReshapeMain(int w, int h) {
    ImGui_ImplGLUT_ReshapeFunc(w, h);

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
    
    zArray = (WorldCoordinate *)malloc(test_steps * sizeof(WorldCoordinate));
    dzxArray = (WorldCoordinate *)malloc(test_steps * sizeof(WorldCoordinate));
    dzyArray = (WorldCoordinate *)malloc(test_steps * sizeof(WorldCoordinate));

    for (int i = 0; i < 3 * width * height; i++) {
        pixelsMain[i] = 0;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;

    ImGui::StyleColorsDark();
    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL2_Init();

    glutKeyboardUpFunc(ImGui_ImplGLUT_KeyboardUpFunc);
    glutSpecialUpFunc(ImGui_ImplGLUT_SpecialUpFunc);

    glutKeyboardFunc(&keyPressedMain);
    glutSpecialFunc(&specialKeyPressedMain);
    glutMouseFunc(&mousePressedMain);
    glutMotionFunc(&mouseMovedMain);
    glutPassiveMotionFunc(&mouseMovedMain);
    glutReshapeFunc(&onReshapeMain);

    glutDisplayFunc(&displayMain);
}

void destroyMainWindow() {
    free(pixelsMain);
    
    free(zArray);
    free(dzxArray);
    free(dzyArray);
}

#ifdef MANDEL_GPU_USE_DOUBLE
IntPair to_pair(double num) {
    unsigned int sign = num >= 0;
    uint64_t inum = (uint64_t)fabs(num);

    return (IntPair){
        sign, inum, (uint64_t)(((double)fabs(num) - (double)inum) * (~0UL))
    };
}

void transformView() {
    viewMainCL.scaleX = to_pair(viewMain.scaleX);
    viewMainCL.scaleY = to_pair(viewMain.scaleY);
    viewMainCL.centerX = to_pair(viewMain.centerX);
    viewMainCL.centerY = to_pair(viewMain.centerY);
    viewMainCL.theta = to_pair(viewMain.theta);
    viewMainCL.sinTheta = to_pair(viewMain.sinTheta);
    viewMainCL.cosTheta = to_pair(viewMain.cosTheta);
    viewMainCL.sizeX = viewMain.sizeX;
    viewMainCL.sizeY = viewMain.sizeY;
    viewMainCL.particlesX = viewMain.particlesX;
    viewMainCL.particlesY = viewMain.particlesY;
}
#endif

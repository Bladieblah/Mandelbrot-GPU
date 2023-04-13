#include <chrono>

#include <GLUT/glut.h>

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glut.h"
#include "../imgui/backends/imgui_impl_opengl2.h"

#include "colourMap.hpp"
#include "config.hpp"

using namespace std;


int windowIdGradient;
uint32_t *pixelsGradient;

float col1[3];

void fill_gradient() {

}

void displayGradient() {
    // --------------------------- RESET ---------------------------
    glutSetWindow(windowIdGradient);

    ImGuiIO& io = ImGui::GetIO();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // --------------------------- GRADIENT ---------------------------

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
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0, -1.0);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0,  1.0);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0,  1.0);
    glEnd();

    glDisable (GL_TEXTURE_2D);

    // --------------------------- IMGUI ---------------------------

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();

    for (size_t i = 0; i < cm->getColorCount(); i++) {
        if (ImGui::ColorPicker3("Test Picker", col1, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV)) {
            fprintf(stderr, "%f,%f,%f\n", col1[0], col1[1], col1[2]);
        }
    }

    // bool changed = false;

    // if () {
    //     updateColormap();
    // }

    // --------------------------- DRAW ---------------------------
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glutSwapBuffers();
}

void createGradientWindow() {
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_MULTISAMPLE);
    glutInitWindowSize(1280, 720);
    windowIdGradient = glutCreateWindow("Dear ImGui GLUT+OpenGL2 Example");

    // pixelsGradient = (uint32_t *)malloc(3 * config->num_colours * sizeof(uint32_t));

    glutDisplayFunc(&displayGradient);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL2_Init();
    ImGui_ImplGLUT_InstallFuncs();
}
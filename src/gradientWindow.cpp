#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glut.h"
#include "../imgui/backends/imgui_impl_opengl2.h"

#define GL_SILENCE_DEPRECATION

#include <GLUT/glut.h>

int windowIdGradient;

float col1[4];

void displayGradient() {
    glutSetWindow(windowIdGradient);

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();

    ImGui::ColorPicker4("Test Picker", col1);

    // Rendering
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glutSwapBuffers();
}

void createGradientWindow() {
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_MULTISAMPLE);
    glutInitWindowSize(1280, 720);
    windowIdGradient = glutCreateWindow("Dear ImGui GLUT+OpenGL2 Example");

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
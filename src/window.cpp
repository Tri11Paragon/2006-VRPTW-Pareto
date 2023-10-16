//
// Created by brett on 16/10/23.
//
#include <blt/window/window.h>

#include "imgui.h"
#include "implot.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <blt/std/logging.h>
#include <blt/std/assert.h>
#include <blt/std/hashmap.h>


#ifdef BLT_BUILD_GLFW

static void glfw_error_callback(int error, const char* description)
{
    BLT_ERROR("GLFW Error %d: %s\n", error, description);
}

namespace blt
{
    
    struct window_container {
        GLFWwindow* window = nullptr;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    } window;
    
    void init_glfw()
    {
        glfwSetErrorCallback(glfw_error_callback);
        BLT_ASSERT(glfwInit());
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
        
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        
        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();
    }
    
    void create_window(size_t width, size_t height)
    {
        // Create window with graphics context
        window.window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
        assert(window != nullptr);
        glfwMakeContextCurrent(window.window);
        glfwSwapInterval(1); // Enable vsync
        
        const char* glsl_version = "#version 130";
        
        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window.window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
        
        ImPlot::CreateContext();
    }
    
    bool draw(const std::function<void()>& run)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        run();
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window.window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(window.clear_color.x * window.clear_color.w, window.clear_color.y * window.clear_color.w, window.clear_color.z * window.clear_color.w, window.clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window.window);
        
        if (glfwWindowShouldClose(window.window))
        {
            cleanup();
            return false;
        } else
            return true;
    }
    
    void cleanup()
    {
        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(window.window);
        glfwTerminate();
    }
    
}

#endif
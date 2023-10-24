//
// Created by brett on 16/10/23.
//

#if defined __has_include
    #if __has_include(<GLFW/glfw3.h>) && __has_include("imgui_impl_glfw.h")
	#define BLT_BUILD_GLFW
    #endif
#endif

#ifndef BLT_WINDOW_H
#define BLT_WINDOW_H

#include <string>
#include <functional>

namespace blt
{
#ifdef BLT_BUILD_GLFW
    
    void init_glfw();
    void create_window(size_t width = 1280, size_t height = 720);
    // returns false if the window has closed.
    bool draw(const std::function<void()>& run);
    void cleanup();

#endif
}

#endif //BLT_WINDOW_H

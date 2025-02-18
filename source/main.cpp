// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "RichTextDocument.h"
#include "misc/freetype/imgui_freetype.h"
#include <cstddef>
#include <nlohmann/json.hpp>
#define SDL_MAIN_HANDLED

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_stdlib.h"
#include <stdio.h>
#include <SDL3/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include "glad/gl.h"
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif



#include "imgui/imnodes.h"
#include "node.hpp"
#include "graph.h"
#include "RichTextEditor.h"


// Main code
int main(int, char**)
{

    // Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", 1280, 720, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }

    int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
    if (version == 0) {
        printf("Failed to initialize OpenGL context\n");
        return -1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    
    // Query default monitor resolution
    float windowScale = SDL_GetWindowDisplayScale(window);
    ImGui::GetStyle().ScaleAllSizes(windowScale);


    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();
    
    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImNodes::CreateContext();
    ImNodes::StyleColorsLight();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    // io.Fonts->AddFontDefault();
    static ImWchar ranges[] = { 0x1, (ImWchar)0x1FFFF, 0 };
    
    ImFontConfig fontCfg;
    fontCfg.OversampleH = 2;
    fontCfg.OversampleV = 2;
    fontCfg.RasterizerMultiply = 1.5f;
    fontCfg.RasterizerDensity = windowScale;

    ImFontConfig emojiCfg;
    emojiCfg.OversampleH = 2;
    emojiCfg.OversampleV = 2;
    emojiCfg.RasterizerMultiply = 1.5f;
    emojiCfg.RasterizerDensity = windowScale;
    emojiCfg.MergeMode = true;
    emojiCfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;

    ImFont* font = io.Fonts->AddFontFromFileTTF("resource/CourierPrime-Regular.ttf", windowScale * 18.0f, &fontCfg, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("resource/Twemoji.Mozilla.ttf", windowScale * 18.0f, &emojiCfg, ranges);

    ImFont* fontBold = io.Fonts->AddFontFromFileTTF("resource/CourierPrime-Bold.ttf", windowScale * 18.0f, &fontCfg, ranges);
    io.Fonts->AddFontFromFileTTF("resource/Twemoji.Mozilla.ttf", windowScale * 18.0f, &emojiCfg, ranges);

    ImFont* fontItalic = io.Fonts->AddFontFromFileTTF("resource/CourierPrime-Italic.ttf", windowScale * 18.0f, &fontCfg, ranges);
    io.Fonts->AddFontFromFileTTF("resource/Twemoji.Mozilla.ttf", windowScale * 18.0f, &emojiCfg, ranges);

    ImFont* fontItalicBold = io.Fonts->AddFontFromFileTTF("resource/CourierPrime-BoldItalic.ttf", windowScale * 18.0f, &fontCfg, ranges);
    io.Fonts->AddFontFromFileTTF("resource/Twemoji.Mozilla.ttf", windowScale * 18.0f, &emojiCfg, ranges);

    IM_ASSERT(font != nullptr && fontBold != nullptr && fontItalic != nullptr && fontItalicBold != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::string node_1_name;
    std::string node_2_name;

    example::Graph<Node> graph;

    auto json = nlohmann::json::parse(R"(
        {
            "text": "Some Text Here! 😀 🦊 ",
            "bold": true,
            "underline": true,
            "link": "https://github.com/cullvox",
            "children": [
                {
                    "text": "Some more GIDDY gjigiy child 🦊 text!",
                    "bold": true,
                    "size": 45.0,
                    "italic": true,
                    "underline": true
                },
                {
                    "text": "\nshould be bold! 🦊",
                    "size": 23.0,
                    "underline": true,
                    "color": "#Ab4C3245"
                },
                {
                    "text": "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Fusce nulla nunc, pellentesque sodales odio et, sodales placerat diam. Pellentesque ante tortor, tristique a scelerisque eu, lacinia at justo. Cras gravida, nulla ac feugiat elementum, dolor orci malesuada est, eget molestie urna eros at justo. Nam quis neque vitae velit consectetur imperdiet. Quisque ligula leo, auctor eget cursus ac, consequat in justo. Praesent feugiat euismod mauris in mattis. Phasellus pellentesque quis eros vel interdum. Donec facilisis enim orci, sed ullamcorper mi dapibus vel. Aliquam dignissim eleifend nulla ac ullamcorper. Nullam ultricies tortor nec semper pretium. Nam venenatis tortor non magna auctor fermentum. Pellentesque non velit faucibus, mattis felis quis, malesuada augue. Nam a condimentum orci. ",
                    "size": 18.0,
                    "bold": false
                },
                {
                    "text": "\nSOME MORE TEXT! 🦊",
                    "bold": false,
                    "size": 23.0,
                    "highlight": "#Ab70ded7"
                },
                {
                    "text": "\nSOME MORE GIDDY gjigiy TEXT! 🦊",
                    "bold": false,
                    "underline": false
                }
            ]
        }
        )");

    RichTextDocument doc{json};
    RichTextEditor editor{font, fontBold, fontItalic, fontItalicBold};
    editor.SetDocument(doc);
    editor.SetDPIScaling(windowScale);

    ImNodes::GetStyle().GridSpacing *= windowScale;

    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // ImGui::PushFont(font);

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::DockSpaceOverViewport();


        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        ImGui::Begin("Nodes!");
        ImNodes::BeginNodeEditor();

        const float node_width = 60.0f;

        ImNodes::BeginNode(0);

        ImNodes::BeginNodeTitleBar();
        ImGui::SetNextItemWidth(node_width + 25.0f);
        ImGui::InputText("", &node_1_name);
        ImNodes::EndNodeTitleBar();


        ImNodes::BeginInputAttribute(0);
        ImGui::Text("In");
        ImNodes::EndInputAttribute();

        ImGui::SameLine();

        ImNodes::BeginOutputAttribute(1);
        const float label_width = ImGui::CalcTextSize("Out").x;
        ImGui::Indent(node_width - label_width);
        ImGui::TextUnformatted("Out");
        ImNodes::EndOutputAttribute();

        ImNodes::EndNode();

        ImNodes::BeginNode(3);

        ImNodes::BeginNodeTitleBar();
        ImGui::SetNextItemWidth(node_width + 25.0f);
        ImGui::InputText("", &node_2_name);
        ImNodes::EndNodeTitleBar();


        ImNodes::BeginInputAttribute(2);
        ImGui::Text("In");
        ImNodes::EndInputAttribute();

        ImGui::SameLine();

        ImNodes::BeginOutputAttribute(3);
        ImGui::Indent(node_width - label_width);
        ImGui::TextUnformatted("Out");
        ImNodes::EndOutputAttribute();

        ImNodes::EndNode();

        ImNodes::Link(8, 1, 2);

        ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);
        
        ImNodes::EndNodeEditor();
        ImGui::End();

        ImGui::Begin("Inspector");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        int selected_count = ImNodes::NumSelectedNodes();
        if (selected_count > 0) {
            std::vector<int> selected_nodes((size_t)selected_count);
            ImNodes::GetSelectedNodes(selected_nodes.data());

            int node = selected_nodes[0];

            auto position = ImNodes::GetNodeGridSpacePos(node);
            ImGui::Text("Node ID: %d\nNode Position: (%f, %f)", node, position.x, position.y); 
        }

        ImGui::End();

        ImGui::Begin("Script");

        editor.Render();

        ImGui::End();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplayFramebufferScale.x, (int)io.DisplayFramebufferScale.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImNodes::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

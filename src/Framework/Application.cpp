#include <Framework/Application.hpp>

// clang-format off
#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
// clang-format on

#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

void Application::run()
{
    FrameMarkStart("App Run");
    if (!initialize())
    {
        return;
    }

    spdlog::info("Application: Initialized");

    if (!load())
    {
        return;
    }

    spdlog::info("Application: Loaded");

    f64 previousTime = glfwGetTime();
    while (!glfwWindowShouldClose(_windowHandle))
    {
        f64 currentTime = glfwGetTime();
        f32 frameTime = (f32)(currentTime - previousTime);
        if (frameTime > 0.1F)
        {
            frameTime = 0.1F;
        }
        previousTime = currentTime;

        glfwPollEvents();
        update(frameTime);
        render(frameTime);
    }

    spdlog::info("Application: Unloading");

    unload();

    spdlog::info("Application: Unloaded");
    FrameMarkEnd("App Run");
}

void Application::close()
{
    glfwSetWindowShouldClose(_windowHandle, 1);
}

bool Application::keyIsPressed(s32 key)
{
    return glfwGetKey(_windowHandle, key) == GLFW_PRESS;
}

bool Application::initialize()
{
    if (!glfwInit())
    {
        spdlog::error("Glfw: Unable to initialize");
        return false;
    }

    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *primaryMonitorVideoMode = glfwGetVideoMode(primaryMonitor);

    _windowHandle = glfwCreateWindow(_windowWidth, _windowHeight, "Golf Flight Simulator 3D", NULL, NULL);
    if (_windowHandle == NULL)
    {
        spdlog::error("Glfw: Unable to create window");
        glfwTerminate();
        return false;
    }

    u32 screenWidth = primaryMonitorVideoMode->width;
    u32 screenHeight = primaryMonitorVideoMode->height;
    glfwSetWindowPos(_windowHandle, screenWidth / 2 - _windowWidth / 2, screenHeight / 2 - _windowHeight / 2);

    glfwMakeContextCurrent(_windowHandle);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    ImGui::CreateContext();
    afterCreatedUIContext();
    ImGui_ImplGlfw_InitForOpenGL(_windowHandle, true);
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    return true;
}

static void GLAPIENTRY openGLDebugCallBack(GLenum source,
                                           GLenum type,
                                           GLuint id,
                                           GLenum severity,
                                           GLsizei length,
                                           const GLchar *message,
                                           const GLvoid *userParam)
{
    (void)id;
    (void)length;
    (void)userParam;

    const char *sourceName;
    switch (source)
    {
        case GL_DEBUG_SOURCE_API_ARB:
            sourceName = "API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
            sourceName = "Window System";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
            sourceName = "Shader Compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
            sourceName = "Third Party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION_ARB:
            sourceName = "Application";
            break;
        case GL_DEBUG_SOURCE_OTHER_ARB:
            sourceName = "Other";
            break;
    }

    const char *errorType;
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR_ARB:
            errorType = "Error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
            errorType = "Deprecated Functionality";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
            errorType = "Undefined Behavior";
            break;
        case GL_DEBUG_TYPE_PORTABILITY_ARB:
            errorType = "Portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE_ARB:
            errorType = "Performance";
            break;
        case GL_DEBUG_TYPE_OTHER_ARB:
            errorType = "Other";
            break;
    }

    const char *typeSeverity;
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH_ARB:
            typeSeverity = "High";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM_ARB:
            typeSeverity = "Medium";
            break;
        case GL_DEBUG_SEVERITY_LOW_ARB:
            typeSeverity = "Low";
            break;
    }

    spdlog::debug("OpenGL: {} message from {},\tSeverity: {}\nMessage: {}\n", errorType, sourceName, typeSeverity,
                  message);
    if (severity == GL_DEBUG_SEVERITY_HIGH_ARB)
    {
        assert(false);
    }
}

bool Application::load()
{
    if (GL_ARB_debug_output)
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageCallbackARB(openGLDebugCallBack, NULL);
    }

    glEnable(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glfwSwapInterval(1);

    return true;
}

void Application::unload()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    beforeDestroyUIContext();
    ImGui::DestroyContext();

    glfwTerminate();
}

void Application::render(f32 frameTime)
{
    ZoneScopedC(tracy::Color::Red2);

    renderScene(frameTime);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {
        renderUI(frameTime);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        ImGui::EndFrame();
    }

    glfwSwapBuffers(_windowHandle);
}

void Application::renderScene(f32 frameTime)
{
    (void)frameTime;
}

void Application::renderUI(f32 frameTime)
{
    (void)frameTime;
}

void Application::update(f32 frameTime)
{
    (void)frameTime;
}

void Application::afterCreatedUIContext() {}

void Application::beforeDestroyUIContext() {}

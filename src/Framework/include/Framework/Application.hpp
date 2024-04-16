#pragma once

#include "Types.hpp"

#include <assert.h>
#include <string.h>

#define arrayCount(arr) (sizeof(arr) / sizeof((arr)[0]))

#define invalidDefaultCase assert(!"ERROR: Invalid default case")
#define invalidCodePath assert(!"ERROR: Invalid code path")

#define bzero(b, len) (memset((b), 0, (len)))

struct GLFWwindow;

class Application
{
public:
    void run();

protected:
    void close();
    bool keyIsPressed(s32 key);

    f64 getDeltaTime();

    virtual void afterCreatedUIContext();
    virtual void beforeDestroyUIContext();
    virtual bool initialize();
    virtual bool load();
    virtual void unload();
    virtual void renderScene(f32 frameTime);
    virtual void renderUI(f32 frameTime);
    virtual void update(f32 frameTime);

    GLFWwindow *_windowHandle;
    u32 _windowWidth = 1920;
    u32 _windowHeight = 1080;

private:
    void render(f32 frameTime);
};
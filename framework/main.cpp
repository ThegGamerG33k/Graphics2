#include "Framework.h"
#include "draw.h"
#include "update.h"
#include "setup.h"
#include "Globals.h"
#include <SDL.h>

void mainloop(Globals* globs);

int main(int , char**)
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Rect r;
    SDL_GetDisplayBounds( 0, &r );

    auto globs = new Globals();
    globs->height = int(r.w/4);
    globs->width = globs->height;

    SDL_Window* win = SDL_CreateWindow("ETGG", 10,40,
        globs->width, globs->height, SDL_WINDOW_VULKAN );

    VkPhysicalDeviceFeatures featuresToEnable{};
    featuresToEnable.fillModeNonSolid=VK_TRUE;
    auto ctx = new VulkanContext(win,featuresToEnable);

    globs->ctx = ctx;
    CommandBuffer::initialize(ctx);

    setup(globs);

    mainloop(globs);

    CleanupManager::cleanupEverything();
    globs->ctx->cleanup();

    SDL_Quit();
    return 0;
}

void mainloop(Globals* globs)
{
    int targetFramesPerSecond = 60;
    float targetSecondsPerFrame = float(1.0/targetFramesPerSecond);
    float timeQuantum = 0.005f ;     //5 msec
    float accumulated = 0;

    double last = timeutil::time_sec();

    while(globs->keepLooping){

        double now = timeutil::time_sec();
        float elapsed = float(now-last);
        last = now;

        accumulated += elapsed;
        while(accumulated >= timeQuantum){
            update(globs,timeQuantum);
            accumulated -= timeQuantum;
        }

        draw(globs);

        double end = timeutil::time_sec();
        double frameTime = end-now;
        double leftover = targetSecondsPerFrame - frameTime;
        if(leftover > 0){
            timeutil::sleep(leftover);
        }
    }
}


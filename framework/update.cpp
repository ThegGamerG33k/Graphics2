#include "Framework.h"
#include <SDL.h>
#include "Globals.h"
#include "imagedecode.h"
#include <iostream>
#include <fstream>
#include "Text.h"
#include <format>

void update(Globals* globs, float elapsed)
{
    SDL_Event ev;
    while(true){
        bool eventOccurred = SDL_PollEvent(&ev);
        if(!eventOccurred)
            break;
        if(ev.type == SDL_QUIT)
            globs->keepLooping=false;
        if(ev.type == SDL_KEYDOWN){
            auto k = ev.key.keysym.sym;
            globs->pressedKeys.insert(k);
            if(k == SDLK_ESCAPE )
                globs->keepLooping=false;
            if(k == SDLK_F1){
                auto [w,h,fmt,pix] = globs->ctx->screenshot(utils::getRenderedSwapchainImageIndex());
                auto p = imageencode::encodePNG(w,h,fmt,pix);
                std::ofstream fp("screenshot.png",std::ios::binary);
                fp.write(p.data(), p.size());
                std::cout << "Wrote screenshot.png\n";
            }
            if(k == SDLK_RETURN){
                if( globs->pressedKeys.contains(SDLK_LSHIFT) ){
                    if( globs->mouseLook ){
                        globs->mouseLook = false;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                    }
                    std::cout << "Eye?\n";
                    vec3 e;
                    std::string tmp;
                    std::cin >> e.x >> e.y >> e.z;
                    std::cout << "Coi?\n";
                    vec3 c;
                    std::cin >> c.x >> c.y >> c.z;
                    std::cout << "Up?\n";
                    vec3 u;
                    std::cin >> u.x >> u.y >> u.z;
                    globs->camera->lookAt(e,c,u);
                } else {
                    std::cout << "eye: " << globs->camera->eye.x << " " << globs->camera->eye.y << " " << globs->camera->eye.z << "\n";
                    vec3 c = globs->camera->eye + globs->camera->look;
                    std::cout << "coi: " << c.x << " " << c.y << " " << c.z << "\n";
                    std::cout << "up:  " << globs->camera->up.x << " " << globs->camera->up.y << " " << globs->camera->up.z << "\n";
                }
            }
            if(k == SDLK_TAB){
                globs->mouseLook = ! globs->mouseLook;
                if(globs->mouseLook)
                     SDL_SetRelativeMouseMode(SDL_TRUE);
                else
                     SDL_SetRelativeMouseMode(SDL_FALSE);
            }
        }
        if(ev.type == SDL_KEYUP ){
            globs->pressedKeys.erase(ev.key.keysym.sym);
        }

        if(ev.type == SDL_MOUSEBUTTONDOWN){
        }
        if(ev.type == SDL_MOUSEBUTTONUP){
        }
        if(ev.type == SDL_MOUSEMOTION){
            if(globs->mouseLook){
                globs->camera->turn( -0.01f*float(ev.motion.xrel));
                globs->camera->tilt( -0.01f*float(ev.motion.yrel));
            }
        }
    }


    if(globs->pressedKeys.contains(SDLK_w))
        globs->camera->strafeNoUpDown(0,0,elapsed);
    if(globs->pressedKeys.contains(SDLK_s))
        globs->camera->strafeNoUpDown(0,0,-elapsed);
    if(globs->pressedKeys.contains(SDLK_a))
        globs->camera->strafeNoUpDown(-elapsed,0,0);
    if(globs->pressedKeys.contains(SDLK_d))
        globs->camera->strafeNoUpDown(elapsed,0,0);
    if(globs->pressedKeys.contains(SDLK_j))
        globs->camera->turn(-elapsed);
    if(globs->pressedKeys.contains(SDLK_k))
        globs->camera->turn(elapsed);

    if(globs->pressedKeys.contains(SDLK_r))
        globs->camera->strafeUp(elapsed);
    if(globs->pressedKeys.contains(SDLK_f))
        globs->camera->strafeUp(-elapsed);


    globs->text->clear();
    std::string tmp = std::format("Eye: {:.02f} {:.02f} {:.02f}",globs->camera->eye.x, globs->camera->eye.y, globs->camera->eye.z);
    globs->text->print(12,102,1,1,1,1, tmp);
}

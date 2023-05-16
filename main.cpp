#include <SDL2/SDL.h>

#include "initSDL.h"
#include "render.h"
#include "drawContext.h"
#include "things.h"
#include "miniMap.h"
#include "scene.h"
#include "errors.h"


int 
main(int argc, char **argv)
{
    err_code ret = initial_setup();
    if(ret != NO_ERROR)
        std::exit(ret);
   
    INIT_DRAW_CONTEXT(dc);
    if ( !dc.isValid() )
        std::exit(dc.m_error);

    SDL_Event e; 
    bool canRun = true; 

    Map map("./map.txt");
    if( !map.isLoaded() )
        std::exit(MAP_NOT_LOADED);

    Player player(1.5, 1.5);

    miniMap mm{};

    scene sc {
        map, player, mm,
    };
    do {
        while( SDL_PollEvent(&e) ) {
            if(e.type == SDL_QUIT) {
                canRun = false;
            } else
            if(e.type == SDL_KEYDOWN) {
                player.handle( e.key.keysym.sym, map);
            }
        }
        dc.clear();
        draw(sc, dc);
        mm.draw(map, player, dc);
        dc.update();
    } while(canRun);

    std::exit(EXIT_SUCCESS);
}

#include <SDL2/SDL.h>
#include <cstddef>
#include <memory>

#include "initSDL.h"
#include "render.h"
#include "drawContext.h"
#include "things.h"
#include "miniMap.h"
#include "scene.h"
#include "tileMap.h"
#include "errors.h"


#ifndef ASSETS_PATH
#define ASSETS "."
#endif

#ifdef BENCH_RENDER
#include "timer.h"
#endif


int 
main(int argc, char **argv)
{

    err_code ret = initial_setup();
    if(ret != NO_ERROR)
        std::exit(ret);
   
    INIT_DRAW_CONTEXT(dc);
    if ( !dc.isValid() )
        std::exit(dc.m_error);

    Map map(ASSETS_PATH"/maps/test_map");
    if( !map.isLoaded() )
        std::exit(MAP_NOT_LOADED);

    tileMap tm;
    tm.load(ASSETS_PATH"/maps/test_map/pack2.png");
    if( !tm.isLoaded() )
        std::exit(TILEMAP_NOT_LOADED);

    tileMap coin_txt{0x0000FF00};
    coin_txt.load(ASSETS_PATH"/items/my_coin.png");
    if( !coin_txt.isLoaded() )
        std::exit(TILEMAP_NOT_LOADED);

    Thing player(1.5, 1.5);
    Thing coin1{4.5, 4.5, &coin_txt, 0};
    Thing coin2{1.5, 1.5, &coin_txt, 0};

    Things things{};
    int min_things_no = 16;
    std::vector<int> things_ids_buff{};
    std::vector<float> dists_to_player{};
    things.reserve(min_things_no);
    things_ids_buff.resize(min_things_no);
    dists_to_player.resize(min_things_no);
    
    things.push_back( std::move(coin1) );
    things.push_back( std::move(coin2) );

    miniMap mm{};

    scene sc {
        map, player, things, mm,
    };

    std::unique_ptr<float[]> z_buffer( new float[dc.SCREEN_WIDTH] );
    drawBuffers db {z_buffer.get(), dists_to_player, things_ids_buff};

    SDL_Event e; 
    bool canRun = true; 
#ifdef BENCH_RENDER
        timer tmr{};
#endif
    while(canRun) {
        auto th_size = things.size();
        if(db.things_ids.size() < th_size) db.things_ids.resize(th_size * 2);
        if(db.things_dst.size() < th_size) db.things_dst.resize(th_size * 2);

        while( SDL_PollEvent(&e) ) {
            if(e.type == SDL_QUIT) {
                canRun = false;
            } else
            if(e.type == SDL_KEYDOWN) {
                player.handle( e.key.keysym.sym, map);
            }
        }
#ifdef BENCH_RENDER
        tmr.reset();
#endif
        dc.clear();
        draw(sc, dc, tm, db);
        mm.draw(map, player, dc);
        dc.update();
#ifdef BENCH_RENDER
        tmr.timeit();
        std::cout << "Render took " << tmr.getElapsedSC() << " seconds." << std::endl;
#endif
    }

    std::exit(EXIT_SUCCESS);
}

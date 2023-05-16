#include <SDL2/SDL.h>
#include <iostream>
#include <cstdlib>

#include <initSDL.h>

void cleanup_at_exit(void) 
{
    SDL_Quit();
    std::cout << "Clean up handler is successfull." << "\n"; 
}

err_code initial_setup(void)
{
    if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
        std::cout << "Couldn't init SDL with error " << SDL_GetError() << "\n"; 
        return INIT_FAIL;
    }
    std::cout << "SDL inited successfully.\n"; 
    
    if( 0 != std::atexit(cleanup_at_exit) ) {
        std::cout << "Couldn't register cleanup_at_exit exit handler" << "\n";
        return EXIT_HANDLER_REG_FAIL;
    }

    return NO_ERROR;
}



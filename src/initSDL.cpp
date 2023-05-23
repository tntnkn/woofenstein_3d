#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <cstdlib>

#include <initSDL.h>

void cleanup_at_exit(void) 
{
    IMG_Quit();
    SDL_Quit();
#ifdef DEBUG
    std::cout << "Clean up handler is successfull." << "\n"; 
#endif
}

err_code initial_setup(void)
{
    if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
#ifdef DEBUG
        std::cout << "Couldn't init SDL with error " << SDL_GetError() << "\n"; 
#endif
        return INIT_FAIL;
    }
#ifdef DEBUG
    std::cout << "SDL inited successfully.\n"; 
#endif

    int img_flags = IMG_INIT_PNG;
    if( (IMG_Init(img_flags) & img_flags) != img_flags)
    {
#ifdef DEBUG
        std::cout << "Couldn't init SDL_image with error " 
                  << IMG_GetError() << std::endl;
#endif
        return INIT_IMAGE_FAIL;
    }
#ifdef DEBUG
    std::cout << "SDL_image inited successfully.\n"; 
#endif
    
    if( 0 != std::atexit(cleanup_at_exit) ) {
#ifdef DEBUG
        std::cout << "Couldn't register cleanup_at_exit exit handler" << "\n";
#endif
        return EXIT_HANDLER_REG_FAIL;
    }

    return NO_ERROR;
}

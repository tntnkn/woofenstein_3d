#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <cstdlib>

#include <initSDL.h>

void cleanup_at_exit(void) 
{
    IMG_Quit();
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

    int img_flags = IMG_INIT_PNG;
    if( (IMG_Init(img_flags) & img_flags) != img_flags)
    {
        printf( "Couldn't init SDL_image with error ", IMG_GetError() );
        return INIT_IMAGE_FAIL;
    }
    std::cout << "SDL_image inited successfully.\n"; 
    
    if( 0 != std::atexit(cleanup_at_exit) ) {
        std::cout << "Couldn't register cleanup_at_exit exit handler" << "\n";
        return EXIT_HANDLER_REG_FAIL;
    }

    return NO_ERROR;
}

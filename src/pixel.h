#include <SDL2/SDL.h>


#define REQUIRED_PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888

#define RMASK 0x00FF0000
#define GMASK 0x0000FF00
#define BMASK 0x000000FF
#define AMASK 0xFF000000

#define ONEALPHA 0x01000000

#define GET_R(c) ( (c & RMASK) >> 16 )
#define GET_G(c) ( (c & GMASK) >> 8  )
#define GET_B(c) ( (c & GMASK)       )
#define GET_A(c) ( (c & AMASK) >> 24 )

/*
#define RGBA_TO_REQUIRED(c) (           \
        ((c) & 0x000000FF) << 24 |      \
        ((c) & 0xFF000000) >> 8  |      \
        ((c) & 0x00FF0000) >> 8  |      \
        ((c) & 0x0000FF00) >> 8)
*/
#define RGBA_TO_REQUIRED(c) (           \
        ((c) & 0x000000FF) << 24 |      \
        ((c) & 0xFFFFFF00) >> 8)

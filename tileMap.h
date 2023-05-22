#ifndef TILEMAP_SENTRY
#define TILEMAP_SENTRY


#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_surface.h>
#include <cstdint>
#include <iostream>
#include <memory>

#include "errors.h"


#define DEFAULT_TW 64
#define DEFAULT_TH 64
#define TILEMAP_PTR std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)> 


struct colorRBG {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};


class tileMap {
  public:
    const int m_tw = DEFAULT_TW;
    const int m_th = DEFAULT_TH;
    const int m_td = DEFAULT_TW * DEFAULT_TH;

    tileMap()  {}
    ~tileMap() { m_pixels = nullptr; }

    err_code load(const char *path) {
        TILEMAP_PTR tm { IMG_Load(path), SDL_FreeSurface };
        if(NULL == tm) {
            std::cout << "Error loading " << path 
                      << " with error "   << IMG_GetError() << std::endl;
            return TILEMAP_NOT_LOADED;
        }
        if(tm->format->BytesPerPixel != 4) {
            // Maybe will play with it later.
            std::cout << "Wrong pixel size of tile map " << path << ". "
                      << tm->format->BytesPerPixel << ", but 4 expected." 
                      << std::endl;
            return TILEMAP_WRONG_PIXEL_SIZE;
        }

        uint32_t *p = __extractPixels( tm.get() );
        if(!p) {
            std::cout << "Cannot get pixels from tilemap " << path << "."
                      << std::endl;
            return TILEMAP_NO_PIXELS_GOT;
        }

        R_mask = tm->format->Rmask;
        G_mask = tm->format->Gmask;
        B_mask = tm->format->Bmask;
        R_loss = tm->format->Rloss;
        G_loss = tm->format->Gloss;
        B_loss = tm->format->Bloss;
        Rshift = tm->format->Rshift;
        Gshift = tm->format->Gshift;
        Bshift = tm->format->Bshift;

        m_no_textures = (tm->w/m_tw) * (tm->h/m_th);
        m_pixels.reset(p);

        return NO_ERROR; 
    }
    bool isLoaded() { return m_pixels != nullptr; }

    uint8_t get_r(uint32_t c) { return (((c&R_mask) >> Rshift) << R_loss); }
    uint8_t get_g(uint32_t c) { return (((c&G_mask) >> Gshift) << G_loss); }
    uint8_t get_b(uint32_t c) { return (((c&B_mask) >> Bshift) << B_loss); }

    uint32_t getColor(int t_no, int x, int y) {
        if(t_no >= m_no_textures || x >= m_tw || y >= m_th) {
            std::cout << "Addressing tilemap with wrong texture dimensions!"
                      << t_no << " " << x << " " << y << std::endl;
            return 0;
        }
        return m_pixels[ m_td*t_no + (m_tw*y+x) ];
    }

    colorRBG getColorRGB(int t_no, int x, int y) {
        int c = getColor(t_no, x, y);
        return {
            get_r(c),
            get_g(c),
            get_b(c),
        };
    }

  private:

    uint32_t *__extractPixels(SDL_Surface *s) {
        // At thip point during loading I am confident about the type.
        int sw = s->w;
        int sh = s->h;

        uint32_t *dst = reinterpret_cast<uint32_t*>(
            calloc(s->w*s->h, sizeof(uint32_t)) ); 
        uint32_t *src = reinterpret_cast<uint32_t*>(
            s->pixels);
        size_t i = 0;

        if( !dst)
            return NULL;
        
        for(int y = 0; y < sh/m_th; ++y) {
            int offy = y*m_th;
            for(int x = 0; x < sw/m_tw; ++x) {
                int offx = x*m_tw;
                for(int ty = 0; ty < m_th; ++ty) {
                    for(int tx = 0; tx < m_tw; ++tx) {
                        int y = (offy+ty)*sw + (offx+tx);
                        dst[i++] = src[y]; 
                    }
                }
            }
        }
        return dst;
    }

    //TILEMAP_PTR m_repr {nullptr, SDL_FreeSurface};
    size_t m_no_textures = 0;
    std::unique_ptr<uint32_t[], void(*)(void*)>m_pixels {nullptr, free};

    uint32_t R_mask;
    uint32_t G_mask;
    uint32_t B_mask;
    uint32_t R_loss;
    uint32_t G_loss;
    uint32_t B_loss;
    uint32_t Rshift;
    uint32_t Gshift;
    uint32_t Bshift;
};


#endif

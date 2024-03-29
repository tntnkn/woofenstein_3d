#ifndef TILEMAP_SENTRY
#define TILEMAP_SENTRY


#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_surface.h>
#include <cstdint>
#include <iostream>
#include <memory>

#include "pixel.h"
#include "errors.h"


#define DEFAULT_TW 64
#define DEFAULT_TH 64
#define TILEMAP_PTR std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)> 


struct colorRBGA {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};


class tileMap {
  public:
    const int m_tw = DEFAULT_TW;
    const int m_th = DEFAULT_TH;
    const int m_td = DEFAULT_TW * DEFAULT_TH;

    tileMap() {}
    tileMap(uint32_t transparent_color) : 
        m_transparent_color_is_set(true),
        // transparent_color is accepted as RGBA color and translated in ctor!
        m_transparent_color( RGBA_TO_REQUIRED(transparent_color) ) {};
    ~tileMap() { m_pixels = nullptr; m_no_textures = 0; }

    err_code load(const char *path) {
        TILEMAP_PTR tm { IMG_Load(path), SDL_FreeSurface };
        if(NULL == tm) {
#ifdef DEBUG
            std::cout << "Error loading " << path 
                      << " with error "   << IMG_GetError() << std::endl;
#endif
            return TILEMAP_NOT_LOADED;
        }
        if(tm->format->BytesPerPixel != 4) {
            // Maybe will play with it later.
#ifdef DEBUG
            std::cout << "Wrong pixel size of tile map " << path << ". "
                      << tm->format->BytesPerPixel << ", but 4 expected." 
                      << std::endl;
#endif
            return TILEMAP_WRONG_PIXEL_SIZE;
        }

        if(tm->format->format != REQUIRED_PIXEL_FORMAT) {
            TILEMAP_PTR s {
                SDL_CreateRGBSurface(
                    0, tm->w, tm->h, 32, RMASK, GMASK, BMASK, AMASK),
                SDL_FreeSurface
            };
            int r = SDL_ConvertPixels(
                tm->w, tm->h, tm->format->format, tm->pixels, tm->pitch,
                REQUIRED_PIXEL_FORMAT, s->pixels, tm->pitch);
            if(0 != r) {
#ifdef DEBUG
                std::cout << "Cannot convert tilemap format " << path 
                          << ". " << std::endl;
#endif
                return TILEMAP_CANNOT_CONVERT_PIXELS;
            }
            std::swap(tm, s);
        }

        if(m_transparent_color_is_set) {
            if(0 != SDL_SetColorKey(tm.get(), SDL_TRUE, m_transparent_color)) {
#ifdef DEBUG
                std::cout << "Cannot set color key for texture " << path 
                          << ". " << std::endl;
#endif
                return TILEMAP_CANNOT_SET_COLOR_KEY;
            }

            TILEMAP_PTR s {
                SDL_CreateRGBSurface(
                    0, tm->w, tm->h, 32, RMASK, GMASK, BMASK, AMASK),
                SDL_FreeSurface
            };
            SDL_BlitSurface(tm.get(), NULL, s.get(), NULL);

            std::swap(tm, s);
        }

        uint32_t *p = __extractPixels( tm.get() );
        if(!p) {
#ifdef DEBUG
            std::cout << "Cannot get pixels from tilemap " << path << "."
                      << std::endl;
#endif
            return TILEMAP_NO_PIXELS_GOT;
        }

        R_mask = tm->format->Rmask;
        G_mask = tm->format->Gmask;
        B_mask = tm->format->Bmask;
        A_mask = tm->format->Amask;
        R_loss = tm->format->Rloss;
        G_loss = tm->format->Gloss;
        B_loss = tm->format->Bloss;
        A_loss = tm->format->Aloss;
        Rshift = tm->format->Rshift;
        Gshift = tm->format->Gshift;
        Bshift = tm->format->Bshift;
        Ashift = tm->format->Ashift;

        m_no_textures = (tm->w/m_tw) * (tm->h/m_th);
        m_pixels.reset(p);

        return NO_ERROR; 
    }
    bool isLoaded() { return m_pixels != nullptr; }

    uint8_t get_r(uint32_t c) { return (((c&R_mask) >> Rshift) << R_loss); }
    uint8_t get_g(uint32_t c) { return (((c&G_mask) >> Gshift) << G_loss); }
    uint8_t get_b(uint32_t c) { return (((c&B_mask) >> Bshift) << B_loss); }
    uint8_t get_a(uint32_t c) { return (((c&A_mask) >> Ashift) << A_loss); }

    uint32_t getColor(int t_no, int x, int y) {
        if(t_no >= m_no_textures || x >= m_tw || y >= m_th) {
#ifdef DEBUG
            std::cout << "Addressing tilemap with wrong texture dimensions!"
                      << t_no << " " << x << " " << y << std::endl;
#endif
            return 0;
        }
        return m_pixels[ m_td*t_no + (m_tw*y+x) ];
    }

    colorRBGA getColorRGBA(int t_no, int x, int y) {
        int c = getColor(t_no, x, y);
        return {
            get_r(c),
            get_g(c),
            get_b(c),
            get_a(c),
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
    uint32_t A_mask;
    uint32_t R_loss;
    uint32_t G_loss;
    uint32_t B_loss;
    uint32_t A_loss;
    uint32_t Rshift;
    uint32_t Gshift;
    uint32_t Bshift;
    uint32_t Ashift;

    uint32_t m_transparent_color = 0;
    bool m_transparent_color_is_set = false;
};


#endif

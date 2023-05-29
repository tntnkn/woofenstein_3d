#ifndef DRAWCONTEXT_SENTRY
#define DRAWCONTEXT_SENTRY


#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <functional>
#include <cassert>
#include <errors.h>
#include "pixel.h"


using pos_t = int;

class drawContext {
    public:
        static const pos_t SCREEN_WIDTH  = 640;
        static const pos_t SCREEN_HEIGHT = 480;
        static const uint_fast32_t SCALE = 64;

        std::unique_ptr<
                        SDL_Window, 
                        void(*)(SDL_Window *)
                       > m_window {nullptr, SDL_DestroyWindow};
        std::unique_ptr<
                        SDL_Renderer,
                        void(*)(SDL_Renderer *)
                       > m_renderer {nullptr, SDL_DestroyRenderer};
        std::unique_ptr<
                        SDL_Texture,
                        void(*)(SDL_Texture *)
                       > m_screen {nullptr, SDL_DestroyTexture};
        
        err_code m_error = NO_ERROR;

        #define WINDOW   m_window.get()
        #define RENDERER m_renderer.get()
        #define SCREEN   m_screen.get()

        drawContext()  = default;
        ~drawContext() = default;

        err_code init() {
            SDL_Window *window = SDL_CreateWindow(
                "Window name",
                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                SCREEN_WIDTH, SCREEN_HEIGHT,
                SDL_WINDOW_SHOWN);
            if(!window) {
#ifdef DEBUG
                std::cout << "Coundn't create window with error " << SDL_GetError() << "\n"; 
#endif
                m_error = WIN_CREATE_FAIL;
                return WIN_CREATE_FAIL;
            }
#ifdef DEBUG
            std::cout << "SDL window created successfully.\n"; 
#endif
            m_window.reset(window);

            SDL_Renderer *renderer = SDL_CreateRenderer(
                window,
                -1,
                SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
            if(!renderer) {
#ifdef DEBUG
                std::cout << "Couldn't create renderer with error " << SDL_GetError() << "\n"; 
#endif
                m_error = RENDERER_CREATE_FAIL;
                return RENDERER_CREATE_FAIL;
            }
            m_renderer.reset(renderer);

            SDL_Texture *screen = SDL_CreateTexture(
                renderer, 
                REQUIRED_PIXEL_FORMAT, SDL_TEXTUREACCESS_STREAMING, 
                SCREEN_WIDTH, SCREEN_HEIGHT);
            if(!screen) {
#ifdef DEBUG
                std::cout << "Couldn't create screen texture with error " << SDL_GetError() << "\n"; 
#endif
                m_error = SCREEN_CREATION_FAIL;
                return SCREEN_CREATION_FAIL;
            }
            m_screen.reset(screen); 
            //m_screen_pixels = (uint32_t*)calloc(SCREEN_WIDTH*SCREEN_HEIGHT, sizeof(uint32_t));

            return NO_ERROR;
        };
        
        bool isValid(){ return m_error == NO_ERROR; };

        void update() { 
            //SDL_UpdateTexture(SCREEN, NULL, m_screen_pixels, SCREEN_WIDTH*4);
            SDL_RenderPresent(RENDERER); 
        };

        void clear()  { 
            SDL_SetRenderDrawColor(RENDERER, 0x80, 0x80, 0x80, 0xFF); 
            SDL_RenderClear(RENDERER); 
        };

        void lock() {
            int pitch;
            SDL_LockTexture(SCREEN, NULL, (void**)&m_screen_pixels, &pitch);
        }

        void unlock() {
            SDL_UnlockTexture(SCREEN);
            SDL_RenderCopy(RENDERER, SCREEN, NULL, NULL);
            m_screen_pixels = NULL;
        }

        void setPixel(int x, int y, int r, int g, int b, int a) {
            SDL_SetRenderDrawColor(RENDERER, r, g, b, a); 
            SDL_RenderDrawPoint(RENDERER, x, y);
        }

        void setPixel(int x, int y, uint32_t color) {
            m_screen_pixels[SCREEN_WIDTH*y+x] = color; 
        }

        SDL_Window*   win_ptr() { return WINDOW; }; 
        SDL_Renderer* ren_ptr() { return RENDERER; }; 
        SDL_Texture*  scr_ptr() { return SCREEN; }; 

        #undef WINDOW
        #undef RENDERER
        #undef SCREEN

  private:
        uint32_t *m_screen_pixels {nullptr};
};

#define INIT_DRAW_CONTEXT(name) drawContext dc{}; dc.init() 


#endif

#ifndef DRAWCONTEXT_SENTRY
#define DRAWCONTEXT_SENTRY


#include <SDL2/SDL.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <functional>

#include <errors.h>


using pos_t = int;

class drawContext {
    public:
        static const pos_t SCREEN_WIDTH  = 640;
        static const pos_t SCREEN_HEIGHT = 480;
        static const uint_fast32_t SCALE = 64;

        std::unique_ptr<
                        SDL_Window, 
                        std::function<void(SDL_Window *)>
                       > m_window {nullptr, SDL_DestroyWindow};
        //std::unique_ptr<SDL_Surface> m_screen {nullptr};
        std::unique_ptr<
                        SDL_Renderer,
                        std::function<void(SDL_Renderer *)>
                       > m_renderer {nullptr, SDL_DestroyRenderer};
        
        err_code m_error = NO_ERROR;

        #define WINDOW   m_window.get()
        #define RENDERER m_renderer.get()

        drawContext()  = default;
        ~drawContext() = default;

        err_code init() {
            SDL_Window *window = SDL_CreateWindow(
                                      "Window name",
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SCREEN_WIDTH,
                                      SCREEN_HEIGHT,
                                      SDL_WINDOW_SHOWN);
            if(!window) {
                std::cout << "Coundn't create window with error " << SDL_GetError() << "\n"; 
                m_error = WIN_CREATE_FAIL;
                return WIN_CREATE_FAIL;
            }
            std::cout << "SDL window created successfully.\n"; 
            m_window.reset(window);

            SDL_Renderer *renderer = SDL_CreateRenderer(
                                      window,
                                      -1,
                                      SDL_RENDERER_ACCELERATED);
            if(!renderer) {
                std::cout << "Couldn't create renderer with error " << SDL_GetError() << "\n"; 
                m_error = RENDERER_CREATE_FAIL;
                return RENDERER_CREATE_FAIL;
            }
            m_renderer.reset(renderer);
            /*
            m_screen.reset( SDL_GetWindowSurface(WINDOW) );
            SDL_FillRect(m_screen.get(), 
                         NULL, 
                         SDL_MapRGB(m_screen->format, 0xFF, 0xFF, 0xFF));
            */

            return NO_ERROR;
        };
        
        bool isValid(){ return m_error == NO_ERROR; };
        void update() { SDL_RenderPresent(RENDERER); };
        void clear()  { 
            SDL_SetRenderDrawColor(RENDERER, 0x80, 0x80, 0x80, 0xFF); 
            SDL_RenderClear(RENDERER); 
        };

        SDL_Window*   win_ptr() { return WINDOW; }; 
        SDL_Renderer* ren_ptr() { return RENDERER; }; 

        #undef WINDOW
        #undef RENDERER
};

#define INIT_DRAW_CONTEXT(name) drawContext dc{}; dc.init() 


#endif

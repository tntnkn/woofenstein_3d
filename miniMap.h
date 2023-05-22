#ifndef MINIMAP_SENTRY
#define MINIMAP_SENTRY


#include <SDL2/SDL.h>
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_render.h>
#include <cstdint>

#include "drawContext.h"
#include "things.h"


class miniMap {
    uint8_t m_alpha = 100;
   
  public:
    int m_scale = 32;
    miniMap() {};
    miniMap(int s) : m_scale(s) {};
    ~miniMap() {};

    void draw(Map &m, Player &p, drawContext &dc) {
        SDL_BlendMode cb;
        SDL_Renderer *rend = dc.ren_ptr();
        SDL_GetRenderDrawBlendMode(rend, &cb);
        SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
        draw(m, dc);
        draw(p, dc, m.w, m.h);
        SDL_SetRenderDrawBlendMode(rend, cb);
    }

    void draw(Map &m, drawContext &dc) {
        int h = m.h, w = m.w;
        for(int i = 0; i < h; ++i) {
            for(int j = 0; j < w; ++j) {
                char c = m.getTile(j, i);
                SDL_Renderer *rend = dc.ren_ptr();
                SDL_Rect t = { 
                    j*m_scale, (h-i-1)*m_scale, m_scale, m_scale,
                };
                switch(c) {
                    case(FLOOR): {
                        SDL_SetRenderDrawColor(rend, 0x00, 0x00, 0x00, m_alpha); 
                        break;
                    }
                                 /*
                    case(WALL): {
                        SDL_SetRenderDrawColor(rend, 0xFF, 0xF2, 0x00, m_alpha); 
                        break;
                    }
                                */
                    default: {
                        SDL_SetRenderDrawColor(rend, 0xFF, 0xFF, 0xFF, m_alpha); 
                        break;
                    }
                }
                SDL_RenderFillRect(rend, &t);
                SDL_SetRenderDrawColor(rend, 0x80, 0x80, 0x80, m_alpha); 
                SDL_RenderDrawRect(rend, &t);
            }
        }
    }

    void draw(Player &p, drawContext &dc, int mw, int mh) {
        SDL_Renderer *rend = dc.ren_ptr();
        /* 
         * Entity's y is inverted as SDL's origin is top left.
         */
        int xi = (int)p.x, yi = (int)p.y;
        SDL_Rect pr = { 
            (int)((p.x-p.w/2)*m_scale), 
            (int)((mh-p.y-p.h/2)*m_scale), 
            (int)(p.w*m_scale), 
            (int)(p.h*m_scale),
        };
        SDL_SetRenderDrawColor(rend, 0xFF, 0xF2, 0x00, 0xFF); 
        SDL_RenderFillRect(rend, &pr);
        
        float lx = p.x+cos(p.a)*p.w;
        float ly = (mh-p.y)-sin(p.a)*p.h;
        float lw = p.w/2, lh = p.h/2;
        SDL_Rect lr = { 
            (int)((lx-lw/2)*m_scale), 
            (int)((ly-lh/2)*m_scale), 
            (int)(lw*m_scale), 
            (int)(lh*m_scale),
        };
        SDL_SetRenderDrawColor(rend, 0xFF, 0x00, 0x00, 0xFF); 
        SDL_RenderFillRect(rend, &lr);
    }

    void drawLine(float x1, float y1, float x2, float y2, 
                  int R, int G, int B, Map &m, drawContext &dc) {
        SDL_Renderer *rend = dc.ren_ptr();
        SDL_SetRenderDrawColor(rend, R, G, B, m_alpha); 
        SDL_RenderDrawLine(rend, 
                           x1      *m_scale,
                           (m.h-y1)*m_scale,
                           x2      *m_scale, 
                           (m.h-y2)*m_scale);
    }
};


#endif

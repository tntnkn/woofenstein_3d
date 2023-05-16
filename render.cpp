#include <SDL2/SDL.h>

#include "render.h"
#include "things.h"
#include "miniMap.h"
#include "linal.h"


enum WALL_HIT {
    WH_NONE, WH_HORIZONTAL, WH_VERTICAL,
};

void
draw(scene &sc, drawContext &dc)
{
    Player &p = sc.p;
    Map &m = sc.m;
    miniMap &mm = sc.mm;

    float fov = 1.15192; // 66 degrees
    float h_fov_tan = tan(fov/2);
#ifdef FAST_DDA
    // pdirl should be 1 as computation of perpDist below relies on it.
    float pdirl = 1;
#else
    // pdirl can be any number as perpDist is computed via dot product.
    float pdirl = 1;
#endif
    float pdirx = pdirl*cos(p.a), pdiry = pdirl*sin(p.a);
    // Camera
    float cdirl = h_fov_tan * pdirl;
    float cdirx = -pdiry/pdirl*cdirl, cdiry = pdirx/pdirl*cdirl;

    SDL_Renderer *rend = dc.ren_ptr();

#ifdef DEBUG
    mm.drawLine(p.x, p.y, p.x+pdirx, p.y+pdiry, 0x00, 0xFF, 0x00, m, dc); 
    mm.drawLine(p.x, p.y, p.x+cdirx, p.y+cdiry, 0xFF, 0x00, 0x00, m, dc); 
#endif

    for(int i = 0; i < dc.SCREEN_WIDTH; i++) {
        // Cofficient for camera vector, from -1 to 1.
        float cc = 2.0*(float)i/(float)dc.SCREEN_WIDTH - 1.0; 
        float rdirx = pdirx+cdirx*cc;
        float rdiry = pdiry+cdiry*cc;

        // The main question is how much x and y contribute to ray length.
        // So below is change in len of ray for each 1 unit change in x or y.
#ifdef FAST_DDA
        // In fact, instead of computing exactly the length of ray for one
        // unit of x or y, we can do the same with just the ratios.
        float rxtl_ratio = rdirx == 0 ? 1e30 : std::abs(1 / rdirx);
        float rytl_ratio = rdiry == 0 ? 1e30 : std::abs(1 / rdiry);
#else
        /*
        float rxtl_ratio = std::sqrt(1+(rdiry*rdiry)/(rdirx*rdirx));
        float rytl_ratio = std::sqrt(1+(rdirx*rdirx)/(rdiry*rdiry));
        */
        float rdirl = std::sqrt( dot(rdirx, rdiry, rdirx, rdiry) );
        float rxtl_ratio = rdirx == 0 ? 1e30 : std::abs(rdirl / rdirx);
        float rytl_ratio = rdiry == 0 ? 1e30 : std::abs(rdirl / rdiry);
#endif

        // And this is a ray length from start to next hit x or y.
        float rdirlx = 0;
        float rdirly = 0;
        // Coordinates and step size in map space.
        int gridx = (int)p.x;
        int gridy = (int)p.y;
        int gridstepx = 0;
        int gridstepy = 0;

        WALL_HIT wh = WH_NONE;

        int maxgridl = 100;
        int curmaxgridl = 0;
        float offx = 0;
        float offy = 0;
        // Calculate initial conditions.
        if(rdirx < 0) {
            gridstepx = -1;
            offx = rdirlx = (p.x - gridx) * rxtl_ratio;
        } else {
            gridstepx =  1;
            offx = rdirlx = ((gridx+1.0) - p.x) * rxtl_ratio;
        }
        if(rdiry < 0) {
            gridstepy = -1;
            offy = rdirly = (p.y - gridy) * rytl_ratio;
        } else {
            gridstepy =  1;
            offy = rdirly = ((gridy+1.0) - p.y) * rytl_ratio;
        }

        do {
            if(rdirlx < rdirly) {
                rdirlx += rxtl_ratio;
                gridx  += gridstepx;
                wh = WH_VERTICAL;
                curmaxgridl = std::abs(gridx-p.x);
            } else {
                rdirly += rytl_ratio;
                gridy  += gridstepy;
                wh = WH_HORIZONTAL;
                curmaxgridl = std::abs(gridy-p.y);
            }
        } while (m.getTile(gridx, gridy) != WALL
              && curmaxgridl < maxgridl);

        float perpDist = 0;
        SDL_SetRenderDrawColor(rend, 0x00, 0x00, 0xFF, 128); 
        switch(wh) {
            case(WH_HORIZONTAL): {
#ifdef FAST_DDA
                perpDist = rdirly - rytl_ratio;
                float rdirl = 1;
#else
                float d = dot(pdirx, pdiry, rdirx, rdiry); 
                perpDist = std::abs( (d/(pdirl*rdirl)) * (rdirly - rytl_ratio));
#endif
#ifdef DEBUG
                mm.drawLine(p.x, p.y, 
                            p.x+rdirx/rdirl*(rdirly-rytl_ratio), p.y+rdiry/rdirl*(rdirly-rytl_ratio), 
                            0x00, 0xFF, 0xFF, m, dc); 
#endif
            } break;
            case(WH_VERTICAL): {
#ifdef FAST_DDA
                perpDist = rdirlx - rxtl_ratio;
                float rdirl = 1;
#else
                float d = dot(pdirx, pdiry, rdirx, rdiry); 
                perpDist = std::abs( (d/(pdirl*rdirl)) * (rdirlx - rxtl_ratio));
#endif
#ifdef DEBUG
                mm.drawLine(p.x, p.y, 
                            p.x+rdirx/rdirl*(rdirlx-rxtl_ratio), p.y+rdiry/rdirl*(rdirlx-rxtl_ratio), 
                            0xFF, 0xFF, 0xFF, m, dc); 
#endif
            } break;
            case(WH_NONE):
            default:
                break;
        };

        int line_h = dc.SCREEN_HEIGHT / perpDist;
        int line_b = dc.SCREEN_HEIGHT/2 - line_h/2;
        if(line_b < 0) line_b = 0;
        int line_t = dc.SCREEN_HEIGHT/2 + line_h/2;
        if(line_t >= dc.SCREEN_HEIGHT) line_t = dc.SCREEN_HEIGHT-1;

        SDL_SetRenderDrawColor(rend, 0x00, 0x00, 0xFF, 128); 
        SDL_RenderDrawLine(rend, dc.SCREEN_WIDTH-i, line_b, dc.SCREEN_WIDTH-i, line_t);
    }
}

#include <SDL2/SDL.h>
#include <cstdint>
#include <cstdlib>

#include "render.h"
#include "things.h"
#include "miniMap.h"
#include "linal.h"

#include <cassert>


enum WALL_HIT {
    WH_NONE, WH_HORIZONTAL, WH_VERTICAL,
};

void
draw(scene &sc, drawContext &dc, tileMap &tm)
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
        // Calculate initial conditions.
        if(rdirx < 0) {
            gridstepx = -1;
            rdirlx = (p.x - gridx) * rxtl_ratio;
        } else {
            gridstepx =  1;
            rdirlx = ((gridx+1.0) - p.x) * rxtl_ratio;
        }
        if(rdiry < 0) {
            gridstepy = -1;
            rdirly = (p.y - gridy) * rytl_ratio;
        } else {
            gridstepy =  1;
            rdirly = ((gridy+1.0) - p.y) * rytl_ratio;
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
        } while (!m.isWall(gridx, gridy) && curmaxgridl < maxgridl);

        float perpDist = 0;
        float whc_n = 0; //wall hit coordinate normalized

        switch(wh) {
            case(WH_HORIZONTAL): {
#ifdef FAST_DDA
                perpDist = rdirly - rytl_ratio;
#else
                //float d = dot(pdirx, pdiry, rdirx, rdiry); 
                //perpDist = std::abs( (d/(pdirl*rdirl)) * (rdirly - rytl_ratio));
                // cos(rdir, pdir)*rdirl == 1! rdir always projects at pdir
                // as it is computed using it and it's perp.
                perpDist = (rdirly - rytl_ratio) / rdirl;
#endif
                whc_n = p.x + rdirx * perpDist;
            } break;
            case(WH_VERTICAL): {
#ifdef FAST_DDA
                perpDist = rdirlx - rxtl_ratio;
#else
                //float d = dot(pdirx, pdiry, rdirx, rdiry); 
                //perpDist = std::abs( (d/(pdirl*rdirl)) * (rdirlx - rxtl_ratio));
                // cos(rdir, pdir)*rdirl == 1! rdir always projects at pdir
                // as it is computed using it and it's perp.
                perpDist = (rdirlx - rxtl_ratio) / rdirl;
#endif
                whc_n = p.y + rdiry * perpDist;
            } break;
            case(WH_NONE):
            default:
                break;
        };
        whc_n -= std::floor(whc_n); //TODO test just casting into int

#ifdef DEBUG
        int c = wh == WH_VERTICAL ? 0x00 : 0xFF;
        // Normalization factor to rdirx and rdiry is included in perpDist!
        mm.drawLine(p.x, p.y, 
                    p.x+rdirx*perpDist, p.y+rdiry*perpDist, 
                    c, 0xFF, 0xFF, m, dc); 
#endif

        int line_h = dc.SCREEN_HEIGHT / perpDist;
        //if(perpDist < 1.0)
            //std::cout << perpDist << std::endl;
        int line_b = dc.SCREEN_HEIGHT/2 - line_h/2;
        /* The problem -- when perpDist < 1 the line_h > SCREEN_HEIGHT.
         * So resulting line_b and line_t may be beyond screen.
         * If they are clamped now, then textures will be clamped too,
         * resulting in artifacts when standing close to walls.
         * Better to leave it like this and handle corner cases later. */
        //if(line_b < 0) line_b = 0;
        int line_t = dc.SCREEN_HEIGHT/2 + line_h/2;
        //if(line_t >= dc.SCREEN_HEIGHT) line_t = dc.SCREEN_HEIGHT-1;

#ifndef NO_RENDER_TEX
        int tx = 0;
        int ty = 0;
        int tw = 64;
        int th = 64;
        int wall_t = m.getWallOrder(gridx, gridy);
        int mask = th - 1;

        tx = (whc_n * (float)tw);
        // In below cases ray approaches tile from its top.
        if(wh == WH_VERTICAL   && rdirx > 0) tx = tw-1-tx;
        else
        if(wh == WH_HORIZONTAL && rdiry < 0) tx = tw-1-tx;;
       
        /* It uses the abridged version of Bresenham's integer line algorithm.
         * I presume here that th will never be >= line_h. */
       
        int m = th; // rise / run * run, see below
        int y_inc = m >= 0 ? 1 : -1;
        int accum = 0;
        int d = std::abs(m) * 2;         //slope * 2 * run
        int threshold = line_h;          //0.5   * 2 * run
        int thres_inc = 2 * line_h;      //1.0   * 2 * run

        struct colorRBG rgb;
        for(int y = line_b; y < line_t; ++y) {
            //uint32_t c = textures[wall_t][tw * ty + tx];
            rgb = tm.getColorRGB(wall_t, tx, ty);
            SDL_SetRenderDrawColor(
                rend, rgb.r, rgb.g, rgb.b, 255); 
            SDL_RenderDrawPoint(rend, dc.SCREEN_WIDTH-i, dc.SCREEN_HEIGHT-y);
            accum += d;
            if(accum >= threshold) {
                ty += y_inc;
                ty &= mask;
                threshold += thres_inc;
            }
        }
#else
        SDL_SetRenderDrawColor(rend, 0x00, 0x00, 0xFF, 255); 
        SDL_RenderDrawLine(rend, dc.SCREEN_WIDTH-i, line_b, dc.SCREEN_WIDTH-i, line_t);
#endif
    }
}

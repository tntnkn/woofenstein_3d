#include <SDL2/SDL.h>
#include <cstdint>
#include <cstdlib>
#include <algorithm>

#include "render.h"
#include "things.h"
#include "miniMap.h"
#include "linal.h"
#include "tileMap.h"
#include "guard.h"


enum WALL_HIT {
    WH_NONE, WH_HORIZONTAL, WH_VERTICAL,
};

void
draw(scene &sc, drawContext &dc, tileMap &tm, drawBuffers buff)
{
    dc.lock();
    auto unlocker = [&dc](){ dc.unlock(); };
    auto unlock_guard = make_simple_guard(unlocker);

    Thing   &p      = sc.p;
    Things  &things = sc.things;
    Map     &map    = sc.m;
    miniMap &mm     = sc.mm;

    auto z_buffer   = buff.z;
    auto things_dst = buff.things_dst; 
    auto things_ids = buff.things_ids; 

    float fov       = 1.15192; // 66 degrees
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

#ifdef DEBUG
    mm.drawLine(p.x, p.y, p.x+pdirx, p.y+pdiry, 0x00, 0xFF, 0x00, map, dc); 
    mm.drawLine(p.x, p.y, p.x+cdirx, p.y+cdiry, 0xFF, 0x00, 0x00, map, dc); 
#endif

    // Walls, floor, ceiling.
    for(int i = 0; i < dc.SCREEN_WIDTH; i++) {
        // Cofficient for camera vector, from 1 to -1.
        // (cdir is 90d to the left of pdir).
        float cc = -(2.0*(float)i/(float)dc.SCREEN_WIDTH - 1.0); 
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
        } while (!map.isWall(gridx, gridy) && curmaxgridl < maxgridl);

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
        z_buffer[i] = perpDist;
        whc_n -= std::floor(whc_n); //TODO test just casting into int

        /* The problem of perpDist being < 1 and the line_h > SCREEN_HEIGHT
         * is handled further below. */
        int line_h = dc.SCREEN_HEIGHT / perpDist;
        int line_b = dc.SCREEN_HEIGHT/2 - line_h/2;
        int line_t = dc.SCREEN_HEIGHT/2 + line_h/2;
#ifdef DEBUG
        if(perpDist < 1.0)
            std::cout << "perpDist < 1.0 " << perpDist << std::endl;
#endif

#ifndef NO_RENDER_TEX
        // Walls
        int tx = 0;
        int ty = 0;
        int tw = tm.m_tw;
        int th = tm.m_th;
        int wall_t = map.getWall(gridx, gridy);
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

        int line_start = line_b;
        int line_end   = line_t;
        if(line_start < 0) { 
            for(int i = 0; i < std::abs(line_start); ++i) {
                accum += d;
                if(accum >= threshold) {
                    ty += y_inc;
                    ty &= mask;
                    threshold += thres_inc;
                }
            }
            line_start = 0;
        } 
        if(line_end > dc.SCREEN_HEIGHT) {
            line_end = dc.SCREEN_HEIGHT;
        }
        for(int y = line_start; y < line_end; ++y) {
            /*
            rgb = tm.getColorRGB(wall_t, tx, ty);
            dc.setPixel(dc.SCREEN_WIDTH-i, dc.SCREEN_HEIGHT-y,
                        rgb.r, rgb.g, rgb.b, 255); 
            */
            dc.setPixel(i, dc.SCREEN_HEIGHT-1-y,
                        tm.getColor(wall_t, tx, ty) );
            accum += d;
            if(accum >= threshold) {
                ty += y_inc;
                ty &= mask;
                threshold += thres_inc;
            }
        }

        /* Floor and ceiling. 
         * At the same time as bigZ is in the middle of the screen and 
         * they are symmetrical. */
        float bigZ = (float)dc.SCREEN_HEIGHT / 2;
        for(int y = 0; y < line_start; ++y) {
            float smallZ  = bigZ - y;     

#ifdef FAST_DDA
            /* pdirl/row_dist = smallZ/bigZ */
            // Here pdirl == 1 in every case.
            float row_dist = bigZ/smallZ;  
#else
            /* pdirl is included as normalizing coefficient for rdirl that is 
             * used later, not as a part of the ratio formula! */
            float row_dist = bigZ/smallZ/pdirl; 
#endif

            /* Imagine a right triangle with pdir and rdir as sides. 
             * Both hit the imaginary screen while rdir also goes though
             * pixel being colored (well, its projection to the ground).
             * If all the sides are multiplied by row_dist, then 
             * the resulting triangle is similar to original one and 
             * resulting ray is hitting the floor/wall in a correct sample spot 
             */
            float f_tilex = p.x + rdirx * row_dist;
            float f_tiley = p.y + rdiry * row_dist; 

            int tile_x = f_tilex;
            int tile_y = f_tiley;
            
            int tx = (int)(tw * (f_tilex - tile_x) ) & (tw-1);
            int ty = (int)(th * (f_tiley - tile_y) ) & (th-1); 

            int floor_t = map.getFloor(tile_x, tile_y);
            /*
            rgb = tm.getColorRGB(floor_t, tx, ty);
            dc.setPixel(dc.SCREEN_WIDTH-i, dc.SCREEN_HEIGHT-y,
                        rgb.r, rgb.g, rgb.b, 255); 
            */
            dc.setPixel(i, dc.SCREEN_HEIGHT-y-1,
                        tm.getColor(floor_t, tx, ty) );

            int ceil_t  = map.getCeil(tile_x, tile_y);
            /*
            rgb = tm.getColorRGB(ceil_t, tx, ty);
            dc.setPixel(dc.SCREEN_WIDTH-i, y,
                        rgb.r, rgb.g, rgb.b, 255); 
            */
            dc.setPixel(i, y,
                        tm.getColor(ceil_t, tx, ty) );
        }

#else
        SDL_Renderer *rend = dc.ren_ptr();
        SDL_SetRenderDrawColor(rend, 0x00, 0x00, 0xFF, 255); 
        SDL_RenderDrawLine(rend, dc.SCREEN_WIDTH-i, line_b, dc.SCREEN_WIDTH-i, line_t);
#endif


#ifdef DEBUG
        int mm_ray_r = wh == WH_VERTICAL ? 0x00 : 0xFF;
        // Normalization factor to rdirx and rdiry is included in perpDist!
        mm.drawLine(p.x, p.y, 
                    p.x+rdirx*perpDist, p.y+rdiry*perpDist, 
                    mm_ray_r, 0xFF, 0xFF, map, dc); 
#endif

    } //end drawing walls, floor, ceiling.

    // Sprites
    size_t th_size = things.size();
    float tmpX = 0;
    float tmpY = 0;
    // Buffers's sizes are kept in sync with no of things by the app.
    for(size_t i = 0; i < th_size; ++i) {
        tmpX = p.x - things[i].x;
        tmpY = p.y - things[i].y;
        things_dst[i] = dot(tmpX, tmpY, tmpX, tmpY); //norm would do too.
        things_ids[i] = i;
    }
    std::sort(
        things_ids.begin(), 
        //size is always == capacity.
        things_ids.begin() + th_size,        
        [&](int a, int b) { return std::isgreater(things_dst[a], things_dst[b]); } 
    );

    float inv_det = 1.0 / (cdirx * pdiry - pdirx * cdiry);
    for(int i = 0; i < th_size; ++i) {
        Thing &thing = things[ things_ids[i] ];
        // calculating thing position relative to [cdir, pdir] space.
        float tmp_x = thing.x - p.x;
        float tmp_y = thing.y - p.y;
        float th_x = inv_det * (tmp_x * pdiry - tmp_y * pdirx); 
        float th_y = inv_det * (tmp_y * cdirx - tmp_x * cdiry); 
        float a_th_y = std::abs(th_y);

        if(th_y < 0)
            continue;

        int th_h = dc.SCREEN_HEIGHT / a_th_y;
        int th_w = th_h; // because it's a square!

        // If th_x/th_y > 1 -> thing's center is beyond FOV.
        // Division also projects.
        int thing_center_screen_x = dc.SCREEN_WIDTH / 2 - 
            ((dc.SCREEN_WIDTH/2) * th_x/a_th_y);
        int thing_center_screen_y = dc.SCREEN_HEIGHT / 2; 
   
        int hor_off = 0;
        int hor_start = thing_center_screen_x - th_w/2; 
        if(hor_start < 0) { hor_off = -hor_start; hor_start = 0; }
        int hor_end   = thing_center_screen_x + th_w/2;
        if(hor_end >= dc.SCREEN_WIDTH) hor_end = dc.SCREEN_WIDTH - 1;

        int ver_off = 0;
        int ver_start = thing_center_screen_y - th_h/2;
        if(ver_start < 0) { ver_off = -ver_start; ver_start = 0; }
        int ver_end   = thing_center_screen_y + th_h/2;
        if(ver_end >= dc.SCREEN_HEIGHT) ver_end = dc.SCREEN_HEIGHT - 1;

        auto sprite = thing.sprite;
        int th = sprite->m_th;
        int tw = sprite->m_tw;

        /* This algo seems to be slightly more performant than Bresenham's */
        int hor_off_ratio = hor_off * tw / th_w;
        int ver_off_ratio = ver_off * tw / th_w;
        for(int row = hor_start; row < hor_end; ++row) {
            if( th_y >= z_buffer[row] )
                continue;
            // roses are red, float math is bad.
            int tx = int(256*((row-hor_start) * tw/th_w))/256 + hor_off_ratio;
            for(int col = ver_start; col < ver_end; ++col) {
                int ty = int(256*(col-ver_start) * th/th_h)/256 + ver_off_ratio;
                dc.setPixel( row, col, sprite->getColor(0, tx, ty) );
            }
        }

        /*
        int tx = 0;
        int ty = 0;
        int mask = th - 1;
        int m = th; // rise / run * run, see below
        int y_inc = m >= 0 ? 1 : -1;
        int accum = 0;
        int d = std::abs(m) * 2;       //slope * 2 * run
        int threshold = th_h;          //0.5   * 2 * run
        int thres_inc = 2 * th_h;      //1.0   * 2 * run

        if(ver_start < 0) { 
            for(int i = 0; i < std::abs(ver_start); ++i) {
                accum += d;
                if(accum >= threshold) {
                    ty += y_inc;
                    ty &= mask;
                    threshold += thres_inc;
                }
            }
            ver_start = 0;
        } 
        if(ver_end > dc.SCREEN_HEIGHT) {
            ver_end = dc.SCREEN_HEIGHT;
        }
        for(int y = ver_start; y < ver_end; ++y) {
            for(int x = hor_start; x < hor_end; ++x) {
                if(x < 0 || x >= dc.SCREEN_WIDTH || th_y > z_buffer[x])
                    continue;
                int tx = int(256*(x-hor_start) * tw/th_w)/256;
                dc.setPixel(x, y, sprite->getColor(0, tx, ty) );
            }
            accum += d;
            if(accum >= threshold) {
                ty += y_inc;
                ty &= mask;
                threshold += thres_inc;
            }
        }
        */
    }
}

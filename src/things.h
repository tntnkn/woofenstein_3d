#ifndef THNINGS_SENTRY
#define THNINGS_SENTRY


#include <SDL2/SDL.h>
#include <cstdint>
#include <memory>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cctype>

#include "drawContext.h"
#include "errors.h"
#include "pi.h"



struct boundBox {
    float tlx;
    float tly;
    float brx;
    float bry;
};


enum MAP_ERRORS : signed char {
    OUT_OF_BOUNDS = -1,
};

enum COLLISIONS : char {
    FLOOR         = 0,
    WALL          = 1,
};


class Map {
    using REPR_PTR = std::unique_ptr<char[], void(*)(void*)>;

  public:
    int w = 0; 
    int h = 0;

    Map() {};
    Map(const char *path) { load(path); };

    int load(const char *path) {
        int ret = 0;
        std::string str(path); 
        if(ret = __load( m_walls, str + std::string("/walls.txt") ) )
            return ret;
        if(ret = __load( m_floor, str + std::string("/floor.txt") ) )
            return ret;
        if(ret = __load( m_ceil , str + std::string("/ceil.txt" ) ) )
            return ret;
        if(ret = __load( m_coll , str + std::string("/coll.txt" ) ) )
            return ret;
        //char *str = (char*)calloc( strlen(path) + 6, 1);
        //strcpy(str, path);
        /*
        if(ret = __load( m_walls, strcat(str, "/walls.txt") ) )
            return ret;
        if(ret = __load( m_floor, strcat(str, "/floor.txt") ) )
            return ret;
        if(ret = __load( m_ceil , strcat(str, "/ceil.txt") )  )
            return ret;
        if(ret = __load( m_coll , strcat(str, "/coll.txt") )  )
            return ret;
        */
        return ret;
    };
    
    bool isLoaded() const { 
        return 
            m_walls != nullptr &&
            m_floor != nullptr && 
            m_ceil  != nullptr && 
            m_coll  != nullptr; 
    };

    template<typename T>
    char getCollision(T x, T y) const { //xy with origin in BOT LEFT
        translateXY(x, y);
        if( !_isWithin(x, y) )
            return OUT_OF_BOUNDS;
        return _getTile(m_coll, x, y);
    };

    template<typename T>
    char getWall(T x, T y) const { //xy with origin in BOT LEFT
        translateXY(x, y);
        if( !_isWithin(x, y) )
            return OUT_OF_BOUNDS;
        return _getTile(m_walls, x, y);
    };

    template<typename T>
    char getFloor(T x, T y) const { //xy with origin in BOT LEFT
        translateXY(x, y);
        if( !_isWithin(x, y) )
            return OUT_OF_BOUNDS;
        return _getTile(m_floor, x, y);
    };

    template<typename T>
    char getCeil(T x, T y) const { //xy with origin in BOT LEFT
        translateXY(x, y);
        if( !_isWithin(x, y) )
            return OUT_OF_BOUNDS;
        return _getTile(m_ceil, x, y);
    };

    template<typename T>
    bool isWall(T x, T y) const {
        char t = getCollision(x, y);
        return t == WALL;
    }

    bool isWithin(boundBox &bbx) const {
        boundBox nbbx = bbx; 
        translateXY(nbbx.tlx, nbbx.tly);
        return _isWithin(nbbx);
    };

    template<typename T>
    bool isWithin(T x, T y) const {
        translateXY(x, y);
        return _isWithin(x, y);
    };

    bool canMoveTo(float x, float y, boundBox &bbx) const {
        boundBox nbbx = bbx; 
        translateXY(x, y);
        translateXY(nbbx.tlx, nbbx.tly);
        translateXY(nbbx.brx, nbbx.bry);
        return _canMoveTo(x, y, nbbx);
    };

  private:
    /*
     * Player's y is adjusted as map is essentially stored upside down.
     */
    void translateXY(int   &x, int   &y) const { y = h-y-1; };
    void translateXY(float &x, float &y) const { y = (float)h-y; };

    char _getTile(const REPR_PTR &repr, int x, int y) const {
        return repr[w*y+x];
    };

    bool _isWithin(boundBox &bbx) const {
        if( (int)(bbx.brx) >= w || (int)(bbx.bry) >= h  
         || (int)(bbx.tlx) < 0  || (int)(bbx.tly) < 0 
        ){
            return false;
        }
        return true;
    };
    
    bool _isWithin(int x, int y) const {
        if( x >= w || y >= h  
         || x < 0  || y < 0 
        ){
            return false;
        }
        return true;
    };

    bool _canMoveTo(float x, float y, boundBox &bbx) const {
#ifdef DEBUG
        std::cout << "testing coordinates " << x << " " << y << std::endl;
#endif
        //adjustXY(&x, &y);
        if( !_isWithin(bbx) )
            return false;
#ifdef DEBUG
        std::cout << bbx.tlx << " " << bbx.tly << " "
                  << bbx.brx << " " << bbx.bry << std::endl;
#endif
        if( _getTile(m_walls, bbx.brx, bbx.bry) != FLOOR
         || _getTile(m_walls, bbx.brx, bbx.tly) != FLOOR
         || _getTile(m_walls, bbx.tlx, bbx.bry) != FLOOR
         || _getTile(m_walls, bbx.tlx, bbx.tly) != FLOOR
        )
            return false;
#ifdef DEBUG
        std::cout << "can move to " << x << " " << y << std::endl;
#endif
        return true;
    };
    
    void adjustXY(float *xp, float *yp) const {
        float c = 0.001;
        float x = *xp, y = *yp;
        float testx = x + c, testy = y + c;
        if( (int)x != (int)testx )
            *xp = testx;
        if( (int)y != (int)testy )
            *yp = testy;
    };

    int __load(REPR_PTR &repr, std::string path) {
        std::ifstream f_map(path);
        if( !f_map.good() ) {
#ifdef DEBUG
            std::cout << "Cannot open map in " << path << std::endl;
#endif
            return MAP_FILE_NOT_OPENED;
        }
        int map_w, map_h;
        f_map >> map_w >> map_h;
        int wh = map_w * map_h;
        char *r = reinterpret_cast<char *>( calloc(wh+1, 1) );
        int i = 0;
        while( f_map && i < wh ) {
            char c; f_map >> c;
            r[i++] = c - 48; //ascii
        }

        if(i != wh || ( (w!=0 && map_w!=w) || (h!=0 && map_h!=h) )) {
#ifdef DEBUG
            std::cout << "Wrong dimensions of map " << path << std::endl;
#endif
            return MAP_WRONG_DIMENSIONS;
        }
        repr.reset(r); 
        w = map_w; h = map_h;
        return 0;
    };

    REPR_PTR m_walls { nullptr, free } ;
    REPR_PTR m_floor { nullptr, free } ;
    REPR_PTR m_ceil  { nullptr, free } ;
    REPR_PTR m_coll  { nullptr, free } ;
};


class Player {
  public:
    //x and y of player's center 
    float x;
    float y;
    float v  = 0.1;
    float w  = 0.2;
    float h  = 0.2;
    float a  = PI/2;
    
    Player(float x, float y) : x(x), y(y) {}; 
    ~Player() {};

    void handle(SDL_Keycode k_code, Map &map) {
        float newx = x;
        float newy = y;
        float twopi= 2*PI;
        bool  moving=false;

        switch(k_code) {
            case(SDLK_w): {
                newy = y + sin(a)*v;
                newx = x + cos(a)*v;
                moving = true;
            } break;
            case(SDLK_s): {
                newy = y - sin(a)*v;
                newx = x - cos(a)*v;
                moving = true;
            } break;
            case(SDLK_a): {
                a += 0.1; a = a-twopi>0.001 ? 0 : a; 
            } break;
            case(SDLK_d): {
                a -= 0.1; a = a < 0.0 ? twopi : a; 
            } break;
            default:
                return;
        }
        
        if(!moving)
            return;

        float halfw= w/2;
        float halfh= h/2;
        struct boundBox bbx = {
            newx-halfw,
            newy-halfh,
            newx+halfw, 
            newy+halfh,
        };
        if( map.canMoveTo(newx, newy, bbx) ) {
            x = newx; y = newy;
        }
    }
};


#endif

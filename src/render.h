#ifndef RENDER_SENTRY
#define RENDER_SENTRY


#include "scene.h"
#include "drawContext.h"
#include "tileMap.h"


struct drawBuffers {
    float              *z; 
    std::vector<float> &things_dst;
    std::vector<int>   &things_ids;
};


void
draw(scene &sc, drawContext &dc, tileMap &tm, drawBuffers buff);


#endif

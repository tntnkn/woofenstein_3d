#ifndef SCENE_SENTRY
#define SCENE_SENTRY


#include "things.h"
#include "miniMap.h"


struct scene {
    Map     &m;
    Player  &p;
    miniMap &mm;
};


#endif

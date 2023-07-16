#ifndef SCENE_SENTRY
#define SCENE_SENTRY


#include "things.h"
#include "miniMap.h"


struct scene {
    Map     &m;
    Thing   &p;
    Things  &things;
    miniMap &mm;
};


#endif

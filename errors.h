#ifndef ERRORS_SENTRY
#define ERRORS_SENTRY


using err_code = int;
enum ERRORS : err_code {
    NO_ERROR,
    INIT_FAIL,
    WIN_CREATE_FAIL,
    RENDERER_CREATE_FAIL,
    EXIT_HANDLER_REG_FAIL,

    MAP_NOT_LOADED,
    MAP_FILE_NOT_OPENED,
    MAP_WRONG_DIMENSIONS,
};


#endif

#!/bin/bash


LINK_FLAGS='-lSDL2 -lSDL2_image'
DEFINE_FLAGS='-DDEBUG -DFAST_DDA'
g++ -I./ $LINK_FLAGS main.cpp initSDL.cpp render.cpp -o main $DEFINE_FLAGS

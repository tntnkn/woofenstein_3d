#!/bin/bash

g++ -I./ -lSDL2 main.cpp initSDL.cpp render.cpp -o main -DDEBUG -DFAST_DDA

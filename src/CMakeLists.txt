add_library(game OBJECT
    main.cpp
    initSDL.cpp
    render.cpp
)

add_library(WOOF::GAME ALIAS game)

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

target_include_directories(game
  PRIVATE 
    .
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2_IMAGE_INCLUDE_DIRS}
)

target_link_libraries(game
  PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
)

target_compile_definitions(game 
  PRIVATE 
    ASSETS_PATH="${ASSETS_PATH}"
)

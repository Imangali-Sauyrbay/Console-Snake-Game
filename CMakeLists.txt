cmake_minimum_required(VERSION 3.0)

project(ConsoleSnakeGame)

set(SOURCES
    main.c
    audio.c
    path.c
)

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

add_executable(main main.c audio.c path.c)

target_include_directories(main PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS})

target_link_libraries(main PRIVATE ncurses SDL2::SDL2 SDL2_mixer pthread)

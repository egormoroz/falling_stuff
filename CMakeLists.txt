cmake_minimum_required(VERSION 3.12)
#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "") # works

set(LIBS_FOLDER "C:/libs/SFML-2.5.1/lib/")
set(EXTRA_INCLUDES "C:/libs/SFML-2.5.1/include")

set(DBG_LIBS sfml-main-d.lib sfml-graphics-d.lib sfml-window-d.lib sfml-system-d.lib)
set(RLS_LIBS sfml-main.lib sfml-graphics.lib sfml-window.lib sfml-system.lib)

list(TRANSFORM DBG_LIBS PREPEND ${LIBS_FOLDER})
list(TRANSFORM RLS_LIBS PREPEND ${LIBS_FOLDER})

project(falling_stuff)

add_executable(falling_stuff main.cpp render_buffer.cpp simulation.cpp util.cpp fps_tracker.cpp grid_painter.cpp)

target_include_directories(falling_stuff PUBLIC
    "${PROJECT_BINARY_DIR}"
    ${EXTRA_INCLUDES})

target_link_libraries(falling_stuff debug ${DBG_LIBS})
# target_link_libraries(falling_stuff optimized ${RLS_LIBS})


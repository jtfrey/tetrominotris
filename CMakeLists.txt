#
# CMake build configuration for tetrominotris
#
cmake_minimum_required(VERSION 3.20)

project(tetrominotris
        VERSION 1.0.0
        DESCRIPTION "A blatant ripoff of Tetris"
        LANGUAGES C
    )

option(TBOARD_DEBUG "Enable debug printing in TBoard code" ON)

#
# Locate the curses tui library:
#
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)

# The standard CMake curses module does not locate the
# menu library.  Find the header file and add its path
# to CURSES_INCLUDE_DIRS if not present:
find_path(CURSES_MENU_INCLUDE_PATH
        NAMES ncursesw/menu.h ncursesw/menu.h menu.h
        PATHS ${CURSES_INCLUDE_DIRS}
        NO_DEFAULT_PATH
    )
if ( CURSES_MENU_INCLUDE_PATH-NOT_FOUND )
    message(FATAL_ERROR "No curses menu header found")
endif ()
if ( NOT ${CURSES_MENU_INCLUDE_PATH} IN_LIST CURSES_INCLUDE_DIRS )
    list(APPEND CURSES_INCLUDE_DIRS ${CURSES_MENU_INCLUDE_PATH})
endif ()

# Now find the menu library and add it to CURSES_LIBRARIES if
# not present:
find_library(CURSES_MENU_LIBRARY menu)
if ( CURSES_MENU_LIBRARY-NOT_FOUND )
    message(FATAL_ERROR "No curses menu library found")
endif ()
list(APPEND CURSES_LIBRARIES ${CURSES_MENU_LIBRARY})

#
# Add project info/version variables for the sake of the configure file:
#
set(TETROMINOTRIS_NAME ${PROJECT_NAME})
set(TETROMINOTRIS_VERSION ${tetrominotris_VERSION})

#
# Generate our common header file and have all targets in this
# directory use the binary directory on the search path:
#
configure_file(tetrominotris_config.h.in tetrominotris_config.h)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(TBitGrid_demo TBitGrid.c)
target_compile_definitions(TBitGrid_demo PRIVATE TBITGRID_DEBUG TBITGRID_DEMO)

#
# The game:
#
add_executable(tetrominotris TTetrominos.c TBitGrid.c tui_window.c tetrominotris.c)
target_include_directories(tetrominotris PRIVATE ${CURSES_INCLUDE_DIRS})
target_compile_options(tetrominotris PRIVATE ${CURSES_CFLAGS})
target_link_libraries(tetrominotris PRIVATE ${CURSES_LIBRARIES})

#
# The library demo:
#
add_executable(tetrominotris_demo TTetrominos.c TBitGrid.c tetrominotris_demo.c)

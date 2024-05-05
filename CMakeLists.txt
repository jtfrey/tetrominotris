#
# CMake build configuration for tetrominotris
#
cmake_minimum_required(VERSION 3.20)

project(tetrominotris
        VERSION 1.1.1
        DESCRIPTION "A blatant ripoff of Tetris"
        LANGUAGES C
    )
include(GNUInstallDirs)

option(TBOARD_DEBUG "Enable debug printing in TBoard code" ON)
option(ENABLE_COLOR_DISPLAY "Allow for color display." ON)

if ( NOT TETROMINOTRIS_HISCORES_FILE )
    set(TETROMINOTRIS_HISCORES_FILE "${CMAKE_INSTALL_LOCALSTATEDIR}/tetrominotris/hi-scores" CACHE FILEPATH "Location of the high scores file.")
endif ()
cmake_path(ABSOLUTE_PATH TETROMINOTRIS_HISCORES_FILE BASE_DIRECTORY "${CMAKE_INSTALL_PREFIX}" NORMALIZE OUTPUT_VARIABLE TETROMINOTRIS_HISCORES_FILEPATH)

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

#
# The game:
#
add_executable(tetrominotris TTetrominos.c TBitGrid.c TGameEngine.c TKeymap.c THighScores.c tui_window.c tetrominotris.c)
target_include_directories(tetrominotris PRIVATE ${CURSES_INCLUDE_DIRS})
target_compile_options(tetrominotris PRIVATE ${CURSES_CFLAGS})
target_link_libraries(tetrominotris PRIVATE ${CURSES_LIBRARIES} m)

#
# The hi-score util:
#
add_executable(hi-scores THighScores.c hi-scores.c)

#
# Install target(s):
#
install(TARGETS tetrominotris)
install(FILES example-keymap.txt TYPE SYSCONF)
cmake_path(GET TETROMINOTRIS_HISCORES_FILE PARENT_PATH TETROMINOTRIS_HISCORES_DIR)
cmake_path(GET TETROMINOTRIS_HISCORES_FILE FILENAME TETROMINOTRIS_HISCORES_NAME)
install(FILES assets/hi-scores
        DESTINATION "${TETROMINOTRIS_HISCORES_DIR}"
        RENAME "${TETROMINOTRIS_HISCORES_NAME}"
        PERMISSIONS OWNER_READ
        PERMISSIONS OWNER_WRITE
        PERMISSIONS GROUP_READ
        PERMISSIONS GROUP_WRITE
        PERMISSIONS WORLD_READ
        PERMISSIONS WORLD_WRITE)

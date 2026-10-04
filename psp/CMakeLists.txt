cmake_minimum_required(VERSION 3.5)
project(TopTennis C)
option(TT_PUBLIC "Use only artwork drawn for this project" ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -G0 -Wall -Wno-unused-function -std=gnu11 -I/usr/local/pspdev/psp/include/SDL2")

file(GLOB SRC ${CMAKE_SOURCE_DIR}/../src/*.c)
list(FILTER SRC EXCLUDE REGEX "assetview\\.c$")
add_executable(${PROJECT_NAME} ${SRC} psp_main.c)

target_link_libraries(${PROJECT_NAME}
  SDL2main SDL2 m GL pspnet_apctl pspnet_inet pspnet_resolver pspnet psputility pspvram pspaudio pspvfpu pspdisplay pspgu pspge psphprm pspctrl psppower)

# XMB artwork: icon (ICON0), preview shown when the game is selected (PIC0) and background (PIC1).
# psp/private_art (not in the repository) holds a screenshot of the game; without it, or with -DTT_PUBLIC=ON, the
# artwork drawn for this project in psp/art_public is used.
if(NOT TT_PUBLIC AND EXISTS ${CMAKE_SOURCE_DIR}/private_art/PIC1.PNG)
  set(ART ${CMAKE_SOURCE_DIR}/private_art)
else()
  set(ART ${CMAKE_SOURCE_DIR}/art_public)
endif()

create_pbp_file(
  TARGET ${PROJECT_NAME}
  ICON_PATH ${ART}/ICON0.PNG
  BACKGROUND_PATH ${ART}/PIC1.PNG
  PREVIEW_PATH ${ART}/PIC0.PNG
  TITLE "Top Tennis"
  VERSION 01.04)

cmake_minimum_required(VERSION 3.5)
project(TopTennis C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -G0 -Wall -Wno-unused-function -std=gnu11 -I/usr/local/pspdev/psp/include/SDL2")

file(GLOB SRC ${CMAKE_SOURCE_DIR}/../src/*.c)
list(FILTER SRC EXCLUDE REGEX "assetview\\.c$")
add_executable(${PROJECT_NAME} ${SRC} psp_main.c)

target_link_libraries(${PROJECT_NAME}
  SDL2main SDL2 m GL pspnet_apctl pspnet_inet pspnet_resolver pspnet psputility pspvram pspaudio pspvfpu pspdisplay pspgu pspge psphprm pspctrl psppower)

create_pbp_file(
  TARGET ${PROJECT_NAME}
  ICON_PATH NULL
  BACKGROUND_PATH NULL
  PREVIEW_PATH NULL
  TITLE "Top Tennis"
  VERSION 01.00)

cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(TopTennis C)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Top Tennis")
set(VITA_TITLEID  "TOPT00001")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wno-unused-function -std=gnu11 -I${VITASDK}/arm-vita-eabi/include/SDL2")

file(GLOB SRC ${CMAKE_SOURCE_DIR}/../src/*.c)
list(FILTER SRC EXCLUDE REGEX "assetview\\.c$")
add_executable(${PROJECT_NAME} ${SRC} vita_heap.c)

target_link_libraries(${PROJECT_NAME}
  SDL2 m
  SceGxm_stub SceDisplay_stub SceCtrl_stub SceAppMgr_stub SceAppUtil_stub SceAudio_stub SceAudioIn_stub
  SceSysmodule_stub SceIofilemgr_stub SceCommonDialog_stub SceTouch_stub SceHid_stub SceMotion_stub
  ScePower_stub SceProcessmgr_stub SceNet_stub SceNetCtl_stub)

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
set(VPK_FILES ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png)
# optional private LiveArea art (not in the repository): bg0.png, startup.png, template.xml in vita/private_art;
# the bubble icon stays the generic one
if(EXISTS ${CMAKE_SOURCE_DIR}/private_art/bg0.png)
  set(VPK_FILES
    ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png
    ${CMAKE_SOURCE_DIR}/private_art/bg0.png sce_sys/livearea/contents/bg0.png
    ${CMAKE_SOURCE_DIR}/private_art/startup.png sce_sys/livearea/contents/startup.png
    ${CMAKE_SOURCE_DIR}/private_art/template.xml sce_sys/livearea/contents/template.xml)
endif()

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${VPK_FILES})

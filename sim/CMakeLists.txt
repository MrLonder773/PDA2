cmake_minimum_required(VERSION 3.10)
project(pda2_sim C CXX)

# ════════════════════════════════════════════════════════
#  PDA 2 — sim/CMakeLists.txt
#  PC simulator build. Compiles PDA2Core + the current app
#  roster directly from ../lib/PDA2Core and ../src — no
#  copies, no duplication (see pda2-simulator memory doc).
#
#  FreeRTOS build path from the original lv_port_pc_vscode
#  template is dropped entirely here (not just left OFF):
#  Bt_Class_sim.cpp never touches FreeRTOS, and the
#  FreeRTOS/ submodule + config/FreeRTOSConfig.h were
#  already marked "discard entirely".
# ════════════════════════════════════════════════════════

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

find_package(SDL2 REQUIRED)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

# ── LVGL ──────────────────────────────────────────────────
add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})

# ── Paths into the main repo (sim/ shares these directly) ─
set(PDA2_ROOT     ${PROJECT_SOURCE_DIR}/..)
set(PDA2CORE_SRC  ${PDA2_ROOT}/lib/PDA2Core/src)
set(PDA2_APPS_SRC ${PDA2_ROOT}/src)
set(TINYGL_ROOT   ${PDA2_ROOT}/lib/tinygl)

# ── TinyGL sources ─────────────────────────────────────────
file(GLOB TINYGL_SOURCES ${TINYGL_ROOT}/src/*.c)

# ── Sources ───────────────────────────────────────────────
# NOTE: every *_esp32.cpp file is deliberately excluded — only
# the *_sim.cpp variant + the shared (unsuffixed) files go in.
set(PDA2_SOURCES
    # core
    ${PDA2CORE_SRC}/PDA2.cpp
    ${PDA2CORE_SRC}/pda2_platform_sim.cpp

    # utility — shared / platform-hook style (no _esp32/_sim split)
    ${PDA2CORE_SRC}/utility/Apps_Class.cpp
    ${PDA2CORE_SRC}/utility/Bme_Class.cpp
    ${PDA2CORE_SRC}/utility/Imu_Class.cpp
    ${PDA2CORE_SRC}/utility/QuickPanel_Class.cpp
    ${PDA2CORE_SRC}/utility/Rtc_Class.cpp
    ${PDA2CORE_SRC}/utility/Touch_Class.cpp        # already portable — uses pda2_platform_touch_* hooks
    ${PDA2CORE_SRC}/utility/Usb_Class.cpp
    ${PDA2CORE_SRC}/utility/Fs_Class.cpp

    # utility — _sim implementations
    ${PDA2CORE_SRC}/utility/Bt_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Display_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Fs_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Imu_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Bme_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Prefs_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Rtc_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Usb_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Wifi_Class_sim.cpp

    # apps — контрольный тест CRASH-1: возврат к исходному ростеру
    ${PDA2_APPS_SRC}/apps/clock/ClockApp.cpp
    # ${PDA2_APPS_SRC}/apps/accel/AccelApp.cpp
    ${PDA2_APPS_SRC}/apps/bluetooth/BluetoothApp.cpp
    # ${PDA2_APPS_SRC}/apps/stub/StubApp.cpp
    ${PDA2_APPS_SRC}/apps/tinygltest/TinyGLTestApp.cpp
    
    # Madgwick (AccelApp)
    ${PROJECT_SOURCE_DIR}/Madgwick/src/MadgwickAHRS.cpp

    # tinygl (TinyGLTestApp)
    ${TINYGL_SOURCES}
    
    # cyrillic fonts
    ${PDA2CORE_SRC}/fonts/pda2_cyrillic_12.c
    ${PDA2CORE_SRC}/fonts/pda2_cyrillic_14.c
    ${PDA2CORE_SRC}/fonts/pda2_cyrillic_16.c
    ${PDA2CORE_SRC}/fonts/pda2_cyrillic_20.c
    ${PDA2CORE_SRC}/fonts/pda2_cyrillic_24.c

    # sim entry point
    ${PROJECT_SOURCE_DIR}/src/main.cpp
)

add_executable(pda2_sim ${PDA2_SOURCES})

target_include_directories(pda2_sim PRIVATE
    ${PDA2CORE_SRC}
    ${PDA2_APPS_SRC}
    ${PROJECT_SOURCE_DIR}/Madgwick/src
    ${TINYGL_ROOT}/include                                 # ← добавить
    ${TINYGL_ROOT}/src                                     # ← добавить
)

target_compile_definitions(pda2_sim PRIVATE
    PDA2_SIM
    LV_CONF_INCLUDE_SIMPLE
)

target_link_libraries(pda2_sim lvgl SDL2::SDL2 m pthread)
if(WIN32)
    target_link_libraries(pda2_sim ws2_32)
endif()

# ── Debug warnings (kept from the original template) ──────
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(pda2_sim PRIVATE
        -Wall -Wextra -Wshadow -Wpointer-arith -Wuninitialized
    )
endif()

add_custom_target(run COMMAND ${EXECUTABLE_OUTPUT_PATH}/pda2_sim DEPENDS pda2_sim)
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

# ── Sources ───────────────────────────────────────────────
# NOTE: every *_esp32.cpp file is deliberately excluded — only
# the *_sim.cpp variant + the shared (unsuffixed) files go in.
set(PDA2_SOURCES
    # core
    ${PDA2CORE_SRC}/PDA2.cpp
    ${PDA2CORE_SRC}/pda2_platform_sim.cpp

    # utility — shared / platform-hook style (no _esp32/_sim split)
    # ⚠️ KNOWN BLOCKER (found while wiring this up, not yet fixed):
    #    Apps_Class.cpp unconditionally #includes <Arduino.h> and
    #    calls millis() twice — needs swap to pda2_platform_now_ms().
    ${PDA2CORE_SRC}/utility/Apps_Class.cpp
    # ⚠️ KNOWN BLOCKER: Fs_Class.h/.cpp is built on Arduino String
    #    and unconditionally #includes <LittleFS.h>/<SD.h>/<SPI.h>.
    #    Fs.begin() is called unconditionally from PDA2Class::begin(),
    #    so this blocks linking even though FilesApp isn't registered.
    #    Needs a real decision (stub vs platform-hook split) before
    #    this file will compile here.
    ${PDA2CORE_SRC}/utility/Fs_Class.cpp
    ${PDA2CORE_SRC}/utility/QuickPanel_Class.cpp
    ${PDA2CORE_SRC}/utility/Touch_Class.cpp        # already portable — uses pda2_platform_touch_* hooks
    ${PDA2CORE_SRC}/utility/Usb_Class.cpp

    # utility — _sim implementations
    ${PDA2CORE_SRC}/utility/Bt_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Display_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Imu_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Prefs_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Rtc_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Usb_Class_sim.cpp
    ${PDA2CORE_SRC}/utility/Wifi_Class_sim.cpp

    # apps — current registered roster (mirrors src/main.cpp)
    ${PDA2_APPS_SRC}/apps/clock/ClockApp.cpp
    ${PDA2_APPS_SRC}/apps/accel/AccelApp.cpp
    ${PDA2_APPS_SRC}/apps/gltest/GLTestApp.cpp
    ${PDA2_APPS_SRC}/apps/gltest/render.cpp
    ${PDA2_APPS_SRC}/apps/bluetooth/BluetoothApp.cpp

    # sim entry point
    ${PROJECT_SOURCE_DIR}/src/main.cpp
)

add_executable(pda2_sim ${PDA2_SOURCES})

target_include_directories(pda2_sim PRIVATE
    ${PDA2CORE_SRC}
    ${PDA2_APPS_SRC}
    ${PROJECT_SOURCE_DIR}/tgx/src            # GLTestApp <tgx.h> — vindar/tgx submodule
    ${PROJECT_SOURCE_DIR}/MadgwickAHRS/src   # AccelApp <MadgwickAHRS.h> — arduino-libraries/MadgwickAHRS submodule
)

target_compile_definitions(pda2_sim PRIVATE
    PDA2_SIM
    LV_CONF_INCLUDE_SIMPLE
)

target_link_libraries(pda2_sim lvgl ${SDL2_LIBRARIES} m pthread)

# ── Debug warnings (kept from the original template) ──────
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(pda2_sim PRIVATE
        -Wall -Wextra -Wshadow -Wpointer-arith -Wuninitialized
    )
endif()

add_custom_target(run COMMAND ${EXECUTABLE_OUTPUT_PATH}/pda2_sim DEPENDS pda2_sim)

#pragma once
#include <PDA2.h>
#include "render.h"

class GLTestApp : public PDA2App {
public:
    GLTestApp() { name = "TGX test"; }

    void onInit()  override;
    void onOpen()  override;
    void onClose() override;
    void onTick(uint32_t delta_ms) override;

private:
    static constexpr int W        = 200;
    static constexpr int H        = 200;
    static constexpr int SCREEN_X = (320 - W) / 2;
    static constexpr int SCREEN_Y = (480 - H) / 2;

    ::load   load;
    ::render render;

    // framebuffer живёт здесь, render только рисует в него
    tgx::RGB565 buffer[W * H];
    tgx::Image<tgx::RGB565> im{buffer, W, H};
};

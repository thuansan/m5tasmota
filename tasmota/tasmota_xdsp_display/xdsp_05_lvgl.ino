// xdsp_05_lvgl.ino - Tasmota LVGL Display Driver

#ifdef USE_LVGL_DISPLAY

#include <lvgl.h>
#include "lvgl_tasmota.h"
#include "driver/tft/tft.h"  // Tùy màn hình bạn dùng

#define XDSP_05            5

bool LvglDisplayInitialized = false;

void LvglUpdateTask(void) {
  lv_timer_handler();
}

void LvglSetup(void) {
  lv_init();
  lvgl_display_init(); // hàm khởi tạo driver cụ thể (do bạn định nghĩa trong lvgl_tasmota.h)

  // Tạo giao diện UI nếu có
  lvgl_ui_init(); // tuỳ theo bạn định nghĩa bên ui.c

  LvglDisplayInitialized = true;
  AddTaskEverySecond(LvglUpdateTask);
}

bool Xdsp05(uint8_t function) {
  switch (function) {
    case FUNC_DISPLAY_INIT:
      LvglSetup();
      break;
    case FUNC_DISPLAY_EVERY_50_MSECOND:
      if (LvglDisplayInitialized) lv_timer_handler();
      break;
  }
  return true;
}

#endif  // USE_LVGL_DISPLAY
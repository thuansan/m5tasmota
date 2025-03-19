#define XDRV_99 99

#ifdef USE_LVGL_UI

#include <lvgl.h>

lv_obj_t *label_hello;

void LvglInitDriver(void) {
  // Tạo giao diện LVGL ngay khi khởi động
  label_hello = lv_label_create(lv_scr_act());
  lv_label_set_text(label_hello, "Hello from Tasmota UI!");
  lv_obj_align(label_hello, LV_ALIGN_CENTER, 0, 0);
}

void LvglRefreshDriver(void) {
  // Cập nhật nội dung nếu cần
  // lv_label_set_text_fmt(label_hello, "Temp: %d C", some_value);
}

bool Xdrv99(uint8_t function) {
  switch (function) {
    case FUNC_INIT:
      AddLog(LOG_LEVEL_INFO, PSTR("LVGL UI driver initialized"));
      break;

    case FUNC_LVGL_INIT:
      LvglInitDriver();  // ← gọi tạo UI ở đây
      break;

    case FUNC_LVGL_REFRESH:
      LvglRefreshDriver();
      break;
  }
  return false;
}

#endif  // USE_LVGL_UI

#define XDRV_99 99

#ifdef USE_LVGL_UI

#include <lvgl.h>

lv_obj_t *label_hello;

void LvglInitDriver(void) {
  
  label_hello = lv_label_create(lv_scr_act());
  lv_label_set_text(label_hello, "Hello from Tasmota UI!");
  lv_obj_align(label_hello, LV_ALIGN_CENTER, 0, 0);
}

void LvglRefreshDriver(void) {
  
  // lv_label_set_text_fmt(label_hello, "Temp: %d C", some_value);
}

bool Xdrv99(uint8_t function) {
  switch (function) {
    case FUNC_INIT:
      AddLog(LOG_LEVEL_INFO, PSTR("LVGL UI driver initialized"));
      break;

      case FUNC_PRE_INIT:
      LvglInitDriver();
      break;
  case FUNC_LOOP:
  lv_refr_now(NULL); // Đây là cách gọi hàm refresh toàn bộ màn hình trong LVGL

      break;
  
  }
  return false;
}

#endif  // USE_LVGL_UI

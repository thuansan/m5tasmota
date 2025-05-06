/*
  xdrv_54_lvgl.ino - LVLG integration

  Copyright (C) 2021 Stephan Hadinger, Berry language by Guan Wenliang https://github.com/Skiars/berry

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef ESP32
#if defined(USE_LVGL) && defined(USE_UNIVERSAL_DISPLAY)

#include <renderer.h>
#include "lvgl.h"
#include "core/lv_global.h"         // needed for LV_GLOBAL_DEFAULT
#include "tasmota_lvgl_assets.h"    // force compilation of assets

#define XDRV_54             54

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <ArduinoJson.h>


#include "image/image_resource.h"
#include "sensor_data.h"
#include "time.h"
#include "lv_timer.h"



//callback type when a screen paint is done
typedef void (*lv_paint_cb_t)(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t *pixels);

struct LVGL_Glue {
  lv_display_t *lv_display = nullptr;
  lv_indev_t *lv_indev = nullptr;
  void *lv_pixel_buf = nullptr;
  void *lv_pixel_buf2 = nullptr;
  Ticker tick;
  File * screenshot = nullptr;
  lv_paint_cb_t paint_cb = nullptr;
};
LVGL_Glue * lvgl_glue;


// >>>> CODE ADD <<<<

 extern DHT20 Dht20;


lv_obj_t *wifi_icon;
lv_obj_t *time_label;
static lv_obj_t *temp_label;
static lv_obj_t *humi_label;
static lv_obj_t *so2_label;
static lv_obj_t *ac_label;
static lv_obj_t *co_label;
static lv_obj_t *no2_label;
static lv_obj_t *ozone_label;

String latest_alert_msg = "";


void update_time_label(lv_timer_t *timer);  
void sensor_check_task(lv_timer_t *timer);
void create_relay_controls(lv_obj_t *parent);

bool is_alert_active = false;

void LvglMqttSubscribe() {
  MqttSubscribe("v1/devices/me/rpc/request/+");
}

void HandleMqttData(char* topic, uint8_t* data, unsigned int length) {
  data[length] = '\0';
  String jsonStr = String((char*)data);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, jsonStr);
  if (error) {
    Serial.printf("JSON error: %s\n", error.c_str());
    return;
  }

  // Nếu có cảnh báo
  if (doc.containsKey("method") && doc["method"] == "alarm") {
    if (doc.containsKey("params")) {
      latest_alert_msg = doc["params"].as<String>();
    }
  }
}


void CustomLVGL_InitUI() {
 
  /*Create a Tab view object*/
  
  lv_obj_t * tabview = lv_tabview_create(lv_scr_act());

  // Ẩn thanh tab
  lv_obj_add_flag(lv_tabview_get_tab_bar(tabview), LV_OBJ_FLAG_HIDDEN);

  /*tabs*/
  lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "Tab 1");
  lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "Tab 2");
  lv_obj_t * tab3 = lv_tabview_add_tab(tabview, "Tab 3");
  lv_obj_t * tab4 = lv_tabview_add_tab(tabview, "Tab 4");


// thanh bar
  lv_obj_t *top_bar = lv_obj_create(lv_scr_act());
  lv_obj_set_size(top_bar, LV_HOR_RES, 35);  
  lv_obj_set_style_bg_color(top_bar, lv_color_hex(0xB4B4B4), 0);  
  lv_obj_set_style_border_opa(top_bar, LV_OPA_TRANSP, 0);
  lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
  lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);  

    //hiển thị thời gian
  lv_obj_t *time_container = lv_obj_create(top_bar);
  lv_obj_set_size(time_container, 220, 35);  
  lv_obj_set_style_pad_all(time_container, 0, 0);
  lv_obj_set_style_border_opa(time_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_shadow_opa(time_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_bg_opa(time_container, LV_OPA_TRANSP, 0);
  lv_obj_align(time_container, LV_ALIGN_TOP_LEFT, -20, -13);

  time_label = lv_label_create(time_container);
  lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_tasmota_20, 0); 
  lv_timer_create(update_time_label, 1000, NULL);

  // wifi
  lv_obj_t *wifi_container = lv_obj_create(top_bar);
  lv_obj_set_size(wifi_container, 33, 33);  
  lv_obj_set_style_pad_all(wifi_container, 0, 0);
  lv_obj_set_style_border_opa(wifi_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_shadow_opa(wifi_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_bg_opa(wifi_container, LV_OPA_TRANSP, 0);
  lv_obj_align(wifi_container, LV_ALIGN_TOP_RIGHT, 0, -14);

  wifi_icon = lv_img_create(wifi_container);
  lv_obj_set_size(wifi_icon, 25, 25);  
  lv_obj_align(wifi_icon, LV_ALIGN_CENTER, 0, 0);

    // BK icon
  lv_obj_t *bk_container = lv_obj_create(top_bar);
  lv_obj_set_size(bk_container, 33, 33);  
  lv_obj_set_style_pad_all(bk_container, 0, 0);
  lv_obj_set_style_border_opa(bk_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_shadow_opa(bk_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_bg_opa(bk_container, LV_OPA_TRANSP, 0);
  lv_obj_align(bk_container, LV_ALIGN_TOP_RIGHT, -35, -14);
     
  lv_obj_t *bk_icon = lv_img_create(bk_container);
  lv_img_set_src(bk_icon, &hcmut);
  lv_obj_set_size(bk_icon, 25, 25);  
  lv_obj_align(bk_icon, LV_ALIGN_CENTER, 0, 0);

  //--------------Trang 1:-------------------

  lv_obj_set_style_bg_color(tab1, lv_color_hex(0x201861), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(tab1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *rect1 = lv_obj_create(tab1);
  lv_obj_set_size(rect1, 160, 95);
  lv_obj_set_style_bg_color(rect1, lv_color_hex(0x2379F1), 0); // Temp
  lv_obj_set_style_border_opa(rect1, LV_OPA_TRANSP, 0);
  lv_obj_set_style_radius(rect1, 15, 0);
  lv_obj_align(rect1, LV_ALIGN_TOP_LEFT, -1, 25);

  temp_label = lv_label_create(rect1);
  lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_tasmota_28, 0);
  lv_obj_align(temp_label, LV_ALIGN_LEFT_MID, 0, -15);

  lv_obj_t *C = lv_label_create(rect1);
  lv_label_set_text(C, "°C");
  lv_obj_set_style_text_font(C, &lv_font_montserrat_tasmota_28, 0);
  lv_obj_align(C, LV_ALIGN_LEFT_MID, 10, 15);

  lv_obj_t *dash = lv_label_create(rect1);
  lv_label_set_text(dash, "       |");
  lv_obj_set_style_text_font(dash, &lv_font_montserrat_tasmota_28, 0);
  lv_obj_align(dash, LV_ALIGN_LEFT_MID, 5, 10);

  lv_obj_t *weather_icon = lv_img_create(rect1);
  lv_img_set_src(weather_icon, &weathe);  
  lv_obj_align(weather_icon, LV_ALIGN_RIGHT_MID, 0, -2);
 
  lv_obj_t *rect2 = lv_obj_create(tab1);
  lv_obj_set_size(rect2, 125, 95);
  lv_obj_set_style_bg_color(rect2, lv_color_hex(0x46DCAE), 0); // so2
  lv_obj_set_style_border_opa(rect2, LV_OPA_TRANSP, 0);
  lv_obj_set_style_radius(rect2, 15, 0);
  lv_obj_align(rect2, LV_ALIGN_TOP_RIGHT, 0, 25);

  lv_obj_t *so2_icon = lv_img_create(rect2);
  lv_img_set_src(so2_icon, &co2);  
  lv_obj_align(so2_icon, LV_ALIGN_CENTER, 0, 0);

  so2_label = lv_label_create(rect2);
  lv_obj_set_style_text_font(so2_label, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(so2_label, lv_color_hex(0x32CD32), 0);
  lv_obj_align(so2_label, LV_ALIGN_CENTER, 0, 5);
  
  lv_obj_t *ppm = lv_label_create(rect2);
  lv_label_set_text(ppm, "PPM");
  lv_obj_set_style_text_font(ppm, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(ppm, lv_color_hex(0x46DCAE), 0);
  lv_obj_align(ppm, LV_ALIGN_CENTER, 0, 21);

  lv_obj_t *rect3 = lv_obj_create(tab1);
  lv_obj_set_size(rect3, 140, 95);
  lv_obj_set_style_bg_color(rect3, lv_color_hex(0x49BAEC), 0); // ac measure
  lv_obj_set_style_border_opa(rect3, LV_OPA_TRANSP, 0);
  lv_obj_set_style_radius(rect3, 15, 0);
  lv_obj_align(rect3, LV_ALIGN_BOTTOM_LEFT, -1, 5);

  lv_obj_t *ac_icon = lv_img_create(rect3);
  lv_img_set_src(ac_icon, &ac);  
  lv_obj_align(ac_icon, LV_ALIGN_LEFT_MID, -10, 10);

  lv_obj_t *actext = lv_label_create(rect3);
  lv_label_set_text(actext, "AC measure");
  lv_obj_set_style_text_font(actext, &lv_font_montserrat_18, 0);
  lv_obj_align(actext, LV_ALIGN_LEFT_MID, 0, -32);

  ac_label = lv_label_create(rect3);
  lv_obj_set_style_text_font(ac_label, &lv_font_montserrat_tasmota_20, 0);
  lv_obj_align(ac_label, LV_ALIGN_RIGHT_MID, 0, -10);

  lv_obj_t *A = lv_label_create(rect3);
  lv_label_set_text(A, "A");
  lv_obj_set_style_text_font(A, &lv_font_montserrat_tasmota_20, 0);
  lv_obj_align(A, LV_ALIGN_RIGHT_MID, -10, 20);

  lv_obj_t *rect4 = lv_obj_create(tab1);
  lv_obj_set_size(rect4, 145, 95);
  lv_obj_set_style_bg_color(rect4, lv_color_hex(0x8990F2), 0); // humi
  lv_obj_set_style_border_opa(rect4, LV_OPA_TRANSP, 0);
  lv_obj_set_style_radius(rect4, 15, 0);
  lv_obj_align(rect4, LV_ALIGN_BOTTOM_RIGHT, 0, 5);

  lv_obj_t *humi_icon = lv_img_create(rect4);
  lv_img_set_src(humi_icon, &humi);  
  lv_obj_align(humi_icon, LV_ALIGN_LEFT_MID, -10, 10);

  lv_obj_t *humitext = lv_label_create(rect4);
  lv_label_set_text(humitext, "Humidity");
  lv_obj_set_style_text_font(humitext, &lv_font_montserrat_18, 0);
  lv_obj_align(humitext, LV_ALIGN_LEFT_MID, 0, -32);

  humi_label = lv_label_create(rect4);
  lv_obj_set_style_text_font(humi_label, &lv_font_montserrat_tasmota_20, 0);
  lv_obj_align(humi_label, LV_ALIGN_RIGHT_MID, -5, -10);

  lv_obj_t *humic = lv_label_create(rect4);
  lv_label_set_text(humic, "%");
  lv_obj_set_style_text_font(humic, &lv_font_montserrat_tasmota_28, 0);
  lv_obj_align(humic, LV_ALIGN_RIGHT_MID, -10, 20);

    //--------------Trang 2:-------------------
  lv_obj_set_style_bg_color(tab2, lv_color_hex(0x201861), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(tab2, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_t *block1 = lv_obj_create(tab2);
  lv_obj_set_size(block1, 295, 95);
  lv_obj_set_style_bg_color(block1, lv_color_hex(0x2379F1), 0); // ozone
  lv_obj_set_style_border_opa(block1, LV_OPA_TRANSP, 0);
  lv_obj_set_style_radius(block1, 15, 0);
  lv_obj_align(block1, LV_ALIGN_TOP_LEFT, -1, 25);

  lv_obj_t *ozonetext = lv_label_create(block1);
  lv_label_set_text(ozonetext, "Ozone");
  lv_obj_set_style_text_font(ozonetext, &lv_font_montserrat_tasmota_28, 0);
  lv_obj_align(ozonetext, LV_ALIGN_RIGHT_MID, -80, -15);

  ozone_label = lv_label_create(block1);
  lv_obj_set_style_text_font(ozone_label, &lv_font_montserrat_tasmota_20, 0);
  lv_obj_align(ozone_label, LV_ALIGN_RIGHT_MID, -120, 15);

  lv_obj_t *ozonec = lv_label_create(block1);
  lv_label_set_text(ozonec, "current");
  lv_obj_set_style_text_font(ozonec, &lv_font_montserrat_tasmota_20, 0);
  lv_obj_align(ozonec, LV_ALIGN_RIGHT_MID, -20, 15);

  lv_obj_t *ozone_icon = lv_img_create(block1);
  lv_img_set_src(ozone_icon, &ozone);  
  lv_obj_align(ozone_icon, LV_ALIGN_LEFT_MID, 0, -2);

  lv_obj_t *block2 = lv_obj_create(tab2);
  lv_obj_set_size(block2, 140, 95);
  lv_obj_set_style_bg_color(block2, lv_color_hex(0x49BAEC), 0); // no2
  lv_obj_set_style_border_opa(block2, LV_OPA_TRANSP, 0);
  lv_obj_set_style_radius(block2, 15, 0);
  lv_obj_align(block2, LV_ALIGN_BOTTOM_LEFT, -1, 5);

  lv_obj_t *no2_icon = lv_img_create(block2);
  lv_img_set_src(no2_icon, &No2);  
  lv_obj_align(no2_icon, LV_ALIGN_LEFT_MID, -5, 0);

  no2_label = lv_label_create(block2);
  lv_obj_set_style_text_font(no2_label, &lv_font_montserrat_tasmota_20, 0);
  lv_obj_align(no2_label, LV_ALIGN_RIGHT_MID, 0, -15);

  lv_obj_t *ppmno2 = lv_label_create(block2);
  lv_label_set_text(ppmno2, "ppm");
  lv_obj_set_style_text_font(ppmno2, &lv_font_montserrat_tasmota_20, 0);
  lv_obj_align(ppmno2, LV_ALIGN_RIGHT_MID, 0, 15);

  lv_obj_t *block3 = lv_obj_create(tab2);
  lv_obj_set_size(block3, 145, 95);
  lv_obj_set_style_bg_color(block3, lv_color_hex(0x46DCAE), 0); // co
  lv_obj_set_style_border_opa(block3, LV_OPA_TRANSP, 0);
  lv_obj_set_style_radius(block3, 15, 0);
  lv_obj_align(block3, LV_ALIGN_BOTTOM_RIGHT, 0, 5);

  lv_obj_t *co_icon = lv_img_create(block3);
  lv_img_set_src(co_icon, &Co);  
  lv_obj_align(co_icon, LV_ALIGN_LEFT_MID, -5, 0);

  co_label = lv_label_create(block3);
  lv_obj_set_style_text_font(co_label, &lv_font_montserrat_tasmota_20, 0);
  lv_obj_align(co_label, LV_ALIGN_RIGHT_MID, 0, -15);

  lv_obj_t *ppmco = lv_label_create(block3);
  lv_label_set_text(ppmco, "ppm");
  lv_obj_set_style_text_font(ppmco, &lv_font_montserrat_tasmota_20, 0);
  lv_obj_align(ppmco, LV_ALIGN_RIGHT_MID, 0, 15);

    //--------------Trang 3:-------------------
  lv_obj_set_style_bg_color(tab3, lv_color_hex(0x201861), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(tab3, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  
  lv_obj_t *part1 = lv_obj_create(tab3);
  lv_obj_set_size(part1, 143, 180);
  lv_obj_set_style_bg_color(part1, lv_color_hex(0x2379F1), 0); // 2.5
  lv_obj_set_style_border_opa(part1, LV_OPA_TRANSP, 0);
  lv_obj_set_style_radius(part1, 15, 0);
  lv_obj_align(part1, LV_ALIGN_TOP_LEFT, -1, 25);
  
  lv_obj_t *ppm25_icon = lv_img_create(part1);
  lv_img_set_src(ppm25_icon, &pm2);  
  lv_obj_align(ppm25_icon, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_t *part2 = lv_obj_create(tab3);
  lv_obj_set_size(part2, 143, 180);
  lv_obj_set_style_bg_color(part2, lv_color_hex(0x46DCAE), 0); // 10
  lv_obj_set_style_border_opa(part2, LV_OPA_TRANSP, 0);
  lv_obj_set_style_radius(part2, 15, 0);
  lv_obj_align(part2, LV_ALIGN_TOP_RIGHT, 0, 25);

  lv_obj_t *ppm10_icon = lv_img_create(part2);
  lv_img_set_src(ppm10_icon, &pm10);  
  lv_obj_align(ppm10_icon, LV_ALIGN_TOP_MID, 0, 0);

  //--------------Trang 4:-------------------

  lv_obj_set_style_bg_color(tab4, lv_color_hex(0x201861), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(tab4, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  create_relay_controls(tab4);


  update_wifi_icon(); 
  update_temp_label();
  
  CustomLVGL_StartMonitoring();
}

void create_relay_controls(lv_obj_t *parent) {
  lv_obj_set_style_pad_top(parent, 30, 0);  

  static bool relay_states[4] = {false, false, false, false};

  for (int i = 0; i < 4; i++) {
    // Container
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, 120, 90); 
    lv_obj_set_style_pad_all(container, 5, 0); 
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Xếp 
    int row = i / 2;
    int col = i % 2;
    lv_obj_align(container, LV_ALIGN_TOP_LEFT, 25 + col * 130, 10 + row * 100);

    // 
    char label[16];
    snprintf(label, sizeof(label), "Relay %d", i + 1);
    lv_obj_t *lbl = lv_label_create(container);
    lv_label_set_text(lbl, label);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 5, 0);

    // Button ON/OFF
    lv_obj_t *btn = lv_btn_create(container);
    lv_obj_set_size(btn, 100, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0); // 

    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_RED), 0);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "OFF");
    lv_obj_center(btn_label); 

    // Event handler
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
      int idx = lv_obj_get_index(lv_obj_get_parent(btn)); 

      relay_states[idx] = !relay_states[idx];
      lv_obj_t *btn_lbl = lv_obj_get_child(btn, 0);
      lv_label_set_text(btn_lbl, relay_states[idx] ? "ON" : "OFF");

      lv_color_t color = relay_states[idx] ? 
                         lv_palette_main(LV_PALETTE_GREEN) :
                         lv_palette_main(LV_PALETTE_RED);
      lv_obj_set_style_bg_color(btn, color, 0);

      //control
    }, LV_EVENT_CLICKED, NULL);
  }
}

// Box
void display_alert(const char *message) {
  if (is_alert_active) return;  
  is_alert_active = true;

lv_obj_t *alert_popup = lv_msgbox_create(lv_scr_act());
lv_obj_set_width(alert_popup, 240);  

static lv_style_t style_box, style_text;
lv_style_init(&style_box);
lv_style_set_bg_color(&style_box, lv_color_hex(0xFFFF99));  // nền vàng nhạt
lv_style_set_border_width(&style_box, 2);
lv_style_set_border_color(&style_box, lv_color_hex(0xFFAA00));

lv_style_init(&style_text);
lv_style_set_text_color(&style_text, lv_color_hex(0xCC0000));  // đỏ
lv_style_set_bg_color(&style_text, lv_color_hex(0xFFFF99));
lv_style_set_text_align(&style_text, LV_TEXT_ALIGN_CENTER);

lv_obj_add_style(alert_popup, &style_box, 0);

lv_obj_t *header = lv_obj_create(alert_popup);
lv_obj_set_width(header, lv_pct(100));
lv_obj_set_height(header, 30);
lv_obj_set_style_bg_color(header, lv_color_hex(0xFFFF66), 0);  // vàng đậm
lv_obj_set_style_pad_all(header, 0, 0);
lv_obj_set_style_border_width(header, 0, 0);
lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

lv_obj_t *title = lv_label_create(header);
lv_label_set_text(title, "Warning");
lv_obj_set_style_text_color(title, lv_color_hex(0xCC0000), 0);
lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
lv_obj_center(title);

lv_obj_move_to_index(header, 0);

//message text
lv_obj_t *text = lv_msgbox_add_text(alert_popup, message);
lv_obj_add_style(text, &style_text, 0);

lv_obj_t *ok_btn = lv_msgbox_add_footer_button(alert_popup, "OK");
  lv_obj_add_event_cb(ok_btn, [](lv_event_t * e) {
      lv_obj_t *alert_popup = (lv_obj_t *)lv_event_get_user_data(e); 
      lv_obj_del(alert_popup);   
      is_alert_active = false;
  }, LV_EVENT_CLICKED, alert_popup);  
  
  lv_obj_center(alert_popup);
}

void sensor_check_task(lv_timer_t *timer) {
if (!is_alert_active && latest_alert_msg.length() > 0) {
    display_alert(latest_alert_msg.c_str());
    latest_alert_msg = ""; 
  }
}


void CustomLVGL_StartMonitoring() {
  lv_timer_create(sensor_check_task, 10000, NULL);  
}

// 
void update_temp_label() {
  
  if (Dht20.valid) {  
      char temp_str[32];
      snprintf(temp_str, sizeof(temp_str), "%.1f |", Dht20.temperature);
      lv_label_set_text(temp_label, temp_str);
  }
}


int get_wifi_signal_strength() {
  int rssi = WiFi.RSSI();
  if (rssi > -50) return 4;  
  if (rssi > -60) return 3;     
  return 2;  
    
}

void update_time_label(lv_timer_t *timer) {
  setenv("TZ", "UTC-6", 1);  
  tzset();
  char time_str[32];
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  
  // HH:MM:SS - DD/MM/YY
  snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d - %02d/%02d/%02d",
  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
  timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year - 100);

  lv_label_set_text(time_label, time_str);
}

void update_wifi_icon() {
    int signal_level = get_wifi_signal_strength();
    
    switch(signal_level) {
        case 4: lv_img_set_src(wifi_icon, &W4); break;
        case 3: lv_img_set_src(wifi_icon, &W3); break;
        default: lv_img_set_src(wifi_icon, &W2); break;
    }
}


// This function can be called from your setup or init phase
void CustomLVGL_Initialize() {
  if (lvgl_glue && lvgl_glue->lv_display) {
    CustomLVGL_InitUI();
  }
}
// >>>> END CUSTOM CODE <<<<

// **************************************************
// Logging
// **************************************************
#if LV_USE_LOG
#ifdef USE_BERRY
static void lvbe_debug(lv_log_level_t, const char *msg);
static void lvbe_debug(lv_log_level_t, const char *msg) {
  be_writebuffer("LVG: ", sizeof("LVG: "));
  be_writebuffer(msg, strlen(msg));
}
#endif
#endif

/************************************************************
 * Main screen refresh function
 ************************************************************/
// This is the flush function required for LittlevGL screen updates.
// It receives a bounding rect and an array of pixel data (conveniently
// already in 565 format, so the Earth was lucky there).
void lv_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p);
void lv_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p) {
  uint16_t width = (area->x2 - area->x1 + 1);
  uint16_t height = (area->y2 - area->y1 + 1);

  // check if we are currently doing a screenshot
  if (lvgl_glue->screenshot != nullptr) {
    // save pixels to file
    int32_t btw = (width * height * LV_COLOR_DEPTH + 7) / 8;
    yield();            // ensure WDT does not fire
    while (btw > 0) {
      if (btw > 0) {    // if we had a previous error (ex disk full) don't try to write anymore
        int32_t ret = lvgl_glue->screenshot->write((const uint8_t*) color_p, btw);
        if (ret >= 0) {
          btw -= ret;
        } else {
          btw = 0;  // abort
        }
      }
    }
    lv_disp_flush_ready(disp);
    return; // ok
  }

  uint32_t pixels_len = width * height;
  uint32_t chrono_start = millis();
  renderer->setAddrWindow(area->x1, area->y1, area->x1+width, area->y1+height);
  renderer->pushColors((uint16_t *)color_p, pixels_len, true);
  renderer->setAddrWindow(0,0,0,0);
  renderer->Updateframe();
  uint32_t chrono_time = millis() - chrono_start;

  lv_disp_flush_ready(disp);

  if (pixels_len >= 10000 && (!renderer->lvgl_param.use_dma)) {
    if (HighestLogLevel() >= LOG_LEVEL_DEBUG_MORE) {
      AddLog(LOG_LEVEL_DEBUG_MORE, D_LOG_LVGL "Refreshed %d pixels in %d ms (%i pix/ms)", pixels_len, chrono_time,
              chrono_time > 0 ? pixels_len / chrono_time : -1);
    }
  }
  // if there is a display callback, call it
  if (lvgl_glue->paint_cb != nullptr) {
    lvgl_glue->paint_cb(area->x1, area->y1, area->x2, area->y2, color_p);
  }
}

void lv_set_paint_cb(void* cb);
void lv_set_paint_cb(void* cb) {
  lvgl_glue->paint_cb = (lv_paint_cb_t) cb;
}

void * lv_get_paint_cb(void);
void * lv_get_paint_cb(void) {
  return (void*) lvgl_glue->paint_cb;
}


/************************************************************
 * Emulation of stdio for FreeType
 *
 ************************************************************/

#ifdef USE_UFILESYS

#include <FS.h>
#include "ZipReadFS.h"
extern FS *ffsp;
extern FS *ufsp;
FS lv_zip_ufsp(ZipReadFSImplPtr(new ZipReadFSImpl(&ffsp, "/sd/", &ufsp)));

extern "C" {

  typedef void lvbe_FILE;

  // FILE * fopen ( const char * filename, const char * mode );
  lvbe_FILE * lvbe_fopen(const char * filename, const char * mode ) {

    // Add "/" prefix
    String file_path = "/";
    file_path += filename;

    File f = lv_zip_ufsp.open(file_path, mode);
    // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_fopen(%s) -> %i", file_path.c_str(), (int32_t)f);
    // AddLog(LOG_LEVEL_INFO, "LVG: F=%*_H", sizeof(f), &f);
    if (f) {
      File * f_ptr = new File(f);                 // copy to dynamic object
      *f_ptr = f;                                 // TODO is this necessary?
      return f_ptr;
    }
    return nullptr;
  }

  // int fclose ( FILE * stream );
  lv_fs_res_t lvbe_fclose(lvbe_FILE * stream) {
    File * f_ptr = (File*) stream;
    f_ptr->close();
    delete f_ptr;
    // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_fclose(%p)", f_ptr);
    return LV_FS_RES_OK;
  }

  // size_t fread ( void * ptr, size_t size, size_t count, FILE * stream );
  size_t lvbe_fread(void * ptr, size_t size, size_t count, lvbe_FILE * stream) {
    File * f_ptr = (File*) stream;
    // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_fread (%p, %i, %i, %p)", ptr, size, count, f_ptr);

    int32_t ret = f_ptr->read((uint8_t*) ptr, size * count);
    // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_fread -> %i", ret);
    if (ret < 0) {    // error
      ret = 0;
    }
    return ret;
  }

  // int fseek ( FILE * stream, long int offset, int origin );
  int lvbe_fseek(lvbe_FILE * stream, long int offset, int origin ) {
    File * f_ptr = (File*) stream;
    // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_fseek(%p, %i, %i)", f_ptr, offset, origin);
    
    fs::SeekMode mode = fs::SeekMode::SeekSet;
    if (SEEK_CUR == origin) {
      mode = fs::SeekMode::SeekCur;
    } else if (SEEK_END == origin) {
      mode = fs::SeekMode::SeekEnd;
    }
    bool ok = f_ptr->seek(offset, mode);
    return ok ? 0 : -1;
  }

  // long int ftell ( FILE * stream );
  int lvbe_ftell(lvbe_FILE * stream) {
    File * f_ptr = (File*) stream;
    // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_ftell(%p) -> %i", f_ptr, f_ptr->position());
    return f_ptr->position();
  }

}
#endif // USE_UFILESYS

/************************************************************
 * Callbacks for file system access from LVGL
 *
 * Useful to load fonts or images from file system
 ************************************************************/

#ifdef USE_UFILESYS
static void * lvbe_fs_open(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode);
static void * lvbe_fs_open(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode) {
  // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_fs_open(%p, %p, %s, %i) %i", drv, file_p, path, mode, sizeof(File));
  const char * modes = nullptr;
  switch (mode) {
    case LV_FS_MODE_WR:                   modes = "w";    break;
    case LV_FS_MODE_RD:                   modes = "r";    break;
    case LV_FS_MODE_WR | LV_FS_MODE_RD:   modes = "rw";   break;
  }

  if (modes == nullptr) {
    AddLog(LOG_LEVEL_INFO, "LVG: fs_open, unsupported mode %d", mode);
    return nullptr;
  }

  return (void*) lvbe_fopen(path, modes);
}

static lv_fs_res_t lvbe_fs_close(lv_fs_drv_t * drv, void * file_p);
static lv_fs_res_t lvbe_fs_close(lv_fs_drv_t * drv, void * file_p) {
  return lvbe_fclose((void*)file_p);
}

static lv_fs_res_t lvbe_fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br);
static lv_fs_res_t lvbe_fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br) {
  // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_fs_read(%p, %p, %p, %i, %p)", drv, file_p, buf, btr, br);
  File * f_ptr = (File*) file_p;
  // AddLog(LOG_LEVEL_INFO, "LVG: F=%*_H", sizeof(File), f_ptr);
  int32_t ret = f_ptr->read((uint8_t*) buf, btr);
  // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_fs_read -> %i", ret);
  if (ret >= 0) {
    *br = ret;
    return LV_FS_RES_OK;
  } else {
    return LV_FS_RES_UNKNOWN;
  }
}

static lv_fs_res_t lvbe_fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw);
static lv_fs_res_t lvbe_fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw) {
  // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_fs_write(%p, %p, %p, %i, %p)", drv, file_p, buf, btw, bw);
  File * f_ptr = (File*) file_p;
  int32_t ret = f_ptr->write((const uint8_t*) buf, btw);
  if (ret >= 0) {
    *bw = ret;
    return LV_FS_RES_OK;
  } else {
    return LV_FS_RES_UNKNOWN;
  }
}

static lv_fs_res_t lvbe_fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p);
static lv_fs_res_t lvbe_fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p) {
  // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_fs_tell(%p, %p, %p)", drv, file_p, pos_p);
  File * f_ptr = (File*) file_p;
  *pos_p = f_ptr->position();
  return LV_FS_RES_OK;
}

static lv_fs_res_t lvbe_fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence);
static lv_fs_res_t lvbe_fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence) {
  // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_fs_seek(%p, %p, %i)", drv, file_p, pos);
  File * f_ptr = (File*) file_p;
  SeekMode seek;
  switch (whence) {
    case LV_FS_SEEK_SET: seek = SeekSet; break;
    case LV_FS_SEEK_CUR: seek = SeekCur; break;
    case LV_FS_SEEK_END: seek = SeekEnd; break;
    default: return LV_FS_RES_UNKNOWN;
  }

  if (f_ptr->seek(pos, seek)) {
    return LV_FS_RES_OK;
  } else {
    return LV_FS_RES_UNKNOWN;
  }
}

static lv_fs_res_t lvbe_fs_size(lv_fs_drv_t * drv, void * file_p, uint32_t * size_p);
static lv_fs_res_t lvbe_fs_size(lv_fs_drv_t * drv, void * file_p, uint32_t * size_p) {
  // AddLog(LOG_LEVEL_INFO, "LVG: lvbe_fs_size(%p, %p, %p)", drv, file_p, size_p);
  File * f_ptr = (File*) file_p;
  *size_p = f_ptr->size();
  return LV_FS_RES_OK;
}
#endif // USE_UFILESYS

/*********************************************************************************************\
 * Memory handler
 * Use PSRAM if available
\*********************************************************************************************/
extern "C" {
  
  //Use the following

  // extern void *lvbe_malloc(size_t size);
  // extern void  lvbe_free(void *ptr);
  // extern void *lvbe_realloc(void *ptr, size_t size);
  // extern void *lvbe_calloc(size_t num, size_t size);
  
  void *lvbe_malloc(uint32_t size);
  void *lvbe_realloc(void *ptr, size_t size);
  void *lvbe_calloc(size_t num, size_t size);
#ifdef USE_BERRY_PSRAM
  void *lvbe_malloc(uint32_t size) {
    return special_malloc(size);
  }
  void *lvbe_realloc(void *ptr, size_t size) {
    return special_realloc(ptr, size);
  }
  void *lvbe_calloc(size_t num, size_t size) {
    return special_calloc(num, size);
  }
#else // USE_BERRY_PSRAM
  void *lvbe_malloc(uint32_t size) {
    return malloc(size);
  }
  void *lvbe_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
  }
  void *lvbe_calloc(size_t num, size_t size) {
    return calloc(num, size);
  }
#endif // USE_BERRY_PSRAM

  void lvbe_free(void *ptr) {
    free(ptr);
  }

#ifdef USE_LVGL_PNG_DECODER
  // for PNG decoder, use same allocators as LVGL
  void* lodepng_malloc(size_t size) { return lvbe_malloc(size); }
  void* lodepng_realloc(void* ptr, size_t new_size) { return lvbe_realloc(ptr, new_size); }
  void lodepng_free(void* ptr) { lvbe_free(ptr); }
#endif // USE_LVGL_PNG_DECODER

}

// ARCHITECTURE-SPECIFIC TIMER STUFF ---------------------------------------

extern void lv_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t * px_map);

// Tick interval for LittlevGL internal timekeeping; 1 to 10 ms recommended
static const int lv_tick_interval_ms = 5;

static void lv_tick_handler(void) { lv_tick_inc(lv_tick_interval_ms); }

// TOUCHSCREEN STUFF -------------------------------------------------------

uint32_t Touch_Status(int32_t sel);

//typedef void (*lv_indev_read_cb_t)(lv_indev_t * indev, lv_indev_data_t * data);
void lvgl_touchscreen_read(lv_indev_t *indev_drv, lv_indev_data_t *data);
void lvgl_touchscreen_read(lv_indev_t *indev_drv, lv_indev_data_t *data) {
  data->point.x = Touch_Status(1); // Last-pressed coordinates
  data->point.y = Touch_Status(2);
  data->state = Touch_Status(0) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  data->continue_reading = false; /*No buffering now so no more data read*/
  // keep data for TS calibration
  lv_ts_calibration.state = data->state;
  if (data->state == LV_INDEV_STATE_PRESSED) {    // if not pressed, the data may be invalid
    lv_ts_calibration.x = data->point.x;
    lv_ts_calibration.y = data->point.y;
    lv_ts_calibration.raw_x = Touch_Status(-1);
    lv_ts_calibration.raw_y = Touch_Status(-2);
  }
}

// Actual RAM usage will be 2X these figures, since using 2 DMA buffers...
#define LV_BUFFER_ROWS 60 // Most others have a bit more space

/************************************************************
 * Initialize the display / touchscreen drivers then launch lvgl
 *
 * We use our own simplified mapping on top of Universal display
 ************************************************************/
extern Renderer *Init_uDisplay(const char *desc);

void start_lvgl(const char * uconfig);
void start_lvgl(const char * uconfig) {

  if (lvgl_glue != nullptr) {
    AddLog(LOG_LEVEL_DEBUG_MORE, D_LOG_LVGL "LVGL was already initialized");
    return;
  }

  if (!renderer || uconfig) {
    renderer  = Init_uDisplay((char*)uconfig);
    AddLog(LOG_LEVEL_ERROR, "LVG: Could not start Universal Display");
    if (!renderer) return;
  }

  renderer->DisplayOnff(true);

  // **************************************************
  // Initialize LVGL
  // **************************************************
  lvgl_glue = new LVGL_Glue;

  // Initialize lvgl_glue, passing in address of display & touchscreen
  lv_init();

  // Allocate LvGL display buffer (x2 because DMA double buffering)
  bool status_ok = true;
  size_t lvgl_buffer_size;
  do {
    uint32_t flushlines = renderer->lvgl_pars()->flushlines;
    if (0 == flushlines) flushlines = LV_BUFFER_ROWS;

    lvgl_buffer_size = renderer->width() * flushlines;
    if (renderer->lvgl_pars()->use_dma) {
      lvgl_buffer_size /= 2;
      if (lvgl_buffer_size < 1000000) {
        // allocate preferably in internal memory which is faster than PSRAM
        AddLog(LOG_LEVEL_DEBUG, "LVG: Allocating buffer2 %i bytes in main memory (flushlines %i)", (lvgl_buffer_size * (LV_COLOR_DEPTH / 8)) / 1024, flushlines);
        lvgl_glue->lv_pixel_buf2 = heap_caps_malloc_prefer(lvgl_buffer_size * (LV_COLOR_DEPTH / 8), 2, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL, MALLOC_CAP_8BIT);
      }
      if (!lvgl_glue->lv_pixel_buf2) {
        status_ok = false;
        break;
      }
    }

    // allocate preferably in internal memory which is faster than PSRAM
    AddLog(LOG_LEVEL_DEBUG, "LVG: Allocating buffer1 %i KB in main memory (flushlines %i)", (lvgl_buffer_size * (LV_COLOR_DEPTH / 8)) / 1024, flushlines);
    lvgl_glue->lv_pixel_buf = heap_caps_malloc_prefer(lvgl_buffer_size * (LV_COLOR_DEPTH / 8), 2, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL, MALLOC_CAP_8BIT);
    if (!lvgl_glue->lv_pixel_buf) {
      status_ok = false;
      break;
    }
  } while (0);

  if (!status_ok) {
    if (lvgl_glue->lv_pixel_buf) {
      free(lvgl_glue->lv_pixel_buf);
      lvgl_glue->lv_pixel_buf = NULL;
    }
    if (lvgl_glue->lv_pixel_buf2) {
      free(lvgl_glue->lv_pixel_buf2);
      lvgl_glue->lv_pixel_buf2 = NULL;
    }
    delete lvgl_glue;
    lvgl_glue = nullptr;
    AddLog(LOG_LEVEL_ERROR, "LVG: Could not allocate buffers");
    return;
  }

  // Initialize LvGL display driver
  lvgl_glue->lv_display = lv_display_create(renderer->width(), renderer->height());
  lv_display_set_dpi(lvgl_glue->lv_display, 160);          // set display to 160 DPI instead of default 130 DPI to avoid some rounding in styles
  lv_display_set_flush_cb(lvgl_glue->lv_display, lv_flush_callback);
  lv_display_set_buffers(lvgl_glue->lv_display, lvgl_glue->lv_pixel_buf, lvgl_glue->lv_pixel_buf2, lvgl_buffer_size * (LV_COLOR_DEPTH / 8), LV_DISPLAY_RENDER_MODE_PARTIAL);

  // Initialize LvGL input device (touchscreen already started)
  lvgl_glue->lv_indev = lv_indev_create();
  lv_indev_set_type(lvgl_glue->lv_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(lvgl_glue->lv_indev, lvgl_touchscreen_read);

  // ESP 32------------------------------------------------
  lvgl_glue->tick.attach_ms(lv_tick_interval_ms, lv_tick_handler);
  // -----------------------------------------

  // Set the default background color of the display
  // This is normally overriden by an opaque screen on top
#ifdef USE_BERRY
  // By default set the display color to black and opacity to 100%
  lv_obj_t * background = lv_layer_bottom();
  lv_obj_set_style_bg_color(background, lv_color_hex(USE_LVGL_BG_DEFAULT), static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_DEFAULT));
  lv_obj_set_style_bg_opa(background, LV_OPA_COVER, static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_DEFAULT));
  // lv_disp_set_bg_color(NULL, lv_color_from_uint32(USE_LVGL_BG_DEFAULT));
  // lv_disp_set_bg_opa(NULL, LV_OPA_COVER);
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(USE_LVGL_BG_DEFAULT), static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_DEFAULT));
  lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_DEFAULT));

#ifdef USE_BERRY_LVGL_PANEL
  berry.lvgl_panel_loaded = false;      // we can load the panel
#endif // USE_BERRY_LVGL_PANEL

#if LV_USE_LOG
  lv_log_register_print_cb(lvbe_debug);
#endif // LV_USE_LOG
#endif

#ifdef USE_UFILESYS
  // Add file system mapping
  static lv_fs_drv_t drv;      // LVGL8, needs to be static and not on stack
  lv_fs_drv_init(&drv);                     /*Basic initialization*/

  drv.letter = 'A';                         /*An uppercase letter to identify the drive */
  drv.ready_cb = nullptr;               /*Callback to tell if the drive is ready to use */
  drv.open_cb = &lvbe_fs_open;                 /*Callback to open a file */
  drv.close_cb = &lvbe_fs_close;               /*Callback to close a file */
  drv.read_cb = &lvbe_fs_read;                 /*Callback to read a file */
  drv.write_cb = &lvbe_fs_write;               /*Callback to write a file */
  drv.seek_cb = &lvbe_fs_seek;                 /*Callback to seek in a file (Move cursor) */
  drv.tell_cb = &lvbe_fs_tell;                 /*Callback to tell the cursor position  */

  drv.dir_open_cb = nullptr;         /*Callback to open directory to read its content */
  drv.dir_read_cb = nullptr;         /*Callback to read a directory's content */
  drv.dir_close_cb = nullptr;       /*Callback to close a directory */
  // drv.user_data = nullptr;             /*Any custom data if required*/

  lv_fs_drv_register(&drv);                 /*Finally register the drive*/

#endif // USE_UFILESYS

#ifdef USE_LVGL_FREETYPE
  // initialize the FreeType renderer
  lv_freetype_init(USE_LVGL_FREETYPE_MAX_FACES);
  // lv_freetype_init(USE_LVGL_FREETYPE_MAX_FACES,
  //                  USE_LVGL_FREETYPE_MAX_SIZES,
  //                  UsePSRAM() ? USE_LVGL_FREETYPE_MAX_BYTES_PSRAM : USE_LVGL_FREETYPE_MAX_BYTES);
#endif
#ifdef USE_LVGL_PNG_DECODER
  lv_lodepng_init();
#endif // USE_LVGL_PNG_DECODER

  // TODO check later about cache size
  if (UsePSRAM()) {
    lv_cache_set_max_size(LV_GLOBAL_DEFAULT()->img_cache, LV_IMG_CACHE_DEF_SIZE_PSRAM, nullptr);
  } else {
    lv_cache_set_max_size(LV_GLOBAL_DEFAULT()->img_cache, LV_IMG_CACHE_DEF_SIZE_NOPSRAM, nullptr);
  }

  AddLog(LOG_LEVEL_INFO, PSTR(D_LOG_LVGL "LVGL initialized"));
}

/*********************************************************************************************\
 * Callable from Berry
\*********************************************************************************************/
bool lvgl_started(void);
bool lvgl_started(void) {
  return (lvgl_glue != nullptr);
}

void lvgl_set_screenshot_file(File * file);
void lvgl_set_screenshot_file(File * file) {
  lvgl_glue->screenshot = file;
}

void lvgl_reset_screenshot_file(void);
void lvgl_reset_screenshot_file(void) {
  lvgl_glue->screenshot = nullptr;
}

File * lvgl_get_screenshot_file(void);
File * lvgl_get_screenshot_file(void) {
  return lvgl_glue->screenshot;
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

// Driver initialization function
void LvglDrvInit(void) {
  AddLog(LOG_LEVEL_INFO, PSTR("LVGL Driver Init"));
  CustomLVGL_Initialize();
}

// Driver loop function (if needed)
void LvglDrvLoop(void) {
  lv_tick_inc(5);         // Adjust this value based on loop interval
  lv_timer_handler();
}

// Register the driver with Tasmota

bool Xdrv54(uint32_t function){

  switch (function) {
    case FUNC_INIT:
      LvglDrvInit();
      break;
    case FUNC_LOOP:
      LvglDrvLoop();
      break;
    case FUNC_MQTT_SUBSCRIBE:             
      LvglMqttSubscribe();                  
      break;
  }

  return false;
}



#endif  // defined(USE_LVGL) && defined(USE_UNIVERSAL_DISPLAY)
#endif  // ESP32

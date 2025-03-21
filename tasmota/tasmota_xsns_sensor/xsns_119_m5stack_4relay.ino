/*
  xsns_99_m5stack_4relay.ino - WebUI button sensor for M5Stack 4-Relay module
  Only display 4 buttons on WebUI for now.
*/

#ifdef USE_I2C
#ifdef USE_M5STACK_4RELAY
#define XSNS_119              119
#define I2C_M5STACK_4RELAY_ADDR  0x26  // Địa chỉ I2C mặc định của module 4-Relay

bool m5relay_present = false;

void M5Relay_ShowButtons(void) {
  // Hiển thị các nút trên WebUI (4 dòng, mỗi dòng 1 nút)
  WSContentSend_P(PSTR("<table style='width:100%%'>"));

  for (uint8_t i = 0; i < 4; i++) {
    WSContentSend_P(PSTR(
      "<tr><td style='width:25%%; text-align:center'>"
      "<button onclick='la(\"&k=%d\")' style='width:80%%; height:40px;'>Relay %d</button>"
      "</td></tr>"
    ), i + 1, i + 1);
  }

  WSContentSend_P(PSTR("</table>"));
}

void M5Relay_WebSensor(void) {
  if (!m5relay_present) return;
  M5Relay_ShowButtons();
}

void M5Relay_EverySecond(void) {
  // Sau này có thể update trạng thái relay từng giây nếu muốn
}

bool M5Relay_Detect(void) {
  if (!I2cDevicePresent(I2C_M5STACK_4RELAY_ADDR)) return false;
  m5relay_present = true;
  return true;
}

void M5Relay_Init(void) {
  if (!M5Relay_Detect()) return;
  AddWebSensor(M5Relay_WebSensor);
  AddEverySecondDriver(M5Relay_EverySecond);
}

#endif  // USE_I2C
#endif
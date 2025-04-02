#ifdef USE_I2C
#ifdef USE_M5STACK_4RELAY



#define XSNS_126                 126
#define I2C_M5STACK_4RELAY_ADDR  0x26  

bool m5relay_present = false;
bool relay_state[4] = {false, false, false, false};


bool M5Relay_DevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0);
}


void M5Relay_Write(uint8_t index, bool state) {
  if (index >= 4) return;
  Wire.beginTransmission(I2C_M5STACK_4RELAY_ADDR);
  Wire.write(index);               
  Wire.write(state ? 1 : 0);       
  Wire.endTransmission();
  relay_state[index] = state;
}

void M5Relay_ShowButtons(void) {
  WSContentSend_P(PSTR("<table style='width:80%%; text-align:left;'>"));

  for (uint8_t i = 0; i < 4; i++) {
    WSContentSend_P(PSTR(
      "<tr>"
      "<td style='width:25%%;'><b>Relay %d</b></td>"
      "<td style='width:25%%;'><button onclick='la(\"&k=%d\")' style='width:100px; height:30px;'>Bật / Tắt</button></td>"
      "<td style='width:50%%;'><b>Trạng thái:</b> <span style='color:%s; font-weight:bold;'>%s</span></td>"
      "</tr>"),
      i + 1, i + 1,
      relay_state[i] ? "lime" : "red",
      relay_state[i] ? "ON" : "OFF"
    );
  }

  WSContentSend_P(PSTR("</table>"));
}


void M5Relay_WebUI(void) {
  if (!m5relay_present) return;

  if (Webserver->hasArg("k")) {
    int relay_index = Webserver->arg("k").toInt() - 1;
    if (relay_index >= 0 && relay_index < 4) {
      bool new_state = !relay_state[relay_index];
      M5Relay_Write(relay_index, new_state);
    }
  }

  M5Relay_ShowButtons();
}



bool M5Relay_Detect(void) {
  if (!M5Relay_DevicePresent(I2C_M5STACK_4RELAY_ADDR)) return false;
  m5relay_present = true;
  return true;
}

void M5Relay_Init(void) {
  if (!M5Relay_Detect()) return;
  
  for (uint8_t i = 0; i < 4; i++) {
    M5Relay_Write(i, false);
  }
}

bool Xsns126(uint32_t function) {
  switch (function) {
    
    case FUNC_INIT:
      M5Relay_Init();
      break;

    #ifdef USE_WEBSERVER
    case FUNC_WEB_SENSOR:
      if (m5relay_present) {   
        M5Relay_WebUI(); 
      }
      break;
    #endif
  
  }
  return false;
}


#endif  // USE_M5STACK_4RELAY
#endif  // USE_I2C

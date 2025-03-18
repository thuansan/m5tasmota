// xdrv_99_m5stack_4relay.ino 
#ifdef USE_I2C
#ifdef USE_M5STACK_4RELAY
#define XDRV_99 99

#include <Wire.h>


#define M5RELAY_I2C_ADDR 0x26 
uint8_t m5relay_state = 0x00;

bool CmndM5Relay(void);

void M5Relay_WriteState(uint8_t state) {
  Wire.beginTransmission(M5RELAY_I2C_ADDR);
  Wire.write(state);
  Wire.endTransmission();
}

void M5Relay_Init(void) {
  Wire.begin();
  M5Relay_WriteState(m5relay_state);
}

bool CmndM5Relay(void) {
  if ((XdrvMailbox.index < 1) || (XdrvMailbox.index > 4)) {
    ResponseCmndError();
    return true;
  }
  if ((XdrvMailbox.payload != 0) && (XdrvMailbox.payload != 1)) {
    ResponseCmndError();
    return true;
  }
  bitWrite(m5relay_state, XdrvMailbox.index - 1, XdrvMailbox.payload);
  M5Relay_WriteState(m5relay_state);
  ResponseCmndDone();
  return true;
}

bool Xdrv99(uint32_t function) {
    switch (function) {
      case FUNC_PRE_INIT:
        M5Relay_Init();
        break;
      case FUNC_COMMAND:
        if (strncasecmp_P(XdrvMailbox.command, PSTR("M5Relay"), 7) == 0) {
          CmndM5Relay();
        }
        break;
    }
    return false;
  }
  

#endif // USE_M5STACK_4RELAY
#endif
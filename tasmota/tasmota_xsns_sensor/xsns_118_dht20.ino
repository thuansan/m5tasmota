#ifdef USE_I2C
#ifdef USE_DHT20
#define XSNS_118              118
#define XI2C_42               42  

#define DHT20_ADDR            0x38

#include "sensor_data.h"

DHT20 Dht20 = {41, 50, 1, 1, "DHT20"};  

bool Dht20Read(void)
{
  if (Dht20.valid) { Dht20.valid--; }

  //0xAC, 0x33, 0x00
  Wire.beginTransmission(DHT20_ADDR);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) return false;

  delay(80); 

  
  Wire.requestFrom(DHT20_ADDR, 1);
  uint8_t status = Wire.read();
  if (status & 0x80) return false; 

  
  Wire.requestFrom(DHT20_ADDR, 7);
  uint8_t data[7];
  for (uint8_t i = 0; i < 7; i++) {
    data[i] = Wire.read();
  }

  
  uint32_t raw_humidity = ((uint32_t)(data[1]) << 12) |
                          ((uint32_t)(data[2]) << 4) |
                          ((uint32_t)(data[3]) >> 4);

 
  uint32_t raw_temp = (((uint32_t)(data[3] & 0x0F)) << 16) |
                      ((uint32_t)(data[4]) << 8) |
                      (uint32_t)(data[5]);

  
  Dht20.humidity = ConvertHumidity((raw_humidity * 100.0) / 1048576.0);
  Dht20.temperature = ConvertTemp(((raw_temp * 200.0) / 1048576.0) - 50.0);

  if (isnan(Dht20.temperature) || isnan(Dht20.humidity)) return false;

  Dht20.valid = SENSOR_MAX_MISS;
  return true;
}

void Dht20Detect(void)
{
  if (!I2cSetDevice(DHT20_ADDR)) return;

  if (Dht20Read()) {
    I2cSetActiveFound(DHT20_ADDR, Dht20.name);
    Dht20.count = 1;
  }
}

void Dht20EverySecond(void)
{
  if (TasmotaGlobal.uptime & 1) {
    if (!Dht20Read()) {
      AddLogMissed(Dht20.name, Dht20.valid);
    }
  }
}



void Dht20Show(bool json)
{
  if (Dht20.valid) {
    TempHumDewShow(json, (0 == TasmotaGlobal.tele_period), Dht20.name, Dht20.temperature, Dht20.humidity);
  }
}

bool Xsns118(uint32_t function)
{
  if (!I2cEnabled(XI2C_42)) return false;

  bool result = false;

  if (FUNC_INIT == function) {
    Dht20Detect();
  }
  else if (Dht20.count) {
    switch (function) {
      case FUNC_EVERY_SECOND:
        Dht20EverySecond();
        break;
      case FUNC_JSON_APPEND:
        Dht20Show(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_SENSOR:
        Dht20Show(0);
        break;
#endif
    }
  }
  return result;
}

#endif  // USE_DHT20
#endif  // USE_I2C

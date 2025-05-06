

#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <stdint.h>
#include <math.h>  

// DHT20
typedef struct {
    float   temperature;
    float   humidity;
    uint8_t valid;
    uint8_t count;
    char    name[6];
} DHT20;

extern DHT20 Dht20;

// // SHT1x
// typedef struct {
//     float   temperature;
//     float   humidity;
//     int8_t  sda_pin;
//     int8_t  scl_pin;
//     uint8_t type;
//     uint8_t valid;
//     char    name[6];
// } SHT1x_Data;

// extern SHT1x_Data Sht1x;

// // CO2 Sensor (ví dụ như SCD30)
// typedef struct {
//     float   co2_ppm;
//     float   temperature;
//     float   humidity;
//     uint8_t valid;
//     char    name[6];
// } CO2_Sensor;

// extern CO2_Sensor Co2Sensor;

// // ENV II Sensor
// typedef struct {
//     float temperature;
//     float humidity;
//     float pressure;
//     uint8_t valid;
//     char name[6];
// } ENVII_Sensor;

// extern ENVII_Sensor EnvII;

#endif  // SENSOR_DATA_H



#ifndef DHT20_H
#define DHT20_H

#include <stdint.h>
#include <math.h>  // Để sử dụng NAN

struct DHT20 {
    float   temperature;
    float   humidity;
    uint8_t valid;
    uint8_t count;
    char    name[6];
};

extern DHT20 Dht20;

#endif

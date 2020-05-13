#ifndef DS3231_H
#define DS3231_H

#include <cstdint>



class DS3231
{
public:
    DS3231();
    DS3231::DS3231(int i2c_bus);
    void init();
    void setA1Time(uint8_t A1Day,uint8_t A1Hour, uint8_t A1Minute, 
                   uint8_t A1Second, uint8_t AlarmBits,bool A1Dy, 
                   bool A1h12, bool A1PM);
    void turnOnAlarm(uint8_t alarm);
    void turnOffAlarm(uint8_t alarm);
    bool checkAlarmEnabled(uint8_t alarm);

private:
    int fd;
    int bus;

    // converter decimanl numbers to binary coded decimal
    uint8_t decToBcd(uint8_t val);
    uint8_t bcdToDec(uint8_t val);
    uint8_t readControlByte(bool which);
    void writeControlByte(uint8_t control, bool which);
};

#endif
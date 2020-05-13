#include <cstdint>

int selectDevice(uint8_t dev_addr)
{
   int s;

    s = ioctl(fd, I2C_SLAVE, dev_addr);

    if (s == -1)
    {
       perror("selectDevice");
    }

    return s;
}

// Stubbed functions  for use on other systems than raspberry
#include "bcm2835.h"


int bcm2835_init(void) {
}

int bcm2835_spi_begin(void) {
}

void bcm2835_spi_setClockDivider(uint16_t divider) {

}

bcm2835_spi_setDataMode(uint8_t mode)
{
}

void bcm2835_spi_chipSelect(uint8_t cs)
{
}

void bcm2835_spi_transfern(char* buf, uint32_t len)
{
}

int bcm2835_close(void)
{
}

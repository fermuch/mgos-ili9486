#ifndef __MGOS_ILI9486_HAL_H
#define __MGOS_ILI9486_HAL_H

#include "mgos.h"
#include "mgos_spi.h"

// buf represents a 16-bit RGB 565 uint16_t color buffer of length buflen bytes (so buflen/2 pixels).
// Note: data in 'buf' has to be in network byte order!
void ili9486_send_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *buf, uint32_t buflen);

#endif  // __MGOS_ILI9486_HAL_H
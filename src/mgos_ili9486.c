#include "mgos.h"

#include <stdint.h>

#include "mgos_spi.h"
#include "mgos_gpio.h"

#define ILI9486_SLEEP 0x80
#define SPI_MODE 0

static uint8_t initialization_sequence[] = {
  0x11,       ILI9486_SLEEP,      120,                        // sw reset
  0x3A,       1,                  0x55,                       // interface pixel format
  0xC2,       1,                  0x44,                       // power control 3 (normal mode)
  0xC5,       4,                  0x00, 0x00, 0x00, 0x00,     // VCOM control
  0xE0,       15,                 0x0F, 0x1F, 0x1C, 0x0C,     // PGAMCTRL (positive gamma control)
                                  0x0F, 0x08, 0x48, 0x98,
                                  0x37, 0x0A, 0x13, 0x04,
                                  0x11, 0x0D, 0x00,
  0xE1,       15,                 0x0F, 0x32, 0x2E, 0x0B,     // NGAMCTRL (negative gamma correction)
                                  0x0D, 0x05, 0x47, 0x75,
                                  0x37, 0x06, 0x10, 0x03,
                                  0x24, 0x20, 0x00,
  0x20,       0,                                              // display inversion OFF
  // 0x21,       0,                                           // display inversion ON
  0x36,       1,                  0x48,                       // memory access control
  0x29,       ILI9486_SLEEP,      150
};

static void ili9486_spi_write(const uint8_t *data, uint32_t size) {
  struct mgos_spi *spi = mgos_spi_get_global();

  if (!spi) {
    LOG(LL_ERROR, ("SPI is disabled, set spi.enable=true"));
    return;
  }

  struct mgos_spi_txn txn = {
    .cs   = mgos_sys_config_get_ili9486_cs_index(),
    .mode = SPI_MODE,
    .freq = mgos_sys_config_get_ili9486_spi_freq(),
  };
  txn.hd.tx_data   = data,
  txn.hd.tx_len    = size,
  txn.hd.dummy_len = 0,
  txn.hd.rx_len    = 0,
  txn.hd.rx_data   = NULL,
  mgos_spi_run_txn(spi, false, &txn);
}

static void ili9486_send_commands(const uint8_t *addr) {
  uint8_t numArgs, cmd, delay;

  while (true) {                                        // For each command...
    cmd = *addr++;                                      // save command
    // 0 is a NOP, so technically a valid command, but why on earth would one want it in the list
    if (cmd == 0x00) {
      break;
    }
    numArgs  = *addr++;                                 // Number of args to follow
    delay    = numArgs & ILI9486_SLEEP;                 // If high bit set, delay follows args
    numArgs &= ~ILI9486_SLEEP;                          // Mask out delay bit

    mgos_gpio_write(mgos_sys_config_get_ili9486_dc_pin(), 0);
    ili9486_spi_write(&cmd, 1);

    mgos_gpio_write(mgos_sys_config_get_ili9486_dc_pin(), 1);
    ili9486_spi_write((uint8_t *)addr, numArgs);
    addr += numArgs;

    if (delay) {
      delay = *addr++;               // Read post-command delay time (ms)
      mgos_msleep(delay);
    }
  }
}

bool mgos_ili9486_init(void) {
  // set up DC pin
  mgos_gpio_write(mgos_sys_config_get_ili9486_dc_pin(), 0);
  mgos_gpio_set_mode(mgos_sys_config_get_ili9486_dc_pin(), MGOS_GPIO_MODE_OUTPUT);

  LOG(LL_INFO, ("ILI9486 init (CS%d, DC: %d, RST: %d, MODE: %d, FREQ: %d)",
                mgos_sys_config_get_ili9486_cs_index(),
                mgos_sys_config_get_ili9486_dc_pin(),
                mgos_sys_config_get_ili9486_rst_pin(),
                SPI_MODE, mgos_sys_config_get_ili9486_spi_freq()));

  if (mgos_sys_config_get_ili9486_rst_pin() >= 0) {
    // Issue a 20uS negative pulse on the reset pin and wait 5 mS so interface gets ready.
    mgos_gpio_write(mgos_sys_config_get_ili9486_rst_pin(), 1);
    mgos_gpio_set_mode(mgos_sys_config_get_ili9486_rst_pin(), MGOS_GPIO_MODE_OUTPUT);
    mgos_usleep(1000);
    mgos_gpio_write(mgos_sys_config_get_ili9486_rst_pin(), 0);
    mgos_usleep(20);
    mgos_gpio_write(mgos_sys_config_get_ili9486_rst_pin(), 1);
  }

  ili9486_send_commands(initialization_sequence);

  return true;
}

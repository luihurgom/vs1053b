// SPDX-License-Identifier: MIT
#include "wokwi-api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>  

// Estado mínimo del "dispositivo"
typedef struct {
  pin_t xcs, xdcs, dreq, rst;
  spi_dev_t spi;
  uint8_t buf[256];
} chip_state_t;

static void chip_spi_done(void *user_data, uint8_t *buffer, uint32_t count) {
  (void)buffer;
  (void)count;
  chip_state_t *s = (chip_state_t *)user_data;
  // Mantén DREQ alto (siempre listo)
  pin_write(s->dreq, HIGH);
  // Si aún está seleccionado, sigue tragando bytes
  if (pin_read(s->xcs) == LOW || pin_read(s->xdcs) == LOW) {
    spi_start(s->spi, s->buf, sizeof(s->buf));
  }
}

static void cs_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *s = (chip_state_t *)user_data;
  if (value == LOW) {
    // Al seleccionar (bajar CS), empezamos a recibir
    pin_write(s->dreq, HIGH);
    spi_start(s->spi, s->buf, sizeof(s->buf));
  } else {
    // Al deseleccionar (subir CS), paramos
    spi_stop(s->spi);
  }
}

void chip_init() {
  chip_state_t *s = (chip_state_t *)malloc(sizeof(chip_state_t));
  memset(s, 0, sizeof(*s));

  // Pines
  s->xcs  = pin_init("XCS",  INPUT_PULLUP);
  s->xdcs = pin_init("XDCS", INPUT_PULLUP);
  s->dreq = pin_init("DREQ", OUTPUT_HIGH); // siempre listo
  s->rst  = pin_init("RST",  INPUT_PULLUP);

  pin_t sck  = pin_init("SCK",  INPUT);
  pin_t mosi = pin_init("MOSI", INPUT);
  pin_t miso = pin_init("MISO", OUTPUT_LOW);

  // SPI "dispositivo"
  const spi_config_t cfg = {
    .sck = sck, .mosi = mosi, .miso = miso, .mode = 0,
    .done = chip_spi_done, .user_data = s
  };
  s->spi = spi_init(&cfg);

  // Vigila los dos CS (control/datos)
  const pin_watch_config_t w = { .edge = BOTH, .pin_change = cs_change, .user_data = s };
  pin_watch(s->xcs, &w);
  pin_watch(s->xdcs, &w);

  printf("VS1053B stub inicializado.\n");
}


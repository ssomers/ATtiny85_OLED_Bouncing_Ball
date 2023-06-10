#pragma once
#include "Glyph.h"
#include "OLED.h"

namespace GlyphsOnQuarter {
  
static constexpr uint8_t DIGIT_MARGIN = 1;
static constexpr uint8_t WIDTH_3DIGITS = 3 * (DIGIT_MARGIN + Glyph::SEGS + DIGIT_MARGIN);
static constexpr uint8_t WIDTH_4DIGITS = 4 * (DIGIT_MARGIN + Glyph::SEGS + DIGIT_MARGIN);

template <typename Device>
void send(OLED::QuarterChat<Device>& chat, Glyph const& glyph, uint8_t margin = 0) {
  chat.sendSpacing(margin);
  for (uint8_t x = 0; x < Glyph::SEGS; ++x) {
    byte seg = glyph.seg(x);
    chat.send(seg << 4, seg >> 4);
  }
  chat.sendSpacing(margin);
}

template <typename Device>
void send3digits(OLED::QuarterChat<Device>& chat, uint8_t number) {
  uint8_t const p1 = number / 100;
  uint8_t const p2 = number % 100;
  send(chat, Glyph::digit[p1],      DIGIT_MARGIN);
  send(chat, Glyph::digit[p2 / 10], DIGIT_MARGIN);
  send(chat, Glyph::digit[p2 % 10], DIGIT_MARGIN);
}

template <typename Device>
void send4digits(OLED::QuarterChat<Device>& chat, int number) {
  constexpr uint8_t NUM_ERR_GLYPHS = 5;
  static_assert(WIDTH_4DIGITS == NUM_ERR_GLYPHS * Glyph::SEGS);
  if (number < 0) {
    for (uint8_t _ = 0; _ < NUM_ERR_GLYPHS; ++_) {
      send(chat, Glyph::minus, 0);
    }
    return;
  }
  uint8_t const p1 = number / 100;
  uint8_t const p2 = number % 100;
  if (p1 >= 100) {
    for (uint8_t _ = 0; _ < NUM_ERR_GLYPHS; ++_) {
      send(chat, Glyph::overflow, 0);
    }
    return;
  }
  send(chat, Glyph::digit[p1 / 10], DIGIT_MARGIN);
  send(chat, Glyph::digit[p1 % 10], DIGIT_MARGIN);
  send(chat, Glyph::digit[p2 / 10], DIGIT_MARGIN);
  send(chat, Glyph::digit[p2 % 10], DIGIT_MARGIN);
}

}

#pragma once
#include "Glyph.h"
#include "OLED.h"

struct GlyphsOnQuarter {
  static constexpr uint8_t MARGIN = 1;
  static constexpr uint8_t WIDTH_3DIGITS = 3 * (MARGIN + Glyph::SEGS + MARGIN);
  static constexpr uint8_t WIDTH_4DIGITS = 4 * (MARGIN + Glyph::SEGS + MARGIN);

  template <typename Device>
  static void send(OLED::QuarterChat<Device>& chat, Glyph const& glyph, uint8_t margin = 0) {
    chat.sendSpacing(margin);
    for (uint8_t x = 0; x < Glyph::SEGS; ++x) {
      byte seg = glyph.seg(x);
      chat.send(seg << 4, seg >> 4);
    }
    chat.sendSpacing(margin);
  }

  template <typename Device>
  static void send3digits(OLED::QuarterChat<Device>& chat, uint8_t number) {
    uint8_t const p1 = number / 100;
    uint8_t const p2 = number % 100;
    send(chat, Glyph::digit[p1], MARGIN);
    send(chat, Glyph::digit[p2 / 10], MARGIN);
    send(chat, Glyph::digit[p2 % 10], MARGIN);
  }

  template <typename Device>
  static void send4digits(OLED::QuarterChat<Device>& chat, int number) {
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
    send(chat, Glyph::digit[p1 / 10], MARGIN);
    send(chat, Glyph::digit[p1 % 10], MARGIN);
    send(chat, Glyph::digit[p2 / 10], MARGIN);
    send(chat, Glyph::digit[p2 % 10], MARGIN);
  }
};

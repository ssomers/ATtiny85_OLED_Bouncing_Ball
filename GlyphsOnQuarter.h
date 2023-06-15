#pragma once
#include "Glyph.h"
#include "OLED.h"

namespace GlyphsOnQuarter {

static constexpr uint8_t DIGIT_MARGIN = 1;
static constexpr uint8_t DIGITS_WIDTH(uint8_t N) {
  return N * (DIGIT_MARGIN + Glyph::SEGS + DIGIT_MARGIN);
}

template <typename Device>
void sendTo(OLED::QuarterChat<Device>& chat, Glyph const& glyph, uint8_t margin = 0) {
  chat.sendSpacing(margin);
  for (uint8_t x = 0; x < Glyph::SEGS; ++x) {
    byte seg = glyph.seg(x);
    chat.send(seg << 4, seg >> 4);
  }
  chat.sendSpacing(margin);
}

template <typename Device>
void send2hex(OLED::QuarterChat<Device>& chat, uint8_t number) {
  sendTo(chat, Glyph::hex_digit_hi(number), DIGIT_MARGIN);
  sendTo(chat, Glyph::hex_digit_lo(number), DIGIT_MARGIN);
}

template <typename Device>
void send4hex(OLED::QuarterChat<Device>& chat, uint16_t number) {
  sendTo(chat, Glyph::hex_digit_hi(uint8_t(number >> 8)), DIGIT_MARGIN);
  sendTo(chat, Glyph::hex_digit_lo(uint8_t(number >> 8)), DIGIT_MARGIN);
  sendTo(chat, Glyph::hex_digit_hi(uint8_t(number)), DIGIT_MARGIN);
  sendTo(chat, Glyph::hex_digit_lo(uint8_t(number)), DIGIT_MARGIN);
}

template <typename Device>
void send3dec(OLED::QuarterChat<Device>& chat, uint8_t number) {
  uint8_t p1 = number / 100;
  uint8_t p2 = number % 100;
  sendTo(chat, Glyph::dec_digit[p1],      DIGIT_MARGIN);
  sendTo(chat, Glyph::dec_digit[p2 / 10], DIGIT_MARGIN);
  sendTo(chat, Glyph::dec_digit[p2 % 10], DIGIT_MARGIN);
}

template <typename Device>
void send4dec(OLED::QuarterChat<Device>& chat, int number) {
  constexpr uint8_t NUM_ERR_GLYPHS = 5;
  static_assert(DIGITS_WIDTH(4) == NUM_ERR_GLYPHS * Glyph::SEGS);
  if (number < 0) {
    for (uint8_t _ = 0; _ < NUM_ERR_GLYPHS; ++_) {
      sendTo(chat, Glyph::minus);
    }
    return;
  }
  uint8_t p1 = number / 100;
  uint8_t p2 = number % 100;
  if (p1 >= 100) {
    for (uint8_t _ = 0; _ < NUM_ERR_GLYPHS; ++_) {
      sendTo(chat, Glyph::X);
    }
    return;
  }
  sendTo(chat, Glyph::dec_digit[p1 / 10], DIGIT_MARGIN);
  sendTo(chat, Glyph::dec_digit[p1 % 10], DIGIT_MARGIN);
  sendTo(chat, Glyph::dec_digit[p2 / 10], DIGIT_MARGIN);
  sendTo(chat, Glyph::dec_digit[p2 % 10], DIGIT_MARGIN);
}

}

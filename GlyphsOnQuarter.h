#pragma once
#include "Glyph.h"
#include "OLED.h"

template <typename Device>
class GlyphsOnQuarter : public OLED::QuarterChat<Device> {
    using super = OLED::QuarterChat<Device>;
  private:
    static constexpr byte HEARTBEAT_SEG1 = GlyphExtractor::extractSeg("  # # # ");
    static constexpr byte HEARTBEAT_SEG2 = GlyphExtractor::extractSeg("# # #   ");

    // Should be OLED::Quarter, but that cannot be narrowed until gcc 9.3.
    // Should be const, but AutoFormat screws up.
    uint8_t quarter_bit : 4;
    bool include_heartbeat : 1;

    bool toggle_heartbeat() {
      if (include_heartbeat) {
        include_heartbeat = false;
        static uint8_t heartbeat_per_quarter = 0b0000;
        heartbeat_per_quarter ^= quarter_bit;
        return heartbeat_per_quarter & quarter_bit;
      } else {
        return false;
      }
    }

  public:
    // start_location is merely the initial value of a counter for error reporting.
    explicit GlyphsOnQuarter(uint8_t start_location,
                             OLED::Quarter quarter, uint8_t xBegin = 0, uint8_t xEnd = OLED::WIDTH - 1,
                             bool include_heartbeat = true)
      : super(start_location, quarter, xBegin, xEnd)
      , quarter_bit(uint8_t(1 << static_cast<uint8_t>(quarter)))
      , include_heartbeat(include_heartbeat) {
    }

    GlyphsOnQuarter& send(byte seg, uint8_t times = 1) {
      for (uint8_t x = 0; x < times; ++x) {
        if (toggle_heartbeat()) {
          super::send(seg << 4 | HEARTBEAT_SEG1,
                      seg >> 4 | HEARTBEAT_SEG2);
        } else {
          super::send(seg << 4,
                      seg >> 4);
        }
      }
      return *this;
    }

    GlyphsOnQuarter& send(Glyph const& glyph, uint8_t margin = 0) {
      send(0, margin);
      for (uint8_t x = 0; x < Glyph::SEGS; ++x) {
        send(glyph.seg(x));
      }
      send(0, margin);
      return *this;
    }

    GlyphsOnQuarter& sendColon() {
      send(0, Glyph::DIGIT_MARGIN);
      send(Glyph::COLON_SEG, Glyph::POINT_WIDTH - 2 * Glyph::DIGIT_MARGIN);
      send(0, Glyph::DIGIT_MARGIN);
      return *this;
    }

    GlyphsOnQuarter& sendPoint() {
      send(0, Glyph::DIGIT_MARGIN);
      send(Glyph::POINT_SEG, Glyph::POINT_WIDTH - 2 * Glyph::DIGIT_MARGIN);
      send(0, Glyph::DIGIT_MARGIN);
      return *this;
    }

    GlyphsOnQuarter& send2hex(uint8_t number) {
      send(Glyph::hex_digit_hi(number), Glyph::DIGIT_MARGIN);
      send(Glyph::hex_digit_lo(number), Glyph::DIGIT_MARGIN);
      return *this;
    }

    GlyphsOnQuarter& send4hex(uint16_t number) {
      send(Glyph::hex_digit_hi(uint8_t(number >> 8)), Glyph::DIGIT_MARGIN);
      send(Glyph::hex_digit_lo(uint8_t(number >> 8)), Glyph::DIGIT_MARGIN);
      send(Glyph::hex_digit_hi(uint8_t(number >> 0)), Glyph::DIGIT_MARGIN);
      send(Glyph::hex_digit_lo(uint8_t(number >> 0)), Glyph::DIGIT_MARGIN);
      return *this;
    }

    GlyphsOnQuarter& send3dec(uint8_t number) {
      uint8_t p1 = number / 100;
      uint8_t p2 = number % 100;
      if (p1 != 0) {
        send(Glyph::dec_digit[p1], Glyph:: DIGIT_MARGIN);
      } else {
        send(0, Glyph::DIGIT_WIDTH);
      }
      if (p1 != 0 || p2 >= 10) {
        send(Glyph::dec_digit[p2 / 10], Glyph::DIGIT_MARGIN);
      } else {
        send(0, Glyph::DIGIT_WIDTH);
      }
      send(Glyph::dec_digit[p2 % 10], Glyph::DIGIT_MARGIN);
      return *this;
    }

    GlyphsOnQuarter& send4dec(int number) {
      if (number < 0) {
        send(Glyph::MINUS_SEG, Glyph::DIGIT_WIDTH * 4);
        return *this;
      }
      uint8_t p1 = number / 100;
      uint8_t p2 = number % 100;
      if (p1 >= 100) {
        send(~0, Glyph::DIGIT_WIDTH * 4);
        return *this;
      }
      if (p1 >= 10) {
        send(Glyph::dec_digit[p1 / 10], Glyph::DIGIT_MARGIN);
      } else {
        send(0, Glyph::DIGIT_WIDTH);
      }
      if (p1 != 0) {
        send(Glyph::dec_digit[p1 % 10], Glyph::DIGIT_MARGIN);
      } else {
        send(0, Glyph::DIGIT_WIDTH);
      }
      if (p1 != 0 || p2 >= 10) {
        send(Glyph::dec_digit[p2 / 10], Glyph::DIGIT_MARGIN);
      } else {
        send(0, Glyph::DIGIT_WIDTH);
      }
      send(Glyph::dec_digit[p2 % 10], Glyph::DIGIT_MARGIN);
      return *this;
    }
};

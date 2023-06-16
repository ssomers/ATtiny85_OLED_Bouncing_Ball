#pragma once
#include <Arduino.h>

class Glyph {
    friend class GlyphPair;

  public:
    static uint8_t constexpr SEGS = 8;
    // Separate decimanl and hex arrays so that the latter is not linked in unless the main code references it.
    static Glyph PROGMEM const dec_digit[10];
    static Glyph PROGMEM const ABCDEF[6];
    static Glyph PROGMEM const X;
    static Glyph PROGMEM const minus;
    static Glyph PROGMEM const plus;
    static Glyph PROGMEM const colon;

    static Glyph const& hex_digit_hi(uint8_t n) {
      return hex_digit(n >> 4);
    }

    static Glyph const& hex_digit_lo(uint8_t n) {
      return hex_digit(n & 0xF);
    }

  private:
    static Glyph const& hex_digit(uint8_t n) {
      return n < 10 ? dec_digit[n] : ABCDEF[n - 10];
    }

    byte const seg0;
    byte const seg1;
    byte const seg2;
    byte const seg3;
    byte const seg4;
    byte const seg5;
    byte const seg6;
    byte const seg7;

    static constexpr bool pixel(char c) {
      return c != ' ';
    }

    static constexpr byte extractSegAt(int x, const char* col_per_row, int glyph_index, int glyph_count) {
      return 0
             | pixel(col_per_row[x + SEGS * (glyph_index + glyph_count * 0)]) << 0
             | pixel(col_per_row[x + SEGS * (glyph_index + glyph_count * 1)]) << 1
             | pixel(col_per_row[x + SEGS * (glyph_index + glyph_count * 2)]) << 2
             | pixel(col_per_row[x + SEGS * (glyph_index + glyph_count * 3)]) << 3
             | pixel(col_per_row[x + SEGS * (glyph_index + glyph_count * 4)]) << 4
             | pixel(col_per_row[x + SEGS * (glyph_index + glyph_count * 5)]) << 5
             | pixel(col_per_row[x + SEGS * (glyph_index + glyph_count * 6)]) << 6
             | pixel(col_per_row[x + SEGS * (glyph_index + glyph_count * 7)]) << 7;
    }

    // Private to keep all instances in this class and as PROGMEM.
    constexpr Glyph(const char* col_per_row, int glyph_index = 0, int glyph_count = 1)
      : seg0(extractSegAt(0, col_per_row, glyph_index, glyph_count))
      , seg1(extractSegAt(1, col_per_row, glyph_index, glyph_count))
      , seg2(extractSegAt(2, col_per_row, glyph_index, glyph_count))
      , seg3(extractSegAt(3, col_per_row, glyph_index, glyph_count))
      , seg4(extractSegAt(4, col_per_row, glyph_index, glyph_count))
      , seg5(extractSegAt(5, col_per_row, glyph_index, glyph_count))
      , seg6(extractSegAt(6, col_per_row, glyph_index, glyph_count))
      , seg7(extractSegAt(7, col_per_row, glyph_index, glyph_count))
    {}

  public:
    byte seg(uint8_t x) const {
      switch (x) {
        case 0: return pgm_read_byte(&seg0);
        case 1: return pgm_read_byte(&seg1);
        case 2: return pgm_read_byte(&seg2);
        case 3: return pgm_read_byte(&seg3);
        case 4: return pgm_read_byte(&seg4);
        case 5: return pgm_read_byte(&seg5);
        case 6: return pgm_read_byte(&seg6);
        case 7: return pgm_read_byte(&seg7);
      }
      __builtin_unreachable();
    }
};

class GlyphPair {
  public:
    static GlyphPair PROGMEM const cm;
    static GlyphPair PROGMEM const pin;

    Glyph const left;
    Glyph const right;

  private:
    constexpr GlyphPair(const char* col_per_row)
      : left(col_per_row, 0, 2)
      , right(col_per_row, 1, 2)
    {}
};

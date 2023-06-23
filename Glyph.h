#pragma once
#include <Arduino.h>


namespace GlyphExtractor {

// Convert ascii art character to pixel value.
static constexpr bool pixel(char c) {
  return c != ' ';
}

// Extract display food for one column of one glyph from ascii art representing a sequence of glyphs.
static constexpr byte extractSegAt(const char* art, int glyph_index, int glyph_count, int x, int segs_per_glyph) {
  return 0
         | pixel(art[x + segs_per_glyph * (glyph_index + glyph_count * 0)]) << 0
         | pixel(art[x + segs_per_glyph * (glyph_index + glyph_count * 1)]) << 1
         | pixel(art[x + segs_per_glyph * (glyph_index + glyph_count * 2)]) << 2
         | pixel(art[x + segs_per_glyph * (glyph_index + glyph_count * 3)]) << 3
         | pixel(art[x + segs_per_glyph * (glyph_index + glyph_count * 4)]) << 4
         | pixel(art[x + segs_per_glyph * (glyph_index + glyph_count * 5)]) << 5
         | pixel(art[x + segs_per_glyph * (glyph_index + glyph_count * 6)]) << 6
         | pixel(art[x + segs_per_glyph * (glyph_index + glyph_count * 7)]) << 7;
}

// Convert ascii art representing a column to display food.
static constexpr byte extractSeg(const char* column) {
  return extractSegAt(column, 0, 1, 0, 1);
}

}

class Glyph {
    friend class GlyphPair;

  public:
    static uint8_t constexpr SEGS = 8;
    static constexpr uint8_t DIGIT_MARGIN = 1;
    static constexpr uint8_t DIGIT_WIDTH = DIGIT_MARGIN + SEGS + DIGIT_MARGIN;
    static constexpr uint8_t COLON_WIDTH = DIGIT_MARGIN + 2 + DIGIT_MARGIN;
    static constexpr uint8_t POINT_WIDTH = DIGIT_MARGIN + 2 + DIGIT_MARGIN;

    // Separate decimal and hex arrays so that each is linked in only if the main code references it.
    static Glyph PROGMEM const dec_digit[10];
    static Glyph PROGMEM const ABCDEF[6];
    static Glyph PROGMEM const X;
    static Glyph PROGMEM const at;
    static Glyph PROGMEM const plus;
    static byte constexpr COLON_SEG = GlyphExtractor::extractSeg(" ##  ## ");
    static byte constexpr MINUS_SEG = GlyphExtractor::extractSeg("   ##   ");
    static byte constexpr POINT_SEG = GlyphExtractor::extractSeg("      ##");

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

    // Construct display food from ascii art.
    // Private to keep all instances in this class and as PROGMEM.
    constexpr Glyph(const char* art, int glyph_index = 0, int glyph_count = 1)
      : seg0(GlyphExtractor::extractSegAt(art, glyph_index, glyph_count, 0, SEGS))
      , seg1(GlyphExtractor::extractSegAt(art, glyph_index, glyph_count, 1, SEGS))
      , seg2(GlyphExtractor::extractSegAt(art, glyph_index, glyph_count, 2, SEGS))
      , seg3(GlyphExtractor::extractSegAt(art, glyph_index, glyph_count, 3, SEGS))
      , seg4(GlyphExtractor::extractSegAt(art, glyph_index, glyph_count, 4, SEGS))
      , seg5(GlyphExtractor::extractSegAt(art, glyph_index, glyph_count, 5, SEGS))
      , seg6(GlyphExtractor::extractSegAt(art, glyph_index, glyph_count, 6, SEGS))
      , seg7(GlyphExtractor::extractSegAt(art, glyph_index, glyph_count, 7, SEGS))
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
    static GlyphPair PROGMEM const m;
    static GlyphPair PROGMEM const err;
    static GlyphPair PROGMEM const pin;

    Glyph const left;
    Glyph const right;

  private:
    constexpr GlyphPair(const char* col_per_row)
      : left(col_per_row, 0, 2)
      , right(col_per_row, 1, 2)
    {}
};

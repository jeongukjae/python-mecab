#ifndef _MECAB_UCS_H_
#define _MECAB_UCS_H_

namespace MeCab {

// All internal codes are represented in UCS2,
// if you want to use specific local codes, e.g, big5/euc-kr,
// make a function which maps the local code to the UCS code.

inline unsigned short utf8_to_ucs2(const char* begin, const char* end, size_t* mblen) {
  const size_t len = end - begin;

  if (static_cast<unsigned char>(begin[0]) < 0x80) {
    *mblen = 1;
    return static_cast<unsigned char>(begin[0]);

  } else if (len >= 2 && (begin[0] & 0xe0) == 0xc0) {
    *mblen = 2;
    return ((begin[0] & 0x1f) << 6) | (begin[1] & 0x3f);

  } else if (len >= 3 && (begin[0] & 0xf0) == 0xe0) {
    *mblen = 3;
    return ((begin[0] & 0x0f) << 12) | ((begin[1] & 0x3f) << 6) | (begin[2] & 0x3f);

    /* belows are out of UCS2 */
  } else if (len >= 4 && (begin[0] & 0xf8) == 0xf0) {
    *mblen = 4;
    return 0;

  } else if (len >= 5 && (begin[0] & 0xfc) == 0xf8) {
    *mblen = 5;
    return 0;

  } else if (len >= 6 && (begin[0] & 0xfe) == 0xfc) {
    *mblen = 6;
    return 0;

  } else {
    *mblen = 1;
    return 0;
  }
}

inline unsigned short ascii_to_ucs2(const char* begin, const char* end, size_t* mblen) {
  *mblen = 1;
  return static_cast<unsigned char>(begin[0]);
}

inline unsigned short utf16be_to_ucs2(const char* begin, const char* end, size_t* mblen) {
  const size_t len = end - begin;
  if (len <= 1) {
    *mblen = 1;
    return 0;
  }
  *mblen = 2;
#if defined WORDS_BIGENDIAN
  return (begin[0] << 8 | begin[1]);
#else
  return (begin[1] << 8 | begin[0]);
#endif
  return 0;
}

inline unsigned short utf16le_to_ucs2(const char* begin, const char* end, size_t* mblen) {
  const size_t len = end - begin;
  if (len <= 1) {
    *mblen = 1;
    return 0;
  }
  *mblen = 2;
#ifdef WORDS_BIGENDIAN
  return (begin[1] << 8 | begin[0]);
#else
  return (begin[0] << 8 | begin[1]);
#endif
}

inline unsigned short utf16_to_ucs2(const char* begin, const char* end, size_t* mblen) {
#ifdef WORDS_BIGENDIAN
  return utf16be_to_ucs2(begin, end, mblen);
#else
  return utf16le_to_ucs2(begin, end, mblen);
#endif
}
}  // namespace MeCab

#endif  // _MECAB_UCS_H_

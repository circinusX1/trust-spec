#ifndef CRC32SUM_T_H
#define CRC32SUM_T_H

#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <future>
#include <memory>
#include <regex>
#include <iostream>
#include <sstream>

/**
 */
#if defined(__SSE4_2__) && defined(__GNUC__)   // GCC/Clang
#  include <wmmintrin.h>      // _mm_crc32_* are in here on older gcc
#  include <nmmintrin.h>      // newer gcc puts them here
#elif defined(_MSC_VER) && defined(__AVX2__)   // MSVC
#  include <intrin.h>
#endif

/**
 * crc32
 */
namespace crc32 {

/**
 * @brief crc32_u8
 * @param crc
 * @param v
 * @return
 */
[[gnu::always_inline]] inline uint32_t crc32_u8(uint32_t crc, uint8_t v)
{
#if defined(__SSE4_2__)
    return _mm_crc32_u8(crc, v);
#else
    // Software fallback (same polynomial as hardware)
    crc ^= v;
    for (int i = 0; i < 8; ++i)
        crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320U : 0);
    return crc;
#endif
}

/**
 * @brief crc32_u32
 * @param crc
 * @param v
 * @return
 */
[[gnu::always_inline]] inline uint32_t crc32_u32(uint32_t crc, uint32_t v) {
#if defined(__SSE4_2__)
    return _mm_crc32_u32(crc, v);
#else
    crc = crc32_u8(crc, (uint8_t)(v));
    crc = crc32_u8(crc, (uint8_t)(v >> 8));
    crc = crc32_u8(crc, (uint8_t)(v >> 16));
    return crc32_u8(crc, (uint8_t)(v >> 24));
#endif
}

/**
 * @brief crc32_u64
 * @param crc
 * @param v
 * @return
 */
[[gnu::always_inline]] inline uint32_t crc32_u64(uint32_t crc, uint64_t v) {
#if defined(__SSE4_2__) && (defined(__x86_64__) || defined(_M_X64))
    return _mm_crc32_u64(crc, v);
#else
    crc = crc32_u32(crc, (uint32_t)(v));
    return crc32_u32(crc, (uint32_t)(v >> 32));
#endif
}

/**
 *
/* 16-byte SIMD helper – safe even without AVX2
 */
[[gnu::always_inline]] inline uint32_t process_16bytes(uint32_t crc, const uint8_t* p) {
    uint64_t a = *reinterpret_cast<const uint64_t*>(p);
    uint64_t b = *reinterpret_cast<const uint64_t*>(p + 8);
    crc = crc32_u64(crc, a);
    return crc32_u64(crc, b);
}

} // namespace crc32



#endif // CRC32SUM_T_H

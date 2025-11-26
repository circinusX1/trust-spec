#ifndef PARALEL_CRC_T_H
#define PARALEL_CRC_T_H

#include "crc32sum_t.h"
#include <filesystem>


namespace  fs = std::filesystem;

/**
 * @brief crc_file_parallel
 * @param path
 * @param n_threads
 * @param length
 * @param what
 * @return
 */
uint32_t crc_file_parallel(const fs::path& path, size_t n_threads, uint64_t& length, const int what );

/**
 * @brief The CRC32 class
 */
struct CRC32
{
    static constexpr size_t ALIGN = 4096;
    static constexpr size_t CHUNK_SIZE = 64ULL << 20; // 64 MiB

    alignas(64) static uint32_t shift_table[ALIGN / sizeof(uint32_t)];

    static void init_shift_table() {
        static bool initialized = false;
        if (initialized) return;

        uint32_t crc = 0;
        for (size_t i = 0; i < ALIGN; i += 4)
        {
            crc = crc32::crc32_u32(crc, 0);
            shift_table[i / 4] = crc;
        }
        initialized = true;
    }

    static uint32_t crc32_u8(uint32_t crc, uint8_t v) {
        return crc32::crc32_u8(crc, v);
    }

    static uint32_t crc32_u32(uint32_t crc, uint32_t v) {
        return crc32::crc32_u32(crc, v);
    }

    static uint32_t crc32_u64(uint32_t crc, uint64_t v) {
        return crc32::crc32_u64(crc, v);
    }

    static uint32_t process_16bytes(uint32_t crc, const uint8_t* p) {
        uint64_t v1 = *reinterpret_cast<const uint64_t*>(p);
        uint64_t v2 = *reinterpret_cast<const uint64_t*>(p + 8);
        crc = crc32_u64(crc, v1);
        crc = crc32_u64(crc, v2);
        return crc;
    }

    static uint32_t compute(const uint8_t* data, uint64_t len) {
        uint32_t crc = ~0U;
        const uint8_t* p = data;
        const uint8_t* end = data + len;

        while ((reinterpret_cast<uintptr_t>(p) & 15) && p < end) {
            crc = crc32_u8(crc, *p++);
        }

        while (p + 16 <= end) {
            crc = process_16bytes(crc, p);
            p += 16;
        }

        while (p < end) {
            crc = crc32_u8(crc, *p++);
        }

        return ~crc;
    }

    static uint32_t combine(uint32_t crc1, uint32_t crc2, uint64_t len2) {
        if (len2 == 0) return crc1;
        size_t idx = len2 % ALIGN;
        uint32_t shift = shift_table[idx / sizeof(uint32_t)];
        return crc1 ^ crc32_u32(shift, crc2);
    }
};


#endif // PARALEL_CRC_T_H

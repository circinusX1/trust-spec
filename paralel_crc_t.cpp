#include "paralel_crc_t.h"
#include "map_file_mem_t.h"



alignas(64) uint32_t CRC32::shift_table[CRC32::ALIGN / sizeof(uint32_t)];


// -------------------------------------------------------------
// Parallel CRC Computation
// -------------------------------------------------------------
uint32_t crc_file_parallel(const fs::path& path, size_t n_threads, uint64_t& length, int flag )
{
    CRC32::init_shift_table();
    uint32_t final_crc = 0;
    map_file_t file(path);

    if (file.size() == 0) return ~0U;

    if (n_threads == 0) {
        n_threads = std::thread::hardware_concurrency();
    }

    const uint64_t chunk_size = CRC32::CHUNK_SIZE;
    const uint64_t file_size = file.size();
    const uint64_t n_chunks = (file_size + chunk_size - 1) / chunk_size;
    length = file_size;
    n_threads = std::min((uint64_t)n_threads, n_chunks > 0 ? n_chunks : 1);

    if(flag & 0x4)
    {

        struct Task {
            uint64_t offset;
            uint64_t length;
            std::promise<uint32_t> promise;
        };

        std::vector<Task> tasks;
        tasks.reserve(n_chunks);

        uint64_t pos = 0;
        for (uint64_t i = 0; i < n_chunks; ++i)
        {
            uint64_t len = (i + 1 == n_chunks) ? file_size - pos : chunk_size;
            tasks.push_back({pos, len, {}});
            pos += len;
        }

        // Launch workers
        std::vector<std::thread> workers;
        workers.reserve(n_threads);

        std::atomic<size_t> next_task{0};
        for (size_t i = 0; i < n_threads; ++i)
        {
            workers.emplace_back([&]() {
                while (true)
                {
                    size_t idx = next_task++;
                    if (idx >= tasks.size()) break;

                    auto& task = tasks[idx];
                    uint32_t crc = CRC32::compute(file.data() + task.offset, task.length);
                    task.promise.set_value(crc);
                }
            });
        }

        // Collect results
        std::vector<std::future<uint32_t>> futures;
        futures.reserve(tasks.size());
        for (auto& task : tasks) {
            futures.push_back(task.promise.get_future());
        }

        for (auto& t : workers) {
            t.join();
        }

        // Combine CRCs
        auto it = futures.begin();
        final_crc = it->get();
        uint64_t prefix_len = tasks[0].length;
        ++it;

        for (; it != futures.end(); ++it) {
            uint32_t chunk_crc = it->get();
            size_t task_idx = it - futures.begin();
            final_crc = CRC32::combine(final_crc, chunk_crc, tasks[task_idx].length);
            prefix_len += tasks[task_idx].length;
        }
    }

    return final_crc;
}


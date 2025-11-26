#ifndef MAP_FILE_MEM_T_H
#define MAP_FILE_MEM_T_H

#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/mman.h>

namespace  fs = std::filesystem;

/**
 * @brief The map_file_t class
 */
class map_file_t
{
    uint64_t size_{0};
    uint8_t* data_{nullptr};
    int fd_{ -1 };

public:
    explicit map_file_t(const fs::path& path)
    {
        if (!fs::exists(path))
        {
            throw std::runtime_error("File not found: " + path.string());
        }

        fd_ = open(path.c_str(), O_RDONLY);
        if (fd_ == -1)
        {
            throw std::runtime_error("open() failed");
        }

        struct stat st{};
        if (fstat(fd_, &st) != 0)
        {
            throw std::runtime_error("fstat() failed");
        }
        size_ = static_cast<uint64_t>(st.st_size);

        if (size_ == 0) return;

        data_ = static_cast<uint8_t*>(
            mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0)
            );
        if (data_ == MAP_FAILED) {
            throw std::runtime_error("mmap() failed");
        }
    }

    ~map_file_t()
    {
        if (data_)
        {
            munmap(data_, size_);
            if (fd_ != -1) close(fd_);
        }
    }

    [[nodiscard]] uint64_t size() const noexcept { return size_; }
    [[nodiscard]] const uint8_t* data() const noexcept { return data_; }

    // Non-copyable
    map_file_t(const map_file_t&) = delete;
    map_file_t& operator=(const map_file_t&) = delete;
};

#endif // MAP_FILE_MEM_T_H

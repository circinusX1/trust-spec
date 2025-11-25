// fast_crc_huge.cpp - Fast CRC32 for huge files (C++17, SIMD, multi-threaded)
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <future>
#include <memory>
#include <regex>
#include <iostream>
#include <sstream>
#include <chrono>
#include <string>
#include <regex>
#include <vector>
#include <cstring>
#include <cstdint>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "ini_file.h"
#include <cstdint>
#if defined(__SSE4_2__) && defined(__GNUC__)   // GCC/Clang
#  include <wmmintrin.h>      // _mm_crc32_* are in here on older gcc
#  include <nmmintrin.h>      // newer gcc puts them here
#elif defined(_MSC_VER) && defined(__AVX2__)   // MSVC
#  include <intrin.h>
#endif


#define PORT 15390
#define IP_ADDRESS "127.0.0.1"

namespace crc32 {

[[gnu::always_inline]] inline uint32_t crc32_u8(uint32_t crc, uint8_t v) {
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

[[gnu::always_inline]] inline uint32_t crc32_u64(uint32_t crc, uint64_t v) {
#if defined(__SSE4_2__) && (defined(__x86_64__) || defined(_M_X64))
    return _mm_crc32_u64(crc, v);
#else
    crc = crc32_u32(crc, (uint32_t)(v));
    return crc32_u32(crc, (uint32_t)(v >> 32));
#endif
}

/* 16-byte SIMD helper – safe even without AVX2 */
[[gnu::always_inline]] inline uint32_t process_16bytes(uint32_t crc, const uint8_t* p) {
    uint64_t a = *reinterpret_cast<const uint64_t*>(p);
    uint64_t b = *reinterpret_cast<const uint64_t*>(p + 8);
    crc = crc32_u64(crc, a);
    return crc32_u64(crc, b);
}

} // namespace crc32



std::regex wildcard_to_regex(const std::string& wildcard)
{
    std::string pattern = wildcard;

    pattern = std::regex_replace(pattern, std::regex("[\\.\\+\\(\\)\\[\\]\\{\\}\\^\\$\\|\\\\]"), "\\$&");
    pattern = std::regex_replace(pattern, std::regex("\\*"), ".*");
    pattern = std::regex_replace(pattern, std::regex("\\?"), ".");
    pattern = "^" + pattern + "$";
    return std::regex(pattern, std::regex::ECMAScript | std::regex::icase); // icase for case-insensitive matching
}

class map_file_t {
    uint64_t size_{0};
    uint8_t* data_{nullptr};
    int fd_{ -1 };

public:
    explicit map_file_t(const fs::path& path)
    {
        if (!fs::exists(path)) {
            throw std::runtime_error("File not found: " + path.string());
        }

        fd_ = open(path.c_str(), O_RDONLY);
        if (fd_ == -1) {
            throw std::runtime_error("open() failed");
        }

        struct stat st{};
        if (fstat(fd_, &st) != 0) {
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

    ~map_file_t() {
        if (data_) {
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

// -------------------------------------------------------------
// SIMD CRC32 (AVX2/SSE4.2)
// -------------------------------------------------------------
struct CRC32 {
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

        // Align to 16-byte boundary
        while ((reinterpret_cast<uintptr_t>(p) & 15) && p < end) {
            crc = crc32_u8(crc, *p++);
        }

        // 16-byte SIMD loop
        while (p + 16 <= end) {
            crc = process_16bytes(crc, p);
            p += 16;
        }

        // Remainder
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

alignas(64) uint32_t CRC32::shift_table[CRC32::ALIGN / sizeof(uint32_t)];

// -------------------------------------------------------------
// Parallel CRC Computation
// -------------------------------------------------------------
uint32_t crc_file_parallel(const fs::path& path, size_t n_threads, uint64_t& length, const std::string& what )
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

    if(what.find('C')!=std::string::npos)
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


std::vector<fs::path> find_files_recursive(
    const fs::path& root_path,
    const std::string& wildcard_pattern)
{
    std::vector<fs::path> matching_files;
    std::regex file_regex = wildcard_to_regex(wildcard_pattern);
    std::string prev;
    std::error_code ec;
    try {


        for (const auto& entry : fs::recursive_directory_iterator(root_path, fs::directory_options::skip_permission_denied, ec))
        {
            if (entry.is_regular_file())
            {

                std::string filename = entry.path().filename().string();
                if (std::regex_match(filename, file_regex))
                {
                    matching_files.push_back(entry.path());
                }
                prev=entry.path().parent_path().string()+"/"+entry.path().filename().string();
            }
        }
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Filesystem error: " << prev << ", " << e.what() << std::endl;
    }

    return matching_files;
}

inline size_t split_str(const std::string &str, char delim,
                        std::vector<std::string> &a)
{
    if (str.empty())
        return 0;

    std::string token;
    size_t prev = 0, pos = 0;
    size_t sl = str.length();
    a.clear();
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos)
            pos = sl;
        token = str.substr(prev, pos - prev);
        if (!token.empty())
            a.push_back(token);
        prev = pos + 1;
    } while (pos < sl && prev < sl);
    return a.size();
}

inline std::string proc_exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (pipe)
    {
        try {
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                result += buffer;
            }
        } catch (...) {
            pclose(pipe);
            std::cout << "Exception " << cmd;
            throw;
        }
    }
    pclose(pipe);
    return result;
}

///////////////////////////////////////////////////////////////////////////////
static int sockfd;
static struct sockaddr_in servaddr;
template <typename... Args>
void usend(Args&&... args)
{
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    oss.str();
    ::sendto(sockfd,oss.str().c_str(),oss.str().length(),0,(const struct sockaddr *) &servaddr, sizeof(servaddr));
}

// -------------------------------------------------------------
void remove_crlf(std::string& s) {
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
    s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
}

// -------------------------------------------------------------
// Main
// -------------------------------------------------------------
int main(int argc, char* argv[])
{
    ini_file_t&  cfg = ini_file_t::get(argv[0],fs::path(argv[0]).filename().stem().c_str());
    size_t threads = ::atoi(cfg.key("threads").c_str());
    std::string paths = cfg.key("path");
    std::string comands = cfg.key("command");
    std::string ignores = cfg.key("ignore");
    std::vector<std::string> vstrings;
    std::vector<std::string> vignores;
    std::string fout = cfg.key("output");
    std::fstream  iofile;
    std::string    pkey;

    bool create = false;
    int rv = 0;

    if(argc < 4)     // create
    {
        std::cout << "create/verify sumfile <pkey/PKEY>\n";
        return 0;
    }


    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        perror("socket creation failed");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, IP_ADDRESS, &servaddr.sin_addr) <= 0)
    {
        perror("invalid address/address not supported");
        close(sockfd);
        return 1;
    }
    ::usend(argv[0]," ",argv[1]," ",argv[2]," ",argv[3]);

    // $0 CREATE HASH PK
    if(!::strcmp(argv[1],"CREATE"))
    {
        iofile.open(argv[2],std::ios::out);
        pkey = argv[3];
        create = true;
    }

    else if(!::strcmp(argv[1],"VERIFY"))
    {
        // $0 VERIFY HASH PK
        std::string sig = argv[2]; sig += ".SIG";

        pkey = argv[3];
        if(fs::is_regular_file(fs::path(sig.c_str())) && fs::is_regular_file(fs::path(argv[2])))
        {
            std::string cmd = "/usr/bin/openssl dgst -sha256 -verify ";
            cmd += pkey;
            cmd += " -signature ";
            cmd += sig;
            cmd += " ";
            cmd += argv[2];
            std::cout << cmd << "\n";
            int ret = ::system(cmd.c_str());
            if( WEXITSTATUS(ret) != 0 )
            {
                ::usend("failed to verify ", sig.c_str());
                std::cerr << "Signature for "<<sig << " failed \n";
                return 1;
            }
        }
        iofile.open(argv[2],std::ios::in);
    }
    int lineno = 0;
    auto start = std::chrono::steady_clock::now();
    if(iofile.is_open())
    {
        try{
            split_str(paths,'\n',vstrings);
            split_str(ignores,'\n',vignores);

            for(const auto& p : vstrings)
            {
                std::cout << p << std::endl;
                std::vector<std::string> parts;
                ::split_str(p,',',parts);
                if(parts.size()!=3)
                {
                    std::cerr << "Invalid path line: " << p << std::endl;
                    continue;
                }
                fs::path search_directory = parts[0];
                std::string pattern = parts[1];
                std::string what = parts[2];

                std::vector<fs::path> results = find_files_recursive(search_directory, pattern);

                for(const auto& path : results)
                {
                    if(path.filename().string().find("mtab") != -1)
                    {
                         std::cout << "ignore file: " << path.filename().string() << std::endl;
                    }
                    if(std::find(vignores.begin(), vignores.end(), path.filename().string())!=vignores.end())
                    {
                        std::cout << "ignore file: " << path.filename().string() << std::endl;
                        continue;
                    }
                    try
                    {
                        size_t   fsz;
                        fs::file_time_type ftime = fs::last_write_time(path);
                        uint32_t crc = crc_file_parallel(path, threads, fsz, what);
                        if(create)
                        {
                            iofile << lineno << ":" << path.parent_path().string()<<"/"<< path.filename().string() <<
                                " " << fsz <<
                                " " << (uint64_t)ftime.time_since_epoch().count() <<
                                " " << std::hex << std::setw(8) << std::setfill('0') << crc << std::dec << std::endl;
                            if(lineno %100==0)
                                ::usend("create line: " , lineno);
                        }
                        else
                        {
                            std::stringstream out;
                            std::string line;

                            std::getline(iofile, line);
                            out << lineno << ":" << path.parent_path().string()<<"/"<< path.filename().string() <<
                                " " << fsz <<
                                " " << (uint64_t)ftime.time_since_epoch().count() <<
                                " " << std::hex << std::setw(8) << std::setfill('0') << crc << std::dec;
                            std::string curent = out.str();
                            if(curent != line)
                            {
                                std::cerr << curent << std::endl;
                                std::cerr << lineno << ", " << line << std::endl;
                                ::usend("error: ",lineno,", cnt: ", line, "=", curent);
                                std::cerr << "Invalid checkum at: " << path.filename() << std::endl;
                                throw 1;
                            }
                            if(lineno %100==0)
                                ::usend("check line: ",  lineno);
                        }
                        ++lineno;
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Error: " << path.string() <<", "<< e.what() << std::endl;
                    }
                }
            }

            vstrings.clear();
            split_str(comands,'\n',vstrings);
            for(const auto& p : vstrings)
            {
                std::vector<std::string> cmd_parts;
                ::split_str(p,',', cmd_parts);

                std::cout << cmd_parts[0] << std::endl;

                std::string line;
                std::string proc_exec;
                std::string proc_exec1  = ::proc_exec(cmd_parts[0].c_str());
                std::stringstream sb;
                sb << proc_exec1;
                cmd_parts.erase(cmd_parts.begin()); // remove command);

                while (std::getline(sb, line))
                {
                    for(const auto& re : cmd_parts)
                    {
                        std::regex r(re);
                        std::smatch m;
                        std::regex_search(line, m, r);
                        if(m.size()!=0)
                        {
                            std::cout << line << " rejected \n";
                            continue;
                        }
                    }
                    proc_exec.append(line);
                }

                if(!proc_exec.empty())
                {
                    if(create)
                    {
                        iofile << p << ": " << proc_exec << std::endl;
                    }
                    else
                    {
                        std::stringstream out;
                        std::string line;

                        out << p << ": " << proc_exec << std::endl;
                        std::getline(iofile, line);
                        std::string curent = out.str();

                        while(curent.back()=='\n')curent.pop_back();
                        while(line.back()=='\n')line.pop_back();

                        if(curent != line)
                        {
                            std::cerr << curent << std::endl;
                            std::cerr << line << std::endl;
                            std::cerr << "Invalid checkum at: " << p << std::endl;
                            ::usend(line);
                            throw 2;
                        }
                    }
                }
            }
        }catch(int i)
        {
            rv = i;
        }
    }
    else
    {
        ::usend("no out file");
    }
    iofile.close();

    if(create)
    {
        if( fs::is_regular_file(fs::path(argv[2])))
        {
            std::string cmd = "/usr/bin/openssl dgst -sha256 -sign ";
            std::string sig = argv[2]; sig += ".SIG";
            cmd += pkey;
            cmd += " -out ";
            cmd += sig;
            cmd += " ";
            cmd += argv[2];
            std::cout << cmd << "\n";
            usend(cmd);
            ::system(cmd.c_str());
            if(fs::is_regular_file(fs::path(sig.c_str())))
            {
                std::cout << "Signed: " <<  sig << std::endl;
                ::usend("HASH signed");
            }
            else
            {
                std::cerr << "Signed Failed: " <<  sig << std::endl;
                ::usend("HASH sign failed");
            }
        }
        else
        {
            std::cerr << "Failed to create check file: " << argv[2] << std::endl;
            ::usend("cannot find ", argv[2]);
        }
    }
    ::usend("DONE");
    close(sockfd);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "time:" << duration.count() << "ms \n";
    return rv;
}




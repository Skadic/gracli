#pragma once

#include <fcntl.h>
#include <string>
#include <unistd.h>

namespace gracli {

class FileAccess {
    const std::string path;
    const int         file;
    const size_t      file_size;

    FileAccess(const std::string &path, int file, size_t file_size) : path{path}, file{file}, file_size{file_size} {}

  public:
    ~FileAccess() { close(file); }

    static inline auto from_file(const std::string &path) -> FileAccess {
        const int    file      = open64(path.c_str(), O_RDONLY);
        const size_t file_size = lseek64(file, 0, SEEK_END);
        lseek64(file, 0, SEEK_SET);
        return {path, file, file_size};
    }

    inline auto source_length() const -> size_t { return file_size; }

    inline auto at(size_t i) const -> char {
        char c;
        pread64(file, &c, 1, i);
        return c;
    }

    inline auto substr(char *buf, size_t i, size_t l) const -> char * {
        const ssize_t bytes_read = pread64(file, buf, l, i);
        if (bytes_read > 0) {
            return buf + bytes_read;
        } else {
            return buf;
        }
    }
};

} // namespace gracli

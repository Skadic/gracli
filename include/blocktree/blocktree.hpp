#pragma once

#include <compressed/CBlockTree.h>
#include <pointer_based/BlockTree.h>

namespace gracli {

class BlockTreeRandomAccess {
    std::unique_ptr<CBlockTree> m_cbt;
    size_t m_source_length;
    BlockTreeRandomAccess(CBlockTree *bt, size_t source_length) : m_cbt{bt}, m_source_length{source_length} {}

public:
    static auto from_file(const std::string &path) -> BlockTreeRandomAccess {
        std::ifstream ifs(path);

        uint64_t len = 0;
        ifs.read(reinterpret_cast<char*>(&len), sizeof(uint64_t));
        auto *cbt = new CBlockTree(ifs);
        
        return BlockTreeRandomAccess(cbt, len);
    }

    inline auto at(size_t i) -> char {
        return (char) m_cbt->access(i);
    }

    inline auto substr(char* buf, size_t i, size_t len) -> char* {
        // TODO The susbtring method described in the paper is not implemented for block trees
        for (size_t j = i; j < i + len && j < source_length(); j++) {
            *(buf++) = at(j);
        }
        return buf;
    }

    inline auto source_length() -> size_t {
        return this->m_source_length;
    }
};

} // namespace gracli

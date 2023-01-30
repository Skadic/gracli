#pragma once

#include <compressed/CBlockTree.h>
#include <pointer_based/BlockTree.h>

namespace gracli {

class BlockTreeRandomAccess {
    std::unique_ptr<CBlockTree> m_cbt;
    BlockTreeRandomAccess(CBlockTree *bt) : m_cbt{bt} {}

public:
    static auto from_file(const std::string &path) -> BlockTreeRandomAccess {
        std::ifstream ifs(path);

        auto *cbt = new CBlockTree(ifs);
        
        return BlockTreeRandomAccess(cbt);
    }

    inline auto at(size_t i) -> char {
        return (char) m_cbt->access(i);
    }

    inline auto substr(char* buf, size_t i, size_t len) -> char* {
        return m_cbt->substr(buf, i, len);
    }

    inline auto source_length() -> size_t {
        return this->m_cbt->input_size_;
    }
};

} // namespace gracli

//
// Created by skadic on 04.10.22.
//
#include "lzend/lzend.hpp"
#include <gtest/gtest.h>
#include <filesystem>

const std::string FOX_IN_SOCKS = "test_data/fox.txt";

TEST(lzend_test, random_access_test) {
    auto source_path     = std::filesystem::absolute(FOX_IN_SOCKS);
    auto compressed_path = source_path.string() + ".lzend";
    ASSERT_TRUE(std::filesystem::exists(source_path)) << "Test file " << source_path << " does not exist";
    ASSERT_TRUE(std::filesystem::exists(compressed_path)) << "Test file " << compressed_path << " does not exist";

    std::ifstream ifs(source_path);
    std::noskipws(ifs);
    std::string s;
    ifs >> s;
    auto lzend = gracli::lz::LzEnd::from_file(compressed_path);

    for (size_t i = 0; i < s.length(); i++) {
        ASSERT_EQ(s.at(i), lzend.at(i)) << "Incorrect random access at index " << i;
    }
}

TEST(lzend_test, substring_test) {
    auto source_path     = std::filesystem::absolute(FOX_IN_SOCKS);
    auto compressed_path = source_path.string() + ".lzend";
    ASSERT_TRUE(std::filesystem::exists(source_path)) << "Test file " << source_path << " does not exist";
    ASSERT_TRUE(std::filesystem::exists(compressed_path)) << "Test file " << compressed_path << " does not exist";

    std::ifstream ifs(source_path);
    std::noskipws(ifs);
    std::string s;
    ifs >> s;
    auto lzend = gracli::lz::LzEnd::from_file(compressed_path);
    auto n = s.length();

    char buf1[n + 1];
    char buf2[n + 1];

    for (size_t l = 1; l < n; l++) {
        buf1[l] = 0;
        buf2[l] = 0;
        for(size_t i = 0; i < n - l + 1; i++) {
            std::copy(s.begin() + i, s.begin() + i + l, buf1);
            lzend.substr(buf2, i, l);
            ASSERT_EQ(strcmp(buf1, buf2), 0) << "Incorrect substring at index " << i << " with length " << l << ": \"" << const_cast<const char*>(buf1) << "\" vs \"" << const_cast<const char*>(buf2) << "\"";
        }
    }
}

TEST(lzend_test, decode_test) {
    using namespace gracli::lz;
    // Get the expected data constructed directly from the source text
    auto source_path = std::filesystem::absolute(FOX_IN_SOCKS);
    // Get the data decoded from the parsed file
    auto compressed_path = source_path.string() + ".lzend";
    ASSERT_TRUE(std::filesystem::exists(source_path)) << "Test file " << source_path << " does not exist in the test directory";
    ASSERT_TRUE(std::filesystem::exists(compressed_path)) << "Test file" << compressed_path << "does not exist in the test directory";

    std::ifstream stream(source_path, std::ios::binary);
    std::noskipws(stream);
    std::vector<LzEnd::Char> input((std::istream_iterator<LzEnd::Char>(stream)), std::istream_iterator<LzEnd::Char>());
    auto s1 = input.size();
    LzEnd::Parsing l1;
    compute_lzend<LzEnd::Char, LzEnd::TextOffset>(input.data(), input.size(), &l1);

    auto [l2, s2] = gracli::lz::decode(compressed_path);

    ASSERT_EQ(l1.size(), l2.size()) << "Different number of factors";
    ASSERT_EQ(s1, s2) << "Different text size";

    for(size_t i = 0; i < l1.size(); i++) {
        LzEnd::Phrase p1 = l1[i];
        LzEnd::Phrase p2 = l2[i];
        ASSERT_EQ(p1.m_char, p2.m_char) << "Character of factor " << i << " is different";
        ASSERT_EQ(p1.m_link, p2.m_link) << "Source of factor " << i << " is different";
        ASSERT_EQ(p1.m_len, p2.m_len) << "Length of factor " << i << " is different";
    }
}

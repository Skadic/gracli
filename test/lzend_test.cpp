//
// Created by skadic on 04.10.22.
//
#include "lzend/lzend.hpp"
#include <gtest/gtest.h>
#include <filesystem>

TEST(lzend_test, random_access_test) {
    std::string s = "The quick brown fox jumps over the lazy dog";
    auto lzend = gracli::lz::LzEnd::from_string(s);

    for (size_t i = 0; i < s.length(); i++) {
        ASSERT_EQ(s.at(i), lzend.at(i)) << "Incorrect random access at index " << i;
    }
}

TEST(lzend_test, substring_test) {
    std::string s = "The quick brown fox jumps over the lazy dog";
    auto lzend = gracli::lz::LzEnd::from_string(s);

    char buf1[s.length() + 1];
    char buf2[s.length() + 1];

    for (size_t l = 1; l < s.length(); l++) {
        buf1[l] = 0;
        buf2[l] = 0;
        for(size_t i = 0; i < s.length() - l + 1; i++) {
            std::copy(s.begin() + i, s.begin() + i + l, buf1);
            lzend.substr(buf2, i, l);
            ASSERT_EQ(strcmp(buf1, buf2), 0) << "Incorrect substring at index " << i << " with length " << l << ": \"" << const_cast<const char*>(buf1) << "\" vs \"" << const_cast<const char*>(buf2) << "\"";
        }
    }
}

TEST(lzend_test, decode_test) {
    using namespace gracli::lz;
    ASSERT_TRUE(std::filesystem::exists("test_data/fox.txt")) << "Test file test_data/fox.txt does not exist in the test directory";
    ASSERT_TRUE(std::filesystem::exists("test_data/fox.txt.lzend")) << "Test file test_data/fox.txt.lzend does not exist in the test directory";

    // Get the expected data constructed directly from the source text
    auto absolute_path = std::filesystem::absolute("test_data/fox.txt");
    std::ifstream stream(absolute_path, std::ios::binary);
    std::noskipws(stream);
    std::vector<LzEnd::Char> input((std::istream_iterator<LzEnd::Char>(stream)), std::istream_iterator<LzEnd::Char>());
    auto s1 = input.size();
    LzEnd::Parsing l1;
    compute_lzend<LzEnd::Char, LzEnd::TextOffset>(input.data(), input.size(), &l1);

    // Get the data decoded from the parsed file
    auto compressed = absolute_path.string() + ".lzend";
    auto [l2, s2] = gracli::lz::decode(compressed);

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

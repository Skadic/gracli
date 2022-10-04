//
// Created by skadic on 04.10.22.
//
#include <lzend.hpp>
#include <gtest/gtest.h>

TEST(lzend_test, random_access_test) {
    std::string s = "The quick brown fox jumps over the lazy dog";
    auto lzend = gracli::lz::LzEnd::from_string(s);

    for (int i = 0; i < s.length(); i++) {
        ASSERT_EQ(s.at(i), lzend.at(i)) << "Incorrect random access at index " << i;
    }
}

TEST(lzend_test, substring_test) {
    std::string s = "The quick brown fox jumps over the lazy dog";
    auto lzend = gracli::lz::LzEnd::from_string(s);

    char buf1[s.length() + 1];
    char buf2[s.length() + 1];

    for (int l = 1; l < s.length(); l++) {
        buf1[l] = 0;
        buf2[l] = 0;
        for(int i = 0; i < s.length() - l + 1; i++) {
            std::copy(s.begin() + i, s.begin() + i + l, buf1);
            lzend.substr(buf2, i, l);
            ASSERT_EQ(strcmp(buf1, buf2), 0) << "Incorrect substring at index " << i << " with length " << l << ": \"" << const_cast<const char*>(buf1) << "\" vs \"" << const_cast<const char*>(buf2) << "\"";
        }
    }
}

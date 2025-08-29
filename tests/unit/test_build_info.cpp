/**
 * @file test_build_info.cpp
 * @brief Verify compile-time git hash matches repository HEAD.
 */

#include <gtest/gtest.h>
#include <array>
#include <cstdio>
#include <memory>
#include <string>

#ifndef GARLIC_GIT_HASH
#define GARLIC_GIT_HASH "unknown"
#endif

static std::string run_cmd(const char *cmd)
{
    std::array<char, 256> buf{};
    std::string out;
    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        return out;
    }
    while (fgets(buf.data(), buf.size(), pipe) != nullptr) {
        out += buf.data();
    }
    pclose(pipe);
    // strip trailing newlines
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) {
        out.pop_back();
    }
    return out;
}

TEST(BuildInfo, GitHashMacroPresent)
{
    // Macro should be set (possibly to "unknown")
    ASSERT_GT(std::string(GARLIC_GIT_HASH).size(), 0u);
}

TEST(BuildInfo, GitHashMatchesHEAD)
{
    // Try to get repository HEAD short hash from current directory
    std::string head = run_cmd("git rev-parse --short=12 HEAD");
    if (head.empty()) {
        GTEST_SKIP() << "git not available or not a repo";
    }
    EXPECT_EQ(head, std::string(GARLIC_GIT_HASH));
}


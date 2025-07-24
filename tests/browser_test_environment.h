#pragma once

#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "Debug.h"

// Global Browser instance
extern std::unique_ptr<Browser> g_browser;

class BrowserTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override;
    void TearDown() override;
};

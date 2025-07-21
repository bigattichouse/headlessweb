#include <gtest/gtest.h>
#include "../../src/hweb/Services/ManagerRegistry.h"

class ManagerRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure clean state
        HWeb::ManagerRegistry::cleanup();
    }
    
    void TearDown() override {
        HWeb::ManagerRegistry::cleanup();
    }
};

TEST_F(ManagerRegistryTest, InitializationAndCleanup) {
    EXPECT_FALSE(HWeb::ManagerRegistry::is_initialized());
    
    HWeb::ManagerRegistry::initialize();
    EXPECT_TRUE(HWeb::ManagerRegistry::is_initialized());
    
    HWeb::ManagerRegistry::cleanup();
    EXPECT_FALSE(HWeb::ManagerRegistry::is_initialized());
}

TEST_F(ManagerRegistryTest, ManagerAccess) {
    HWeb::ManagerRegistry::initialize();
    
    // Test that we can get references to all managers
    EXPECT_NO_THROW({
        auto& assertion_mgr = HWeb::ManagerRegistry::get_assertion_manager();
        auto& upload_mgr = HWeb::ManagerRegistry::get_upload_manager();
        auto& download_mgr = HWeb::ManagerRegistry::get_download_manager();
        
        // Verify they're not null by calling a method
        assertion_mgr.setSilentMode(true);
        upload_mgr.setMaxFileSize(1000000);
        download_mgr.setDefaultTimeout(30000);
    });
}

TEST_F(ManagerRegistryTest, AccessWithoutInitializationThrows) {
    EXPECT_THROW(HWeb::ManagerRegistry::get_assertion_manager(), std::runtime_error);
    EXPECT_THROW(HWeb::ManagerRegistry::get_upload_manager(), std::runtime_error);
    EXPECT_THROW(HWeb::ManagerRegistry::get_download_manager(), std::runtime_error);
}

TEST_F(ManagerRegistryTest, MultipleInitializationsAreHandled) {
    HWeb::ManagerRegistry::initialize();
    EXPECT_TRUE(HWeb::ManagerRegistry::is_initialized());
    
    // Second initialization should be safe
    HWeb::ManagerRegistry::initialize();
    EXPECT_TRUE(HWeb::ManagerRegistry::is_initialized());
    
    // Should still be able to access managers
    EXPECT_NO_THROW({
        auto& mgr = HWeb::ManagerRegistry::get_assertion_manager();
        mgr.setSilentMode(false);
    });
}
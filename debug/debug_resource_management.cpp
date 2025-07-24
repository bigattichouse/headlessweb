#include <gtest/gtest.h>
#include "../src/hweb/Services/ManagerRegistry.h"
#include "../src/hweb/Services/SessionService.h"
#include "../src/hweb/Services/NavigationService.h"
#include "../src/Browser/Browser.h"
#include "../src/Session/Manager.h"
#include "../tests/utils/test_helpers.h"
#include "../src/hweb/Config.h"
#include "../src/hweb/Types.h"
#include <memory>
#include <thread>
#include <chrono>
#include <filesystem>
#include <gtk/gtk.h>

class DebugResourceManagementTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!gtk_is_initialized_) {
            gtk_init();
            gtk_is_initialized_ = true;
        }
        
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("debug_resource_management_test");
        session_manager_ = std::make_unique<SessionManager>(temp_dir->getPath());
        session_service_ = std::make_unique<HWeb::SessionService>(*session_manager_);
        navigation_service_ = std::make_unique<HWeb::NavigationService>();
        
        HWeb::HWebConfig test_config;
        test_config.allow_data_uri = true;
        browser_ = std::make_unique<Browser>(test_config);
        
        HWeb::ManagerRegistry::initialize();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        browser_.reset();
        navigation_service_.reset();
        session_service_.reset();
        session_manager_.reset();
        HWeb::ManagerRegistry::cleanup();
        temp_dir.reset();
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::unique_ptr<SessionManager> session_manager_;
    std::unique_ptr<HWeb::SessionService> session_service_;
    std::unique_ptr<HWeb::NavigationService> navigation_service_;
    std::unique_ptr<Browser> browser_;
    static bool gtk_is_initialized_;
};

bool DebugResourceManagementTest::gtk_is_initialized_ = false;

TEST_F(DebugResourceManagementTest, SimplifiedConcurrentAccess) {
    Session concurrent_session = session_service_->initialize_session("concurrent_test");
    std::cout << "Attempting to navigate to URL..." << std::endl;
    bool nav_started = navigation_service_->navigate_to_url(*browser_, "data:text/html,<h1>Concurrent Test</h1>");
    ASSERT_TRUE(nav_started);
    std::cout << "Navigation started. Waiting for completion..." << std::endl;

    // Process GTK events while waiting for navigation
    auto start_time = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::milliseconds(5000);
    bool nav_complete = false;
    GMainContext *context = g_main_context_default();
    
    while (!nav_complete && (std::chrono::steady_clock::now() - start_time) < timeout) {
        while (g_main_context_pending(context)) {
            g_main_context_iteration(context, FALSE);
        }
        nav_complete = navigation_service_->wait_for_navigation_complete(*browser_, 100);
        if (!nav_complete) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    ASSERT_TRUE(nav_complete);
    std::cout << "Navigation completed." << std::endl;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

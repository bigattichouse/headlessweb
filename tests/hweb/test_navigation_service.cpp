#include <gtest/gtest.h>
#include "../../src/hweb/Services/NavigationService.h"

class NavigationServiceTest : public ::testing::Test {
protected:
    HWeb::NavigationService nav_service;
};

TEST_F(NavigationServiceTest, DetermineNavigationStrategy) {
    // Test NEW_URL strategy
    HWeb::HWebConfig config1;
    config1.url = "http://example.com";
    Session session1("test1");
    
    auto strategy1 = nav_service.determine_navigation_strategy(config1, session1);
    EXPECT_EQ(strategy1, HWeb::NavigationStrategy::NEW_URL);
    
    // Test SESSION_RESTORE strategy  
    HWeb::HWebConfig config2;
    // No URL in config, but session has URL, no commands
    Session session2("test2");
    session2.setCurrentUrl("http://saved.com");
    
    auto strategy2 = nav_service.determine_navigation_strategy(config2, session2);
    EXPECT_EQ(strategy2, HWeb::NavigationStrategy::SESSION_RESTORE);
    
    // Test CONTINUE_SESSION strategy
    HWeb::HWebConfig config3;
    config3.commands.push_back({"click", "#button", ""});
    Session session3("test3");
    session3.setCurrentUrl("http://current.com");
    
    auto strategy3 = nav_service.determine_navigation_strategy(config3, session3);
    EXPECT_EQ(strategy3, HWeb::NavigationStrategy::CONTINUE_SESSION);
    
    // Test NO_NAVIGATION strategy
    HWeb::HWebConfig config4;
    Session session4("test4");
    
    auto strategy4 = nav_service.determine_navigation_strategy(config4, session4);
    EXPECT_EQ(strategy4, HWeb::NavigationStrategy::NO_NAVIGATION);
}

TEST_F(NavigationServiceTest, CreateNavigationPlan) {
    // Test NEW_URL plan
    HWeb::HWebConfig config;
    config.url = "http://test.com";
    Session session("test");
    
    auto plan = nav_service.create_navigation_plan(config, session);
    
    EXPECT_TRUE(plan.should_navigate);
    EXPECT_EQ(plan.navigation_url, "http://test.com");
    EXPECT_FALSE(plan.is_session_restore);
    EXPECT_EQ(plan.strategy, HWeb::NavigationStrategy::NEW_URL);
}

TEST_F(NavigationServiceTest, CreateNavigationPlanForSessionRestore) {
    HWeb::HWebConfig config;
    // No new URL specified
    Session session("restore_test");
    session.setCurrentUrl("http://existing.com");
    
    auto plan = nav_service.create_navigation_plan(config, session);
    
    EXPECT_TRUE(plan.should_navigate);
    EXPECT_EQ(plan.navigation_url, "http://existing.com");
    EXPECT_TRUE(plan.is_session_restore);
    EXPECT_EQ(plan.strategy, HWeb::NavigationStrategy::SESSION_RESTORE);
}

TEST_F(NavigationServiceTest, CreateNavigationPlanForNoNavigation) {
    HWeb::HWebConfig config;
    Session session("empty_test");
    // Both empty
    
    auto plan = nav_service.create_navigation_plan(config, session);
    
    EXPECT_FALSE(plan.should_navigate);
    EXPECT_TRUE(plan.navigation_url.empty());
    EXPECT_FALSE(plan.is_session_restore);
    EXPECT_EQ(plan.strategy, HWeb::NavigationStrategy::NO_NAVIGATION);
}
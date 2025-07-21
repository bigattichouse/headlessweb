#pragma once

#include "../Types.h"
#include "../../Browser/Browser.h"
#include "../../Session/Session.h"

namespace HWeb {

class NavigationService {
public:
    NavigationService();
    ~NavigationService();
    
    bool navigate_to_url(Browser& browser, const std::string& url);
    bool wait_for_navigation_complete(Browser& browser, int timeout_ms);
    bool wait_for_page_ready(Browser& browser, int timeout_ms);
    NavigationStrategy determine_navigation_strategy(const HWebConfig& config, const Session& session);
    
    struct NavigationPlan {
        bool should_navigate;
        std::string navigation_url;
        bool is_session_restore;
        NavigationStrategy strategy;
    };
    
    NavigationPlan create_navigation_plan(const HWebConfig& config, const Session& session);
    bool execute_navigation_plan(Browser& browser, Session& session, const NavigationPlan& plan);
    
private:
    void log_navigation_info(const std::string& url, bool is_restore);
};

} // namespace HWeb
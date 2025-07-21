#pragma once

#include "../Types.h"
#include "../../Browser/Browser.h"

namespace HWeb {

class AdvancedWaitHandler {
public:
    AdvancedWaitHandler();
    ~AdvancedWaitHandler();
    
    int handle_command(Browser& browser, const Command& cmd);
    
private:
    int handle_wait_text_advanced(Browser& browser, const Command& cmd);
    int handle_wait_network_idle(Browser& browser, const Command& cmd);
    int handle_wait_network_request(Browser& browser, const Command& cmd);
    int handle_wait_element_visible(Browser& browser, const Command& cmd);
    int handle_wait_element_count(Browser& browser, const Command& cmd);
    int handle_wait_attribute(Browser& browser, const Command& cmd);
    int handle_wait_url_change(Browser& browser, const Command& cmd);
    int handle_wait_title_change(Browser& browser, const Command& cmd);
    int handle_wait_spa_navigation(Browser& browser, const Command& cmd);
    int handle_wait_framework_ready(Browser& browser, const Command& cmd);
    int handle_wait_dom_change(Browser& browser, const Command& cmd);
    int handle_wait_content_change(Browser& browser, const Command& cmd);
};

} // namespace HWeb
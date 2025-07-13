#pragma once

#include <webkit/webkit.h>
#include <string>

// WebKit compatibility layer for handling API changes between versions
// This simplified version avoids deprecated APIs entirely

namespace WebKitCompat {
    
    // Get cookie manager from web view
    // Modern WebKitGTK (2.40+) uses network session, older versions use web context
    inline WebKitCookieManager* getCookieManager(WebKitWebView* webView) {
        #if WEBKIT_CHECK_VERSION(2, 40, 0)
            // Modern API: get from network session
            WebKitNetworkSession* session = webkit_web_view_get_network_session(webView);
            return session ? webkit_network_session_get_cookie_manager(session) : nullptr;
        #else
            // Older API: get from web context
            WebKitWebContext* context = webkit_web_view_get_context(webView);
            return webkit_web_context_get_cookie_manager(context);
        #endif
    }
    
    // Note: Cookie persistence is handled automatically by WebKitGTK
    // In modern versions, it uses the WebKitWebsiteDataManager's base directory
    // In older versions, you would call webkit_cookie_manager_set_persistent_storage
    // but we're avoiding that deprecated API
}

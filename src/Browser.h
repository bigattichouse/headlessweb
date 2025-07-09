#pragma once

#include "Session.h"
#include <string>

// Forward declarations for GTK and WebKit types
typedef struct _GtkWidget GtkWidget;
typedef struct _WebKitWebView WebKitWebView;

class Browser {
public:
    GtkWidget* window; // Made public for simplicity in POC
    WebKitWebView* webView; // Made public for simplicity in POC
    bool operation_completed; // Flag to signal completion

    Browser();
    ~Browser();

    void loadUri(const std::string& uri);
    void executeJavascript(const std::string& script, std::string* result = nullptr);
    std::string getCookies();
    void setCookies(const std::string& cookies);

    bool isOperationCompleted() const;
    bool waitForSelector(const std::string& selector, int timeout_ms);
    bool waitForText(const std::string& text, int timeout_ms);
    std::string getInnerText(const std::string& selector); // New method
};
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
    bool waitForElementWithContent(const std::string& selector, int timeout_ms);
    std::string getInnerText(const std::string& selector);
    std::string getFirstNonEmptyText(const std::string& selector);
    
    // Enhanced form operations
    bool fillInput(const std::string& selector, const std::string& value);
    bool clickElement(const std::string& selector);
    bool submitForm(const std::string& form_selector = "form");
    bool waitForNavigation(int timeout_ms);
    bool searchForm(const std::string& query);
    std::string getCurrentUrl();
    std::string getPageTitle();
    std::string getAttribute(const std::string& selector, const std::string& attribute);

private:
    void waitForJavaScriptCompletion(int timeout_ms = 5000);
};

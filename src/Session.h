#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <json/json.h>

struct Cookie {
    std::string name;
    std::string value;
    std::string domain;
    std::string path;
    bool secure;
    bool httpOnly;
    int64_t expires; // Unix timestamp, -1 for session cookie
};

struct FormField {
    std::string selector;
    std::string name;
    std::string id;
    std::string type;
    std::string value;
    bool checked;
};

struct PageReadyCondition {
    enum Type { SELECTOR, JS_EXPRESSION, CUSTOM };
    Type type;
    std::string value;
    int timeout;
};

class Session {
public:
    Session(const std::string& name);

    // Basic properties
    const std::string& getName() const;
    const std::string& getCurrentUrl() const;
    void setCurrentUrl(const std::string& url);

    // Navigation history
    const std::vector<std::string>& getHistory() const;
    int getHistoryIndex() const;
    void addToHistory(const std::string& url);
    void setHistoryIndex(int index);
    bool canGoBack() const;
    bool canGoForward() const;

    // Cookies
    const std::vector<Cookie>& getCookies() const;
    void setCookies(const std::vector<Cookie>& cookies);
    void addCookie(const Cookie& cookie);
    void clearCookies();

    // Local storage
    const std::map<std::string, std::string>& getLocalStorage() const;
    void setLocalStorage(const std::map<std::string, std::string>& storage);
    void setLocalStorageItem(const std::string& key, const std::string& value);
    
    // Session storage
    const std::map<std::string, std::string>& getSessionStorage() const;
    void setSessionStorage(const std::map<std::string, std::string>& storage);
    void setSessionStorageItem(const std::string& key, const std::string& value);

    // Form state
    const std::vector<FormField>& getFormFields() const;
    void setFormFields(const std::vector<FormField>& fields);
    void addFormField(const FormField& field);
    void clearFormFields();

    // Active UI elements
    const std::set<std::string>& getActiveElements() const;
    void setActiveElements(const std::set<std::string>& elements);
    void addActiveElement(const std::string& selector);

    // Scroll positions (per element)
    void setScrollPosition(const std::string& selector, int x, int y);
    std::pair<int, int> getScrollPosition(const std::string& selector = "window") const;
    const std::map<std::string, std::pair<int, int>>& getAllScrollPositions() const;

    // Page state
    const std::string& getPageHash() const;
    void setPageHash(const std::string& hash);
    const std::string& getDocumentReadyState() const;
    void setDocumentReadyState(const std::string& state);

    // Dynamic content markers
    void addReadyCondition(const PageReadyCondition& condition);
    const std::vector<PageReadyCondition>& getReadyConditions() const;
    void clearReadyConditions();

    // Viewport
    void setViewport(int width, int height);
    std::pair<int, int> getViewport() const;

    // User agent
    const std::string& getUserAgent() const;
    void setUserAgent(const std::string& ua);

    // Custom variables (for --store/--get)
    void setCustomVariable(const std::string& key, const std::string& value);
    std::string getCustomVariable(const std::string& key) const;
    bool hasCustomVariable(const std::string& key) const;

    // Custom state extractors
    void addStateExtractor(const std::string& name, const std::string& jsCode);
    const std::map<std::string, std::string>& getStateExtractors() const;
    
    // Extracted custom state
    void setExtractedState(const std::string& name, const Json::Value& value);
    Json::Value getExtractedState(const std::string& name) const;
    const std::map<std::string, Json::Value>& getAllExtractedState() const;

    // Serialization
    std::string serialize() const;
    static Session deserialize(const std::string& data);

    // Session metadata
    void updateLastAccessed();
    int64_t getLastAccessed() const;
    size_t getApproximateSize() const;
    
    // Action recording
    struct RecordedAction {
        std::string type;
        std::string selector;
        std::string value;
        int delay;
    };
    
    void recordAction(const RecordedAction& action);
    const std::vector<RecordedAction>& getRecordedActions() const;
    void clearRecordedActions();
    bool isRecording() const;
    void setRecording(bool recording);

private:
    std::string name;
    std::string currentUrl;
    std::vector<std::string> history;
    int historyIndex;
    std::vector<Cookie> cookies;
    std::map<std::string, std::string> localStorage;
    std::map<std::string, std::string> sessionStorage;
    std::vector<FormField> formFields;
    std::set<std::string> activeElements;
    std::map<std::string, std::pair<int, int>> scrollPositions;
    std::string pageHash;
    std::string documentReadyState;
    std::vector<PageReadyCondition> readyConditions;
    int viewportWidth;
    int viewportHeight;
    std::string userAgent;
    std::map<std::string, std::string> customVariables;
    std::map<std::string, std::string> stateExtractors;
    std::map<std::string, Json::Value> extractedState;
    int64_t lastAccessed;
    std::vector<RecordedAction> recordedActions;
    bool recording;

    // Helper methods
    Json::Value cookieToJson(const Cookie& cookie) const;
    Cookie jsonToCookie(const Json::Value& json) const;
    Json::Value formFieldToJson(const FormField& field) const;
    FormField jsonToFormField(const Json::Value& json) const;
    Json::Value readyConditionToJson(const PageReadyCondition& condition) const;
    PageReadyCondition jsonToReadyCondition(const Json::Value& json) const;
    Json::Value recordedActionToJson(const RecordedAction& action) const;
    RecordedAction jsonToRecordedAction(const Json::Value& json) const;
};

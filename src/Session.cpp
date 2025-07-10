#include "Session.h"
#include <chrono>
#include <sstream>
#include <algorithm>

Session::Session(const std::string& name) 
    : name(name), historyIndex(-1), viewportWidth(1920), viewportHeight(1080),
      userAgent("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.0 Safari/605.1.15"),
      recording(false) {
    updateLastAccessed();
    scrollPositions["window"] = {0, 0};
}

const std::string& Session::getName() const {
    return name;
}

const std::string& Session::getCurrentUrl() const {
    return currentUrl;
}

void Session::setCurrentUrl(const std::string& url) {
    currentUrl = url;
}

const std::vector<std::string>& Session::getHistory() const {
    return history;
}

int Session::getHistoryIndex() const {
    return historyIndex;
}

void Session::addToHistory(const std::string& url) {
    // If we're not at the end of history, truncate forward history
    if (historyIndex >= 0 && historyIndex < static_cast<int>(history.size()) - 1) {
        history.erase(history.begin() + historyIndex + 1, history.end());
    }
    
    history.push_back(url);
    historyIndex = history.size() - 1;
    
    // Limit history size to prevent unbounded growth
    const size_t MAX_HISTORY = 100;
    if (history.size() > MAX_HISTORY) {
        history.erase(history.begin());
        historyIndex--;
    }
}

void Session::setHistoryIndex(int index) {
    if (index >= 0 && index < static_cast<int>(history.size())) {
        historyIndex = index;
    }
}

bool Session::canGoBack() const {
    return historyIndex > 0;
}

bool Session::canGoForward() const {
    return historyIndex < static_cast<int>(history.size()) - 1;
}

const std::vector<Cookie>& Session::getCookies() const {
    return cookies;
}

void Session::setCookies(const std::vector<Cookie>& cookies) {
    this->cookies = cookies;
}

void Session::addCookie(const Cookie& cookie) {
    // Update existing cookie or add new one
    auto it = std::find_if(cookies.begin(), cookies.end(),
        [&cookie](const Cookie& c) {
            return c.name == cookie.name && c.domain == cookie.domain && c.path == cookie.path;
        });
    
    if (it != cookies.end()) {
        *it = cookie;
    } else {
        cookies.push_back(cookie);
    }
}

void Session::clearCookies() {
    cookies.clear();
}

const std::map<std::string, std::string>& Session::getLocalStorage() const {
    return localStorage;
}

void Session::setLocalStorage(const std::map<std::string, std::string>& storage) {
    localStorage = storage;
}

void Session::setLocalStorageItem(const std::string& key, const std::string& value) {
    localStorage[key] = value;
}

const std::map<std::string, std::string>& Session::getSessionStorage() const {
    return sessionStorage;
}

void Session::setSessionStorage(const std::map<std::string, std::string>& storage) {
    sessionStorage = storage;
}

void Session::setSessionStorageItem(const std::string& key, const std::string& value) {
    sessionStorage[key] = value;
}

const std::vector<FormField>& Session::getFormFields() const {
    return formFields;
}

void Session::setFormFields(const std::vector<FormField>& fields) {
    formFields = fields;
}

void Session::addFormField(const FormField& field) {
    formFields.push_back(field);
}

void Session::clearFormFields() {
    formFields.clear();
}

const std::set<std::string>& Session::getActiveElements() const {
    return activeElements;
}

void Session::setActiveElements(const std::set<std::string>& elements) {
    activeElements = elements;
}

void Session::addActiveElement(const std::string& selector) {
    activeElements.insert(selector);
}

void Session::setScrollPosition(const std::string& selector, int x, int y) {
    scrollPositions[selector] = {x, y};
}

std::pair<int, int> Session::getScrollPosition(const std::string& selector) const {
    auto it = scrollPositions.find(selector);
    return (it != scrollPositions.end()) ? it->second : std::make_pair(0, 0);
}

const std::map<std::string, std::pair<int, int>>& Session::getAllScrollPositions() const {
    return scrollPositions;
}

const std::string& Session::getPageHash() const {
    return pageHash;
}

void Session::setPageHash(const std::string& hash) {
    pageHash = hash;
}

const std::string& Session::getDocumentReadyState() const {
    return documentReadyState;
}

void Session::setDocumentReadyState(const std::string& state) {
    documentReadyState = state;
}

void Session::addReadyCondition(const PageReadyCondition& condition) {
    readyConditions.push_back(condition);
}

const std::vector<PageReadyCondition>& Session::getReadyConditions() const {
    return readyConditions;
}

void Session::clearReadyConditions() {
    readyConditions.clear();
}

void Session::setViewport(int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
}

std::pair<int, int> Session::getViewport() const {
    return {viewportWidth, viewportHeight};
}

const std::string& Session::getUserAgent() const {
    return userAgent;
}

void Session::setUserAgent(const std::string& ua) {
    userAgent = ua;
}

void Session::setCustomVariable(const std::string& key, const std::string& value) {
    customVariables[key] = value;
}

std::string Session::getCustomVariable(const std::string& key) const {
    auto it = customVariables.find(key);
    return (it != customVariables.end()) ? it->second : "";
}

bool Session::hasCustomVariable(const std::string& key) const {
    return customVariables.find(key) != customVariables.end();
}

void Session::addStateExtractor(const std::string& name, const std::string& jsCode) {
    stateExtractors[name] = jsCode;
}

const std::map<std::string, std::string>& Session::getStateExtractors() const {
    return stateExtractors;
}

void Session::setExtractedState(const std::string& name, const Json::Value& value) {
    extractedState[name] = value;
}

Json::Value Session::getExtractedState(const std::string& name) const {
    auto it = extractedState.find(name);
    return (it != extractedState.end()) ? it->second : Json::Value::null;
}

const std::map<std::string, Json::Value>& Session::getAllExtractedState() const {
    return extractedState;
}

void Session::recordAction(const RecordedAction& action) {
    if (recording) {
        recordedActions.push_back(action);
    }
}

const std::vector<Session::RecordedAction>& Session::getRecordedActions() const {
    return recordedActions;
}

void Session::clearRecordedActions() {
    recordedActions.clear();
}

bool Session::isRecording() const {
    return recording;
}

void Session::setRecording(bool recording) {
    this->recording = recording;
}

void Session::updateLastAccessed() {
    auto now = std::chrono::system_clock::now();
    lastAccessed = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

int64_t Session::getLastAccessed() const {
    return lastAccessed;
}

size_t Session::getApproximateSize() const {
    size_t size = sizeof(*this);
    size += name.size() + currentUrl.size() + userAgent.size() + pageHash.size() + documentReadyState.size();
    
    for (const auto& url : history) {
        size += url.size();
    }
    
    for (const auto& cookie : cookies) {
        size += cookie.name.size() + cookie.value.size() + cookie.domain.size() + cookie.path.size();
    }
    
    for (const auto& field : formFields) {
        size += field.selector.size() + field.name.size() + field.id.size() + field.type.size() + field.value.size();
    }
    
    for (const auto& element : activeElements) {
        size += element.size();
    }
    
    for (const auto& [key, value] : localStorage) {
        size += key.size() + value.size();
    }
    
    for (const auto& [key, value] : sessionStorage) {
        size += key.size() + value.size();
    }
    
    for (const auto& [key, value] : customVariables) {
        size += key.size() + value.size();
    }
    
    for (const auto& [key, value] : stateExtractors) {
        size += key.size() + value.size();
    }
    
    for (const auto& action : recordedActions) {
        size += action.type.size() + action.selector.size() + action.value.size();
    }
    
    return size;
}

Json::Value Session::cookieToJson(const Cookie& cookie) const {
    Json::Value json;
    json["name"] = cookie.name;
    json["value"] = cookie.value;
    json["domain"] = cookie.domain;
    json["path"] = cookie.path;
    json["secure"] = cookie.secure;
    json["httpOnly"] = cookie.httpOnly;
    json["expires"] = static_cast<Json::Int64>(cookie.expires);
    return json;
}

Cookie Session::jsonToCookie(const Json::Value& json) const {
    Cookie cookie;
    cookie.name = json.get("name", "").asString();
    cookie.value = json.get("value", "").asString();
    cookie.domain = json.get("domain", "").asString();
    cookie.path = json.get("path", "/").asString();
    cookie.secure = json.get("secure", false).asBool();
    cookie.httpOnly = json.get("httpOnly", false).asBool();
    cookie.expires = json.get("expires", -1).asInt64();
    return cookie;
}

Json::Value Session::formFieldToJson(const FormField& field) const {
    Json::Value json;
    json["selector"] = field.selector;
    json["name"] = field.name;
    json["id"] = field.id;
    json["type"] = field.type;
    json["value"] = field.value;
    json["checked"] = field.checked;
    return json;
}

FormField Session::jsonToFormField(const Json::Value& json) const {
    FormField field;
    field.selector = json.get("selector", "").asString();
    field.name = json.get("name", "").asString();
    field.id = json.get("id", "").asString();
    field.type = json.get("type", "").asString();
    field.value = json.get("value", "").asString();
    field.checked = json.get("checked", false).asBool();
    return field;
}

Json::Value Session::readyConditionToJson(const PageReadyCondition& condition) const {
    Json::Value json;
    json["type"] = static_cast<int>(condition.type);
    json["value"] = condition.value;
    json["timeout"] = condition.timeout;
    return json;
}

PageReadyCondition Session::jsonToReadyCondition(const Json::Value& json) const {
    PageReadyCondition condition;
    condition.type = static_cast<PageReadyCondition::Type>(json.get("type", 0).asInt());
    condition.value = json.get("value", "").asString();
    condition.timeout = json.get("timeout", 10000).asInt();
    return condition;
}

Json::Value Session::recordedActionToJson(const RecordedAction& action) const {
    Json::Value json;
    json["type"] = action.type;
    json["selector"] = action.selector;
    json["value"] = action.value;
    json["delay"] = action.delay;
    return json;
}

Session::RecordedAction Session::jsonToRecordedAction(const Json::Value& json) const {
    RecordedAction action;
    action.type = json.get("type", "").asString();
    action.selector = json.get("selector", "").asString();
    action.value = json.get("value", "").asString();
    action.delay = json.get("delay", 0).asInt();
    return action;
}

std::string Session::serialize() const {
    Json::Value root;
    
    // Version for future compatibility
    root["version"] = 3;
    
    // Basic info
    root["name"] = name;
    root["currentUrl"] = currentUrl;
    root["lastAccessed"] = static_cast<Json::Int64>(lastAccessed);
    
    // Navigation history
    Json::Value historyArray(Json::arrayValue);
    for (const auto& url : history) {
        historyArray.append(url);
    }
    root["history"] = historyArray;
    root["historyIndex"] = historyIndex;
    
    // Cookies
    Json::Value cookiesArray(Json::arrayValue);
    for (const auto& cookie : cookies) {
        cookiesArray.append(cookieToJson(cookie));
    }
    root["cookies"] = cookiesArray;
    
    // Storage
    Json::Value localStorageObj(Json::objectValue);
    for (const auto& [key, value] : localStorage) {
        localStorageObj[key] = value;
    }
    root["localStorage"] = localStorageObj;
    
    Json::Value sessionStorageObj(Json::objectValue);
    for (const auto& [key, value] : sessionStorage) {
        sessionStorageObj[key] = value;
    }
    root["sessionStorage"] = sessionStorageObj;
    
    // Form fields
    Json::Value formFieldsArray(Json::arrayValue);
    for (const auto& field : formFields) {
        formFieldsArray.append(formFieldToJson(field));
    }
    root["formFields"] = formFieldsArray;
    
    // Active elements
    Json::Value activeElementsArray(Json::arrayValue);
    for (const auto& element : activeElements) {
        activeElementsArray.append(element);
    }
    root["activeElements"] = activeElementsArray;
    
    // Scroll positions
    Json::Value scrollPosObj(Json::objectValue);
    for (const auto& [selector, pos] : scrollPositions) {
        Json::Value posArray(Json::arrayValue);
        posArray.append(pos.first);
        posArray.append(pos.second);
        scrollPosObj[selector] = posArray;
    }
    root["scrollPositions"] = scrollPosObj;
    
    // Page state
    root["pageHash"] = pageHash;
    root["documentReadyState"] = documentReadyState;
    
    // Ready conditions
    Json::Value readyConditionsArray(Json::arrayValue);
    for (const auto& condition : readyConditions) {
        readyConditionsArray.append(readyConditionToJson(condition));
    }
    root["readyConditions"] = readyConditionsArray;
    
    // Viewport
    root["viewport"]["width"] = viewportWidth;
    root["viewport"]["height"] = viewportHeight;
    
    // User agent
    root["userAgent"] = userAgent;
    
    // Custom variables
    Json::Value customVarsObj(Json::objectValue);
    for (const auto& [key, value] : customVariables) {
        customVarsObj[key] = value;
    }
    root["customVariables"] = customVarsObj;
    
    // State extractors
    Json::Value extractorsObj(Json::objectValue);
    for (const auto& [key, value] : stateExtractors) {
        extractorsObj[key] = value;
    }
    root["stateExtractors"] = extractorsObj;
    
    // Extracted state
    Json::Value extractedStateObj(Json::objectValue);
    for (const auto& [key, value] : extractedState) {
        extractedStateObj[key] = value;
    }
    root["extractedState"] = extractedStateObj;
    
    // Recorded actions
    Json::Value recordedActionsArray(Json::arrayValue);
    for (const auto& action : recordedActions) {
        recordedActionsArray.append(recordedActionToJson(action));
    }
    root["recordedActions"] = recordedActionsArray;
    root["recording"] = recording;
    
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  "; // Pretty print for debugging
    return Json::writeString(builder, root);
}

Session Session::deserialize(const std::string& data) {
    Json::Value root;
    Json::Reader reader;
    
    if (!reader.parse(data, root)) {
        throw std::runtime_error("Failed to parse session JSON");
    }
    
    Session session(root.get("name", "default").asString());
    
    // Check version for compatibility
    int version = root.get("version", 1).asInt();
    
    // Basic info
    session.currentUrl = root.get("currentUrl", "").asString();
    
    // For backward compatibility, also check old "url" field
    if (session.currentUrl.empty() && root.isMember("url")) {
        session.currentUrl = root["url"].asString();
    }
    
    session.lastAccessed = root.get("lastAccessed", 0).asInt64();
    
    // Navigation history
    if (root.isMember("history") && root["history"].isArray()) {
        session.history.clear();
        for (const auto& url : root["history"]) {
            session.history.push_back(url.asString());
        }
        session.historyIndex = root.get("historyIndex", -1).asInt();
    }
    
    // Cookies
    if (root.isMember("cookies") && root["cookies"].isArray()) {
        for (const auto& cookieJson : root["cookies"]) {
            session.cookies.push_back(session.jsonToCookie(cookieJson));
        }
    }
    
    // Storage
    if (root.isMember("localStorage") && root["localStorage"].isObject()) {
        for (const auto& key : root["localStorage"].getMemberNames()) {
            session.localStorage[key] = root["localStorage"][key].asString();
        }
    }
    
    if (root.isMember("sessionStorage") && root["sessionStorage"].isObject()) {
        for (const auto& key : root["sessionStorage"].getMemberNames()) {
            session.sessionStorage[key] = root["sessionStorage"][key].asString();
        }
    }
    
    // Form fields (version 3+)
    if (version >= 3 && root.isMember("formFields") && root["formFields"].isArray()) {
        for (const auto& fieldJson : root["formFields"]) {
            session.formFields.push_back(session.jsonToFormField(fieldJson));
        }
    }
    
    // Active elements
    if (version >= 3 && root.isMember("activeElements") && root["activeElements"].isArray()) {
        for (const auto& element : root["activeElements"]) {
            session.activeElements.insert(element.asString());
        }
    }
    
    // Scroll positions
    if (root.isMember("scrollPositions") && root["scrollPositions"].isObject()) {
        for (const auto& selector : root["scrollPositions"].getMemberNames()) {
            const auto& posArray = root["scrollPositions"][selector];
            if (posArray.isArray() && posArray.size() >= 2) {
                session.scrollPositions[selector] = {posArray[0].asInt(), posArray[1].asInt()};
            }
        }
    } else if (root.isMember("scroll")) {
        // Backward compatibility with old single scroll position
        session.scrollPositions["window"] = {
            root["scroll"].get("x", 0).asInt(),
            root["scroll"].get("y", 0).asInt()
        };
    }
    
    // Page state
    if (version >= 3) {
        session.pageHash = root.get("pageHash", "").asString();
        session.documentReadyState = root.get("documentReadyState", "").asString();
    }
    
    // Ready conditions
    if (version >= 3 && root.isMember("readyConditions") && root["readyConditions"].isArray()) {
        for (const auto& conditionJson : root["readyConditions"]) {
            session.readyConditions.push_back(session.jsonToReadyCondition(conditionJson));
        }
    }
    
    // Viewport
    if (root.isMember("viewport")) {
        session.viewportWidth = root["viewport"].get("width", 1920).asInt();
        session.viewportHeight = root["viewport"].get("height", 1080).asInt();
    }
    
    // User agent
    if (root.isMember("userAgent")) {
        session.userAgent = root["userAgent"].asString();
    }
    
    // Custom variables
    if (root.isMember("customVariables") && root["customVariables"].isObject()) {
        for (const auto& key : root["customVariables"].getMemberNames()) {
            session.customVariables[key] = root["customVariables"][key].asString();
        }
    }
    
    // State extractors
    if (version >= 3 && root.isMember("stateExtractors") && root["stateExtractors"].isObject()) {
        for (const auto& key : root["stateExtractors"].getMemberNames()) {
            session.stateExtractors[key] = root["stateExtractors"][key].asString();
        }
    }
    
    // Extracted state
    if (version >= 3 && root.isMember("extractedState") && root["extractedState"].isObject()) {
        for (const auto& key : root["extractedState"].getMemberNames()) {
            session.extractedState[key] = root["extractedState"][key];
        }
    }
    
    // Recorded actions
    if (version >= 3 && root.isMember("recordedActions") && root["recordedActions"].isArray()) {
        for (const auto& actionJson : root["recordedActions"]) {
            session.recordedActions.push_back(session.jsonToRecordedAction(actionJson));
        }
        session.recording = root.get("recording", false).asBool();
    }
    
    return session;
}

#include "Session.h"
#include <json/json.h>

Session::Session(const std::string& name) : name(name) {}

const std::string& Session::getName() const {
    return name;
}

const std::string& Session::getUrl() const {
    return url;
}

const std::string& Session::getCookies() const {
    return cookies;
}

void Session::setUrl(const std::string& url) {
    this->url = url;
}

void Session::setCookies(const std::string& cookies) {
    this->cookies = cookies;
}

std::string Session::serialize() const {
    Json::Value sessionJson;
    sessionJson["name"] = name;
    sessionJson["url"] = url;
    sessionJson["cookies"] = cookies;

    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, sessionJson);
}

Session Session::deserialize(const std::string& data) {
    Json::Value sessionJson;
    Json::Reader reader;
    reader.parse(data, sessionJson);

    Session session(sessionJson["name"].asString());
    session.setUrl(sessionJson["url"].asString());
    session.setCookies(sessionJson["cookies"].asString());

    return session;
}

#pragma once

#include <string>
#include <vector>

class Session {
public:
    Session(const std::string& name);

    const std::string& getName() const;
    const std::string& getUrl() const;
    const std::string& getCookies() const;

    void setUrl(const std::string& url);
    void setCookies(const std::string& cookies);

    std::string serialize() const;
    static Session deserialize(const std::string& data);

private:
    std::string name;
    std::string url;
    std::string cookies; // Storing cookies as a string for simplicity in the POC
};

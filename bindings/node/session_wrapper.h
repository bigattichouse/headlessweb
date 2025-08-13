#ifndef SESSION_WRAPPER_H
#define SESSION_WRAPPER_H

#include <napi.h>
#include <memory>
#include "Session/Manager.h"

class SessionWrapper : public Napi::ObjectWrap<SessionWrapper> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    SessionWrapper(const Napi::CallbackInfo& info);
    ~SessionWrapper();

private:
    static Napi::FunctionReference constructor;
    
    // Session management methods
    Napi::Value GetSessionName(const Napi::CallbackInfo& info);
    Napi::Value SaveSession(const Napi::CallbackInfo& info);
    Napi::Value LoadSession(const Napi::CallbackInfo& info);
    Napi::Value DeleteSession(const Napi::CallbackInfo& info);
    Napi::Value ListSessions(const Napi::CallbackInfo& info);
    
    std::unique_ptr<SessionManager> session_manager_;
    std::string session_name_;
};

#endif // SESSION_WRAPPER_H
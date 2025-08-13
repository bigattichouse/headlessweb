#include "session_wrapper.h"
#include <iostream>

Napi::FunctionReference SessionWrapper::constructor;

Napi::Object SessionWrapper::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "Session", {
        InstanceMethod("getSessionName", &SessionWrapper::GetSessionName),
        InstanceMethod("saveSession", &SessionWrapper::SaveSession),
        InstanceMethod("loadSession", &SessionWrapper::LoadSession),
        InstanceMethod("deleteSession", &SessionWrapper::DeleteSession),
        InstanceMethod("listSessions", &SessionWrapper::ListSessions)
    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    
    exports.Set("Session", func);
    return exports;
}

SessionWrapper::SessionWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<SessionWrapper>(info) {
    Napi::Env env = info.Env();
    
    std::string session_name = "default";
    std::string sessions_dir = "./sessions";
    
    if (info.Length() > 0 && info[0].IsString()) {
        session_name = info[0].As<Napi::String>().Utf8Value();
    }
    
    if (info.Length() > 1 && info[1].IsString()) {
        sessions_dir = info[1].As<Napi::String>().Utf8Value();
    }
    
    session_name_ = session_name;
    
    try {
        session_manager_ = std::make_unique<SessionManager>(sessions_dir);
        std::cout << "SessionWrapper created for: " << session_name << std::endl;
    } catch (const std::exception& e) {
        Napi::TypeError::New(env, "Failed to create session manager: " + std::string(e.what())).ThrowAsJavaScriptException();
    }
}

SessionWrapper::~SessionWrapper() {
    std::cout << "SessionWrapper destroyed" << std::endl;
}

Napi::Value SessionWrapper::GetSessionName(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, session_name_);
}

Napi::Value SessionWrapper::SaveSession(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // TODO: Implement session saving
    return Napi::Boolean::New(env, true);
}

Napi::Value SessionWrapper::LoadSession(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // TODO: Implement session loading
    return Napi::Boolean::New(env, true);
}

Napi::Value SessionWrapper::DeleteSession(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // TODO: Implement session deletion
    return Napi::Boolean::New(env, true);
}

Napi::Value SessionWrapper::ListSessions(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // TODO: Implement session listing
    Napi::Array sessions = Napi::Array::New(env, 0);
    return sessions;
}
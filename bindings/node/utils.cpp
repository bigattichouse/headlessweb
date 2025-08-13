#include "utils.h"
#include <iostream>

Napi::Value GetCoreVersion(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // TODO: Get actual version from C++ core
    return Napi::String::New(env, "1.0.0");
}

Napi::Value GetSystemInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    Napi::Object sysInfo = Napi::Object::New(env);
    sysInfo.Set("platform", Napi::String::New(env, "linux"));
    sysInfo.Set("arch", Napi::String::New(env, "x64"));
    sysInfo.Set("webkit_version", Napi::String::New(env, "6.0"));
    
    return sysInfo;
}
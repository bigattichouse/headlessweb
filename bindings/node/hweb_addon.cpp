#include <napi.h>
#include "browser_wrapper.h"
#include "session_wrapper.h"
#include "utils.h"

// Main addon initialization
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // Initialize Browser wrapper
    BrowserWrapper::Init(env, exports);
    
    // Initialize Session wrapper  
    SessionWrapper::Init(env, exports);
    
    // Add utility functions
    exports.Set(Napi::String::New(env, "getCoreVersion"), 
                Napi::Function::New(env, GetCoreVersion));
    
    exports.Set(Napi::String::New(env, "getSystemInfo"), 
                Napi::Function::New(env, GetSystemInfo));
    
    return exports;
}

NODE_API_MODULE(hweb_addon, Init)
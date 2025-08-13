#ifndef UTILS_H
#define UTILS_H

#include <napi.h>

// Utility functions for the addon
Napi::Value GetCoreVersion(const Napi::CallbackInfo& info);
Napi::Value GetSystemInfo(const Napi::CallbackInfo& info);

#endif // UTILS_H
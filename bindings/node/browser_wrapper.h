#ifndef BROWSER_WRAPPER_H
#define BROWSER_WRAPPER_H

#include <napi.h>
#include <memory>
#include "Browser.h"

class BrowserWrapper : public Napi::ObjectWrap<BrowserWrapper> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    BrowserWrapper(const Napi::CallbackInfo& info);
    ~BrowserWrapper();

private:
    static Napi::FunctionReference constructor;
    
    // Navigation methods
    Napi::Value LoadUri(const Napi::CallbackInfo& info);
    Napi::Value LoadUriAsync(const Napi::CallbackInfo& info);
    Napi::Value GetCurrentUrl(const Napi::CallbackInfo& info);
    
    // DOM interaction methods
    Napi::Value ClickElement(const Napi::CallbackInfo& info);
    Napi::Value ClickElementAsync(const Napi::CallbackInfo& info);
    Napi::Value FillInput(const Napi::CallbackInfo& info);
    Napi::Value FillInputAsync(const Napi::CallbackInfo& info);
    Napi::Value SelectOption(const Napi::CallbackInfo& info);
    Napi::Value CheckElement(const Napi::CallbackInfo& info);
    Napi::Value UncheckElement(const Napi::CallbackInfo& info);
    Napi::Value FocusElement(const Napi::CallbackInfo& info);
    
    // Element query methods
    Napi::Value ElementExists(const Napi::CallbackInfo& info);
    Napi::Value CountElements(const Napi::CallbackInfo& info);
    Napi::Value GetInnerText(const Napi::CallbackInfo& info);
    Napi::Value GetElementHtml(const Napi::CallbackInfo& info);
    
    // Attribute methods
    Napi::Value GetAttribute(const Napi::CallbackInfo& info);
    Napi::Value SetAttribute(const Napi::CallbackInfo& info);
    
    // JavaScript execution
    Napi::Value ExecuteJavaScript(const Napi::CallbackInfo& info);
    Napi::Value ExecuteJavaScriptAsync(const Napi::CallbackInfo& info);
    
    // Screenshot methods
    Napi::Value TakeScreenshot(const Napi::CallbackInfo& info);
    Napi::Value TakeScreenshotAsync(const Napi::CallbackInfo& info);
    
    // Waiting methods
    Napi::Value WaitForSelector(const Napi::CallbackInfo& info);
    Napi::Value WaitForNavigation(const Napi::CallbackInfo& info);
    
    // Helper for async operations
    class AsyncWorker;
    
    std::unique_ptr<Browser> browser_;
    std::string session_name_;
};

#endif // BROWSER_WRAPPER_H
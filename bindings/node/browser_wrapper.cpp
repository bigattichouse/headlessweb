#include "browser_wrapper.h"
#include <iostream>
#include <thread>
#include <chrono>

Napi::FunctionReference BrowserWrapper::constructor;

// Async worker for long-running operations
class BrowserWrapper::AsyncWorker : public Napi::AsyncWorker {
public:
    AsyncWorker(Napi::Function& callback, BrowserWrapper* wrapper, const std::string& operation)
        : Napi::AsyncWorker(callback), wrapper_(wrapper), operation_(operation) {}
    
    virtual ~AsyncWorker() {}
    
    void Execute() override {
        try {
            if (operation_ == "navigate") {
                result_bool_ = wrapper_->browser_->loadUri(url_param_);
            } else if (operation_ == "click") {
                result_bool_ = wrapper_->browser_->clickElement(selector_param_);
            } else if (operation_ == "fill") {
                result_bool_ = wrapper_->browser_->fillInput(selector_param_, value_param_);
            } else if (operation_ == "screenshot") {
                result_bool_ = wrapper_->browser_->takeScreenshot(filename_param_);
            } else if (operation_ == "javascript") {
                result_string_ = wrapper_->browser_->executeJavascriptSync(js_param_);
                result_bool_ = true;
            }
        } catch (const std::exception& e) {
            SetError(e.what());
        }
    }
    
    void OnOK() override {
        Napi::HandleScope scope(Env());
        if (operation_ == "javascript") {
            Callback().Call({Env().Null(), Napi::String::New(Env(), result_string_)});
        } else {
            Callback().Call({Env().Null(), Napi::Boolean::New(Env(), result_bool_)});
        }
    }
    
    void OnError(const Napi::Error& e) override {
        Napi::HandleScope scope(Env());
        Callback().Call({e.Value(), Env().Undefined()});
    }
    
    // Parameters for different operations
    std::string url_param_;
    std::string selector_param_;
    std::string value_param_;
    std::string filename_param_;
    std::string js_param_;
    
private:
    BrowserWrapper* wrapper_;
    std::string operation_;
    bool result_bool_ = false;
    std::string result_string_;
};

Napi::Object BrowserWrapper::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "Browser", {
        // Navigation
        InstanceMethod("loadUri", &BrowserWrapper::LoadUri),
        InstanceMethod("loadUriAsync", &BrowserWrapper::LoadUriAsync),
        InstanceMethod("getCurrentUrl", &BrowserWrapper::GetCurrentUrl),
        
        // DOM interaction
        InstanceMethod("clickElement", &BrowserWrapper::ClickElement),
        InstanceMethod("clickElementAsync", &BrowserWrapper::ClickElementAsync),
        InstanceMethod("fillInput", &BrowserWrapper::FillInput),
        InstanceMethod("fillInputAsync", &BrowserWrapper::FillInputAsync),
        InstanceMethod("selectOption", &BrowserWrapper::SelectOption),
        InstanceMethod("checkElement", &BrowserWrapper::CheckElement),
        InstanceMethod("uncheckElement", &BrowserWrapper::UncheckElement),
        InstanceMethod("focusElement", &BrowserWrapper::FocusElement),
        
        // Element queries
        InstanceMethod("elementExists", &BrowserWrapper::ElementExists),
        InstanceMethod("countElements", &BrowserWrapper::CountElements),
        InstanceMethod("getInnerText", &BrowserWrapper::GetInnerText),
        InstanceMethod("getElementHtml", &BrowserWrapper::GetElementHtml),
        
        // Attributes
        InstanceMethod("getAttribute", &BrowserWrapper::GetAttribute),
        InstanceMethod("setAttribute", &BrowserWrapper::SetAttribute),
        
        // JavaScript execution
        InstanceMethod("executeJavaScript", &BrowserWrapper::ExecuteJavaScript),
        InstanceMethod("executeJavaScriptAsync", &BrowserWrapper::ExecuteJavaScriptAsync),
        
        // Screenshots
        InstanceMethod("takeScreenshot", &BrowserWrapper::TakeScreenshot),
        InstanceMethod("takeScreenshotAsync", &BrowserWrapper::TakeScreenshotAsync),
        
        // Waiting
        InstanceMethod("waitForSelector", &BrowserWrapper::WaitForSelector),
        InstanceMethod("waitForNavigation", &BrowserWrapper::WaitForNavigation)
    });

    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    
    exports.Set("Browser", func);
    return exports;
}

BrowserWrapper::BrowserWrapper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<BrowserWrapper>(info) {
    Napi::Env env = info.Env();
    
    // Extract options from constructor argument
    std::string session = "default";
    bool headless = true;
    
    if (info.Length() > 0 && info[0].IsObject()) {
        Napi::Object options = info[0].As<Napi::Object>();
        
        if (options.Has("session")) {
            session = options.Get("session").As<Napi::String>().Utf8Value();
        }
        
        if (options.Has("headless")) {
            headless = options.Get("headless").As<Napi::Boolean>().Value();
        }
    }
    
    session_name_ = session;
    
    try {
        // Create browser instance
        browser_ = std::make_unique<Browser>();
        
        // Set up session directory if needed
        // Note: This would integrate with SessionManager in a full implementation
        
        std::cout << "BrowserWrapper created with session: " << session << std::endl;
    } catch (const std::exception& e) {
        Napi::TypeError::New(env, "Failed to create browser: " + std::string(e.what())).ThrowAsJavaScriptException();
    }
}

BrowserWrapper::~BrowserWrapper() {
    std::cout << "BrowserWrapper destroyed" << std::endl;
}

// Navigation methods
Napi::Value BrowserWrapper::LoadUri(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "URL string expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string url = info[0].As<Napi::String>().Utf8Value();
    
    try {
        bool result = browser_->loadUri(url);
        return Napi::Boolean::New(env, result);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value BrowserWrapper::LoadUriAsync(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction()) {
        Napi::TypeError::New(env, "URL string and callback function expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string url = info[0].As<Napi::String>().Utf8Value();
    Napi::Function callback = info[1].As<Napi::Function>();
    
    AsyncWorker* worker = new AsyncWorker(callback, this, "navigate");
    worker->url_param_ = url;
    worker->Queue();
    
    return env.Undefined();
}

Napi::Value BrowserWrapper::GetCurrentUrl(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    try {
        std::string url = browser_->getCurrentUrl();
        return Napi::String::New(env, url);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

// DOM interaction methods
Napi::Value BrowserWrapper::ClickElement(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Selector string expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string selector = info[0].As<Napi::String>().Utf8Value();
    
    try {
        bool result = browser_->clickElement(selector);
        return Napi::Boolean::New(env, result);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value BrowserWrapper::ClickElementAsync(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction()) {
        Napi::TypeError::New(env, "Selector string and callback function expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string selector = info[0].As<Napi::String>().Utf8Value();
    Napi::Function callback = info[1].As<Napi::Function>();
    
    AsyncWorker* worker = new AsyncWorker(callback, this, "click");
    worker->selector_param_ = selector;
    worker->Queue();
    
    return env.Undefined();
}

Napi::Value BrowserWrapper::FillInput(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "Selector and value strings expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string selector = info[0].As<Napi::String>().Utf8Value();
    std::string value = info[1].As<Napi::String>().Utf8Value();
    
    try {
        bool result = browser_->fillInput(selector, value);
        return Napi::Boolean::New(env, result);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value BrowserWrapper::FillInputAsync(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsFunction()) {
        Napi::TypeError::New(env, "Selector, value strings and callback function expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string selector = info[0].As<Napi::String>().Utf8Value();
    std::string value = info[1].As<Napi::String>().Utf8Value();
    Napi::Function callback = info[2].As<Napi::Function>();
    
    AsyncWorker* worker = new AsyncWorker(callback, this, "fill");
    worker->selector_param_ = selector;
    worker->value_param_ = value;
    worker->Queue();
    
    return env.Undefined();
}

// Element query methods
Napi::Value BrowserWrapper::ElementExists(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Selector string expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string selector = info[0].As<Napi::String>().Utf8Value();
    
    try {
        bool result = browser_->elementExists(selector);
        return Napi::Boolean::New(env, result);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value BrowserWrapper::GetInnerText(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Selector string expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string selector = info[0].As<Napi::String>().Utf8Value();
    
    try {
        std::string text = browser_->getInnerText(selector);
        return Napi::String::New(env, text);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

// JavaScript execution
Napi::Value BrowserWrapper::ExecuteJavaScript(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "JavaScript code string expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string js_code = info[0].As<Napi::String>().Utf8Value();
    
    try {
        std::string result = browser_->executeJavascriptSync(js_code);
        return Napi::String::New(env, result);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

Napi::Value BrowserWrapper::ExecuteJavaScriptAsync(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction()) {
        Napi::TypeError::New(env, "JavaScript code string and callback function expected").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    std::string js_code = info[0].As<Napi::String>().Utf8Value();
    Napi::Function callback = info[1].As<Napi::Function>();
    
    AsyncWorker* worker = new AsyncWorker(callback, this, "javascript");
    worker->js_param_ = js_code;
    worker->Queue();
    
    return env.Undefined();
}

// Screenshot methods
Napi::Value BrowserWrapper::TakeScreenshot(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    std::string filename = "screenshot.png";
    if (info.Length() > 0 && info[0].IsString()) {
        filename = info[0].As<Napi::String>().Utf8Value();
    }
    
    try {
        bool result = browser_->takeScreenshot(filename);
        return Napi::Boolean::New(env, result);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
        return env.Undefined();
    }
}

// Placeholder implementations for other methods
Napi::Value BrowserWrapper::SelectOption(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, false); // TODO: Implement
}

Napi::Value BrowserWrapper::CheckElement(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, false); // TODO: Implement
}

Napi::Value BrowserWrapper::UncheckElement(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, false); // TODO: Implement
}

Napi::Value BrowserWrapper::FocusElement(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, false); // TODO: Implement
}

Napi::Value BrowserWrapper::CountElements(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Number::New(env, 0); // TODO: Implement
}

Napi::Value BrowserWrapper::GetElementHtml(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, ""); // TODO: Implement
}

Napi::Value BrowserWrapper::GetAttribute(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::String::New(env, ""); // TODO: Implement
}

Napi::Value BrowserWrapper::SetAttribute(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, false); // TODO: Implement
}

Napi::Value BrowserWrapper::TakeScreenshotAsync(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return env.Undefined(); // TODO: Implement
}

Napi::Value BrowserWrapper::WaitForSelector(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, false); // TODO: Implement
}

Napi::Value BrowserWrapper::WaitForNavigation(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, false); // TODO: Implement
}
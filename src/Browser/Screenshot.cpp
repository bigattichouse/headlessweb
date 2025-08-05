#include "Browser.h"
#include "../FileOps/PathUtils.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <gdk/gdk.h>
#include <cairo/cairo.h>
#include <iostream>

// External debug flag
extern bool g_debug;

// Screenshot callback structure
struct ScreenshotData {
    std::string filename;
    GMainLoop* loop;
    bool success;
};

// Callback for screenshot completion
void screenshot_callback(GObject* source_object, GAsyncResult* res, gpointer user_data) {
    ScreenshotData* data = static_cast<ScreenshotData*>(user_data);
    GError* error = NULL;
    
    // Get the snapshot result from WebKit
    GdkTexture* texture = webkit_web_view_get_snapshot_finish(
        WEBKIT_WEB_VIEW(source_object), res, &error);
    
    if (error) {
        std::cerr << "Screenshot error: " << error->message << std::endl;
        g_error_free(error);
        data->success = false;
    } else if (texture) {
        // Get texture dimensions
        int width = gdk_texture_get_width(texture);
        int height = gdk_texture_get_height(texture);
        
        debug_output("Screenshot texture size: " + std::to_string(width) + "x" + std::to_string(height));
        
        // Calculate stride for ARGB32 format
        size_t stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
        
        // Allocate buffer for pixel data
        guchar* pixels = (guchar*)g_malloc(height * stride);
        
        if (pixels) {
            // Download texture data to our buffer
            gdk_texture_download(texture, pixels, stride);
            
            // Create cairo surface from the pixel data
            cairo_surface_t* surface = cairo_image_surface_create_for_data(
                pixels, CAIRO_FORMAT_ARGB32, width, height, stride);
            
            if (surface) {
                // Save surface to PNG file
                cairo_status_t status = cairo_surface_write_to_png(surface, data->filename.c_str());
                
                if (status == CAIRO_STATUS_SUCCESS) {
                    data->success = true;
                    debug_output("Screenshot saved successfully: " + data->filename + 
                               " (" + std::to_string(width) + "x" + std::to_string(height) + ")");
                } else {
                    std::cerr << "Failed to write PNG: " << cairo_status_to_string(status) << std::endl;
                    data->success = false;
                }
                
                // Cleanup cairo surface
                cairo_surface_destroy(surface);
            } else {
                std::cerr << "Failed to create cairo surface" << std::endl;
                data->success = false;
            }
            
            // Free pixel buffer
            g_free(pixels);
        } else {
            std::cerr << "Failed to allocate pixel buffer" << std::endl;
            data->success = false;
        }
        
        // Release texture
        g_object_unref(texture);
    } else {
        std::cerr << "No texture returned from WebKit snapshot" << std::endl;
        data->success = false;
    }
    
    // Signal completion
    if (g_main_loop_is_running(data->loop)) {
        g_main_loop_quit(data->loop);
    }
}

// ========== Screenshot Methods ==========

void Browser::takeScreenshot(const std::string& filename) {
    debug_output("Starting headless visible area screenshot: " + filename);
    
    // Validate screenshot path
    if (!FileOps::PathUtils::isSecurePath(filename)) {
        std::cerr << "Error: Invalid or insecure screenshot path: " + filename << std::endl;
        return;
    }
    
    // Create directory if needed
    std::string directory = FileOps::PathUtils::getDirectory(filename);
    if (!directory.empty() && !FileOps::PathUtils::createDirectoriesIfNeeded(directory)) {
        std::cerr << "Error: Cannot create directory for screenshot: " + directory << std::endl;
        return;
    }
    
    // Ensure proper offscreen viewport and rendering
    ensureProperViewportForScreenshots();
    
    // Wait for rendering to complete using event-driven approach
    if (async_nav_) {
        auto future = async_nav_->waitForRenderingComplete(2000);
        if (future.wait_for(std::chrono::milliseconds(2000)) != std::future_status::ready || !future.get()) {
            // Fallback to readiness check or brief wait
            if (readiness_tracker_ && readiness_tracker_->isFullyReady()) {
                // Already ready, proceed
            } else {
                wait(250); // Reduced fallback wait
            }
        }
    } else {
        wait(250); // Reduced fallback wait
    }
    
    // Check if page is ready
    std::string readyState = executeJavascriptSync("(function() { try { return document.readyState; } catch(e) { return 'error'; } })()");
    if (readyState != "complete" && readyState != "interactive") {
        debug_output("Warning: Page not ready for screenshot (state: " + readyState + ")");
    }
    
    ScreenshotData data;
    data.filename = filename;
    data.loop = main_loop;
    data.success = false;
    
    // Take visible area snapshot using WebKit API (completely offscreen)
    webkit_web_view_get_snapshot(
        webView, 
        WEBKIT_SNAPSHOT_REGION_VISIBLE,
        WEBKIT_SNAPSHOT_OPTIONS_NONE, 
        NULL,  // cancellable
        screenshot_callback, 
        &data
    );
    
    // Wait for screenshot completion
    g_main_loop_run(main_loop);
    
    if (!data.success) {
        std::cerr << "Visible area screenshot failed, trying full page as fallback..." << std::endl;
        takeFullPageScreenshot(filename);
    }
}

void Browser::takeFullPageScreenshot(const std::string& filename) {
    debug_output("Starting headless full page screenshot: " + filename);
    
    // Validate screenshot path
    if (!FileOps::PathUtils::isSecurePath(filename)) {
        std::cerr << "Error: Invalid or insecure screenshot path: " + filename << std::endl;
        return;
    }
    
    // Create directory if needed
    std::string directory = FileOps::PathUtils::getDirectory(filename);
    if (!directory.empty() && !FileOps::PathUtils::createDirectoriesIfNeeded(directory)) {
        std::cerr << "Error: Cannot create directory for screenshot: " + directory << std::endl;
        return;
    }
    
    // Ensure proper offscreen viewport and rendering
    ensureProperViewportForScreenshots();
    
    // Wait for rendering to complete using event-driven approach
    if (async_nav_) {
        auto future = async_nav_->waitForRenderingComplete(2000);
        if (future.wait_for(std::chrono::milliseconds(2000)) != std::future_status::ready || !future.get()) {
            // Fallback to readiness check or brief wait
            if (readiness_tracker_ && readiness_tracker_->isFullyReady()) {
                // Already ready, proceed
            } else {
                wait(250); // Reduced fallback wait
            }
        }
    } else {
        wait(250); // Reduced fallback wait
    }
    
    // Check if page is ready
    std::string readyState = executeJavascriptSync("(function() { try { return document.readyState; } catch(e) { return 'error'; } })()");
    if (readyState != "complete" && readyState != "interactive") {
        debug_output("Warning: Page not ready for screenshot (state: " + readyState + ")");
    }
    
    // Get page dimensions for debugging
    std::string pageDimensions = executeJavascriptSync(
        "(function() { "
        "try { "
        "  return document.documentElement.scrollWidth + 'x' + document.documentElement.scrollHeight; "
        "} catch(e) { "
        "  return 'unknown'; "
        "} "
        "})()");
    debug_output("Page dimensions: " + pageDimensions);
    
    ScreenshotData data;
    data.filename = filename;
    data.loop = main_loop;
    data.success = false;
    
    // Take full document snapshot using WebKit API (completely offscreen)
    webkit_web_view_get_snapshot(
        webView, 
        WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT,
        WEBKIT_SNAPSHOT_OPTIONS_NONE, 
        NULL,  // cancellable
        screenshot_callback, 
        &data
    );
    
    // Wait for screenshot completion
    g_main_loop_run(main_loop);
    
    if (!data.success) {
        std::cerr << "Failed to take full page screenshot" << std::endl;
    }
}

#include "Browser.h"
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
    
    // In newer WebKitGTK, this returns a GdkTexture
    GdkTexture* texture = webkit_web_view_get_snapshot_finish(
        WEBKIT_WEB_VIEW(source_object), res, &error);
    
    if (error) {
        std::cerr << "Screenshot error: " << error->message << std::endl;
        g_error_free(error);
        data->success = false;
    } else if (texture) {
        int width = gdk_texture_get_width(texture);
        int height = gdk_texture_get_height(texture);
        
        // Allocate buffer for pixel data
        size_t stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
        guchar* pixels = (guchar*)g_malloc(height * stride);
        
        // Download texture data
        gdk_texture_download(texture, pixels, stride);
        
        // Create cairo surface from the pixel data
        cairo_surface_t* surface = cairo_image_surface_create_for_data(
            pixels, CAIRO_FORMAT_ARGB32, width, height, stride);
        
        // Save to PNG
        cairo_status_t status = cairo_surface_write_to_png(surface, data->filename.c_str());
        
        if (status == CAIRO_STATUS_SUCCESS) {
            data->success = true;
            debug_output("Screenshot saved successfully to: " + data->filename);
        } else {
            std::cerr << "Failed to write PNG: " << cairo_status_to_string(status) << std::endl;
            data->success = false;
        }
        
        // Cleanup
        cairo_surface_destroy(surface);
        g_free(pixels);
        g_object_unref(texture);
    } else {
        std::cerr << "No texture returned from snapshot" << std::endl;
        data->success = false;
    }
    
    if (g_main_loop_is_running(data->loop)) {
        g_main_loop_quit(data->loop);
    }
}

// ========== Screenshot Methods ==========

void Browser::takeScreenshot(const std::string& filename) {
    ScreenshotData data;
    data.filename = filename;
    data.loop = main_loop;
    data.success = false;
    
    debug_output("Taking screenshot of visible area: " + filename);
    
    webkit_web_view_get_snapshot(webView, 
                                WEBKIT_SNAPSHOT_REGION_VISIBLE,
                                WEBKIT_SNAPSHOT_OPTIONS_NONE, 
                                NULL,
                                screenshot_callback, 
                                &data);
    
    // Wait for the screenshot to complete
    g_main_loop_run(main_loop);
    
    if (!data.success) {
        std::cerr << "Failed to take screenshot" << std::endl;
    }
}

void Browser::takeFullPageScreenshot(const std::string& filename) {
    ScreenshotData data;
    data.filename = filename;
    data.loop = main_loop;
    data.success = false;
    
    debug_output("Taking full page screenshot: " + filename);
    
    webkit_web_view_get_snapshot(webView, 
                                WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT,
                                WEBKIT_SNAPSHOT_OPTIONS_NONE, 
                                NULL,
                                screenshot_callback, 
                                &data);
    
    // Wait for the screenshot to complete
    g_main_loop_run(main_loop);
    
    if (!data.success) {
        std::cerr << "Failed to take full page screenshot" << std::endl;
    }
}

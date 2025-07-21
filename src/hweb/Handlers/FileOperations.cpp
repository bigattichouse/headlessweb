#include "FileOperations.h"
#include "../Output.h"
#include "../Services/ManagerRegistry.h"
#include <sstream>

namespace HWeb {

FileOperationHandler::FileOperationHandler() {
}

FileOperationHandler::~FileOperationHandler() {
}

void FileOperationHandler::configure_managers(const FileOperationSettings& settings) {
    settings_ = settings;
    
    auto& upload_manager = ManagerRegistry::get_upload_manager();
    auto& download_manager = ManagerRegistry::get_download_manager();
    
    upload_manager.setMaxFileSize(settings.max_file_size);
    upload_manager.setDefaultTimeout(settings.upload_timeout);
    
    if (!settings.download_dir.empty()) {
        download_manager.setDownloadDirectory(settings.download_dir);
    }
    download_manager.setDefaultTimeout(settings.download_timeout);
}

int FileOperationHandler::handle_command(Browser& browser, const Command& cmd) {
    if (cmd.type == "upload") {
        return handle_upload_command(browser, cmd);
    } else if (cmd.type == "upload-multiple") {
        return handle_upload_multiple_command(browser, cmd);
    } else if (cmd.type == "download-wait") {
        return handle_download_wait_command(cmd);
    } else if (cmd.type == "download-wait-multiple") {
        return handle_download_wait_multiple_command(cmd);
    }
    
    return 0;
}

int FileOperationHandler::handle_upload_command(Browser& browser, const Command& cmd) {
    auto& upload_manager = ManagerRegistry::get_upload_manager();
    
    FileOps::UploadCommand upload_cmd;
    upload_cmd.selector = cmd.selector;
    upload_cmd.filepath = cmd.value;
    upload_cmd.timeout_ms = cmd.timeout;
    upload_cmd.max_file_size = settings_.max_file_size;
    upload_cmd.allowed_types = settings_.allowed_types;
    upload_cmd.json_output = Output::is_json_mode();
    upload_cmd.silent = Output::is_silent_mode();
    
    FileOps::UploadResult result = upload_manager.uploadFile(browser, upload_cmd);
    
    if (result == FileOps::UploadResult::SUCCESS) {
        Output::info("File uploaded successfully: " + cmd.value);
        return 0;
    } else {
        Output::error("Upload failed: " + upload_manager.getErrorMessage(result, cmd.value));
        return static_cast<int>(result);
    }
}

int FileOperationHandler::handle_upload_multiple_command(Browser& browser, const Command& cmd) {
    auto& upload_manager = ManagerRegistry::get_upload_manager();
    
    std::vector<std::string> filepaths;
    std::stringstream ss(cmd.value);
    std::string filepath;
    while (std::getline(ss, filepath, ',')) {
        filepath.erase(0, filepath.find_first_not_of(" \t"));
        filepath.erase(filepath.find_last_not_of(" \t") + 1);
        if (!filepath.empty()) {
            filepaths.push_back(filepath);
        }
    }
    
    FileOps::UploadResult result = upload_manager.uploadMultipleFiles(browser, cmd.selector, filepaths, cmd.timeout);
    
    if (result == FileOps::UploadResult::SUCCESS) {
        Output::info("Multiple files uploaded successfully");
        return 0;
    } else {
        Output::error("Multiple upload failed: " + upload_manager.getErrorMessage(result, cmd.value));
        return static_cast<int>(result);
    }
}

int FileOperationHandler::handle_download_wait_command(const Command& cmd) {
    auto& download_manager = ManagerRegistry::get_download_manager();
    
    FileOps::DownloadCommand download_cmd;
    download_cmd.filename_pattern = cmd.selector;
    download_cmd.download_dir = settings_.download_dir;
    download_cmd.timeout_ms = cmd.timeout;
    download_cmd.json_output = Output::is_json_mode();
    download_cmd.silent = Output::is_silent_mode();
    
    FileOps::DownloadResult result = download_manager.waitForDownload(download_cmd);
    
    if (result == FileOps::DownloadResult::SUCCESS) {
        Output::info("Download completed: " + cmd.selector);
        return 0;
    } else {
        Output::error("Download failed: " + download_manager.getErrorMessage(result, cmd.selector));
        return static_cast<int>(result);
    }
}

int FileOperationHandler::handle_download_wait_multiple_command(const Command& cmd) {
    auto& download_manager = ManagerRegistry::get_download_manager();
    
    std::vector<std::string> patterns;
    std::stringstream ss(cmd.value);
    std::string pattern;
    while (std::getline(ss, pattern, ',')) {
        pattern.erase(0, pattern.find_first_not_of(" \t"));
        pattern.erase(pattern.find_last_not_of(" \t") + 1);
        if (!pattern.empty()) {
            patterns.push_back(pattern);
        }
    }
    
    FileOps::DownloadResult result = download_manager.waitForMultipleDownloads(patterns, settings_.download_dir, cmd.timeout);
    
    if (result == FileOps::DownloadResult::SUCCESS) {
        Output::info("All downloads completed");
        return 0;
    } else {
        Output::error("Multiple download failed: " + download_manager.getErrorMessage(result, cmd.value));
        return static_cast<int>(result);
    }
}

} // namespace HWeb
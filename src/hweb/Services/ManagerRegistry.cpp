#include "ManagerRegistry.h"
#include <stdexcept>

namespace HWeb {

std::unique_ptr<Assertion::Manager> ManagerRegistry::assertion_manager_;
std::unique_ptr<FileOps::UploadManager> ManagerRegistry::upload_manager_;
std::unique_ptr<FileOps::DownloadManager> ManagerRegistry::download_manager_;
bool ManagerRegistry::initialized_ = false;

void ManagerRegistry::initialize() {
    if (initialized_) {
        return;
    }
    
    assertion_manager_ = std::make_unique<Assertion::Manager>();
    upload_manager_ = std::make_unique<FileOps::UploadManager>();
    download_manager_ = std::make_unique<FileOps::DownloadManager>();
    
    initialized_ = true;
}

void ManagerRegistry::cleanup() {
    assertion_manager_.reset();
    upload_manager_.reset();
    download_manager_.reset();
    initialized_ = false;
}

Assertion::Manager& ManagerRegistry::get_assertion_manager() {
    if (!initialized_ || !assertion_manager_) {
        throw std::runtime_error("ManagerRegistry not initialized or assertion manager not available");
    }
    return *assertion_manager_;
}

FileOps::UploadManager& ManagerRegistry::get_upload_manager() {
    if (!initialized_ || !upload_manager_) {
        throw std::runtime_error("ManagerRegistry not initialized or upload manager not available");
    }
    return *upload_manager_;
}

FileOps::DownloadManager& ManagerRegistry::get_download_manager() {
    if (!initialized_ || !download_manager_) {
        throw std::runtime_error("ManagerRegistry not initialized or download manager not available");
    }
    return *download_manager_;
}

bool ManagerRegistry::is_initialized() {
    return initialized_;
}

} // namespace HWeb
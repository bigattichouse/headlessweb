#pragma once

#include <memory>
#include "../../Assertion/Manager.h"
#include "../../FileOps/UploadManager.h"
#include "../../FileOps/DownloadManager.h"

namespace HWeb {

class ManagerRegistry {
private:
    static std::unique_ptr<Assertion::Manager> assertion_manager_;
    static std::unique_ptr<FileOps::UploadManager> upload_manager_;
    static std::unique_ptr<FileOps::DownloadManager> download_manager_;
    static bool initialized_;

public:
    static void initialize();
    static void cleanup();
    
    static Assertion::Manager& get_assertion_manager();
    static FileOps::UploadManager& get_upload_manager();
    static FileOps::DownloadManager& get_download_manager();
    
    static bool is_initialized();
};

} // namespace HWeb
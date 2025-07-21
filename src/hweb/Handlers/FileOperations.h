#pragma once

#include "../Types.h"
#include "../../Browser/Browser.h"
#include "../../FileOps/Types.h"

namespace HWeb {

class FileOperationHandler {
public:
    FileOperationHandler();
    ~FileOperationHandler();
    
    int handle_command(Browser& browser, const Command& cmd);
    void configure_managers(const FileOperationSettings& settings);
    
private:
    int handle_upload_command(Browser& browser, const Command& cmd);
    int handle_upload_multiple_command(Browser& browser, const Command& cmd);
    int handle_download_wait_command(const Command& cmd);
    int handle_download_wait_multiple_command(const Command& cmd);
    
    // Configuration cache
    FileOperationSettings settings_;
};

} // namespace HWeb
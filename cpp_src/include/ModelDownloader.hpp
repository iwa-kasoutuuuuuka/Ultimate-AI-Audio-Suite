#pragma once
#include <string>
#include <functional>

class ModelDownloader {
public:
    static bool checkAndDownload(const std::string& target_dir, std::function<void(float)> progress_cb);
};

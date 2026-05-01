#include "ModelDownloader.hpp"
#include <windows.h>
#include <filesystem>
#include <vector>
#include <iostream>

bool ModelDownloader::checkAndDownload(const std::string& target_dir, std::function<void(float)> progress_cb) {
    namespace fs = std::filesystem;
    if (!fs::exists(target_dir)) fs::create_directories(target_dir);

    std::vector<std::pair<std::string, std::string>> models = {
        {"enhancer_encoder.onnx", "https://huggingface.co/ResembleAI/resemble-enhance/resolve/main/enhancer_encoder.onnx"},
        {"enhancer_decoder.onnx", "https://huggingface.co/ResembleAI/resemble-enhance/resolve/main/enhancer_decoder.onnx"},
        {"cfm_wn.onnx", "https://huggingface.co/ResembleAI/resemble-enhance/resolve/main/cfm_wn.onnx"},
        {"enhancer_vocoder.pt", "https://huggingface.co/ResembleAI/resemble-enhance/resolve/main/enhancer_vocoder.pt"},
        {"hparams.txt", "https://huggingface.co/ResembleAI/resemble-enhance/resolve/main/hparams.txt"}
    };

    for (size_t i = 0; i < models.size(); ++i) {
        fs::path p = fs::path(target_dir) / models[i].first;
        if (!fs::exists(p)) {
            std::cout << "[Downloader] Downloading " << models[i].first << "..." << std::endl;
            std::string cmd = "powershell -Command \"Invoke-WebRequest -Uri " + models[i].second + " -OutFile '" + p.string() + "'\"";
            system(cmd.c_str());
        }
        if (progress_cb) progress_cb((float)(i + 1) / models.size());
    }

    return true;
}

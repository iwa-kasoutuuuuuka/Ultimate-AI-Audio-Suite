#include "AudioProcessor.hpp"
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <filesystem>

struct AudioProcessor::Impl {
    std::string ffmpeg_path = "bin/ffmpeg.exe";
};

AudioProcessor::AudioProcessor() : pimpl(std::make_unique<Impl>()) {
    namespace fs = std::filesystem;
    if (!fs::exists(pimpl->ffmpeg_path)) {
        // Try adjacent to exe
        pimpl->ffmpeg_path = "ffmpeg.exe"; 
    }
}
AudioProcessor::~AudioProcessor() {}

bool AudioProcessor::loadAudio(const std::string& path, int target_sr, std::vector<float>& out_samples) {
    std::string cmd = pimpl->ffmpeg_path + " -i \"" + path + "\" -f f32le -ac 1 -ar " + std::to_string(target_sr) + " -v quiet -";
    
    FILE* pipe = _popen(cmd.c_str(), "rb");
    if (!pipe) return false;

    float buffer[4096];
    out_samples.clear();
    while (true) {
        size_t bytesRead = fread(buffer, sizeof(float), 4096, pipe);
        if (bytesRead == 0) break;
        out_samples.insert(out_samples.end(), buffer, buffer + bytesRead);
    }

    _pclose(pipe);
    return !out_samples.empty();
}

bool AudioProcessor::saveAudio(const std::string& path, const std::vector<float>& samples, int sr, const std::string& format) {
    std::string cmd = pimpl->ffmpeg_path + " -y -f f32le -ac 1 -ar " + std::to_string(sr) + " -i - ";
    if (format == "mp3") {
        cmd += "-b:a 192k \"" + path + "\"";
    } else {
        cmd += "\"" + path + "\""; // Default wav
    }

    FILE* pipe = _popen(cmd.c_str(), "wb");
    if (!pipe) return false;

    fwrite(samples.data(), sizeof(float), samples.size(), pipe);
    _pclose(pipe);
    return true;
}

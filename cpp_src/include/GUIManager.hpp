#pragma once
#include <imgui.h>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <atomic>

#include "Strings.hpp"

struct GLFWwindow;

class GUIManager {
public:
    GUIManager();
    ~GUIManager();

    bool init();
    void run();
    void cleanup();
    void addLog(const std::string& msg); // Moved to public
    
    // Public for callbacks
    std::atomic<bool> is_monitoring{false};
    std::vector<float> mic_visual_buffer;
    std::mutex data_mutex;

private:
    Language current_lang = Language::JP;

    void renderUI();
    void processAudio();
    void renderWaveform(const char* label, const std::vector<float>& data, ImVec4 color);
    void renderSpectrogram(const char* label, const std::vector<float>& data, int w, int h);
    void renderLog();

    GLFWwindow* window = nullptr;
    
    // UI State
    char input_path[1024] = "";
    char output_path[1024] = "enhanced_output.wav";
    float denoise_strength = 0.5f;
    bool use_gpu = true; // Enabled by default
    bool is_batch_mode = false;
    
    std::atomic<bool> is_processing{false};
    std::atomic<float> progress{0.0f};

    // Data
    std::vector<float> original_waveform;
    std::vector<float> enhanced_waveform;
    std::vector<float> original_spectrogram;
    std::vector<float> enhanced_spectrogram;
    int spec_w = 0, spec_h = 0;
    
    std::vector<std::string> batch_queue;
    
    // Log
    std::vector<std::string> logs;
    std::mutex log_mutex;
    std::string models_dir;

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

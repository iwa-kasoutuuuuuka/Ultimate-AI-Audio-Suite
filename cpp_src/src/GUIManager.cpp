#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "GUIManager.hpp"
#include "AIEnhancer.hpp"
#include "AudioProcessor.hpp"
#include "DSPModule.hpp"
#include "ModelDownloader.hpp"

#include <imgui.h>
#include <implot.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <shobjidl.h> 
#include <iostream>
#include <thread>
#include <filesystem>
#include <mutex>
#include <atomic>

struct GUIManager::Impl {
    AIEnhancer enhancer;
    AudioProcessor processor;
    ma_device device;
    bool device_active = false;
};

// Mic Callback
static void ma_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    GUIManager* gui = (GUIManager*)pDevice->pUserData;
    if (!gui || !gui->is_monitoring) return;
    
    // Copy input to visual buffer
    const float* in = (const float*)pInput;
    std::lock_guard<std::mutex> lock(gui->data_mutex);
    if (gui->mic_visual_buffer.size() > 44100) gui->mic_visual_buffer.erase(gui->mic_visual_buffer.begin(), gui->mic_visual_buffer.begin() + frameCount);
    for(ma_uint32 i=0; i<frameCount; ++i) gui->mic_visual_buffer.push_back(in[i]);
    
    // Passthrough for monitoring
    if (pOutput) memcpy(pOutput, in, frameCount * sizeof(float));
}

static GUIManager* g_instance = nullptr;

GUIManager::GUIManager() : pimpl(std::make_unique<Impl>()) {
    g_instance = this;
}
GUIManager::~GUIManager() { 
    if (pimpl->device_active) ma_device_uninit(&pimpl->device);
    g_instance = nullptr;
    cleanup(); 
}

void GUIManager::addLog(const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    logs.push_back(msg);
}

static std::string openFileDialog() {
    std::string result = "";
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return "";

    IFileOpenDialog *pFileOpen = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    
    if (SUCCEEDED(hr)) {
        COMDLG_FILTERSPEC fileTypes[] = { { L"Audio Files", L"*.wav;*.mp3;*.flac;*.m4a" }, { L"All Files", L"*.*" } };
        pFileOpen->SetFileTypes(2, fileTypes);
        hr = pFileOpen->Show(NULL);
        if (SUCCEEDED(hr)) {
            IShellItem *pItem = nullptr;
            if (SUCCEEDED(pFileOpen->GetResult(&pItem))) {
                PWSTR pszFilePath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    int size = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                    result.resize(size - 1);
                    WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &result[0], size, NULL, NULL);
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
    
    if (hr != RPC_E_CHANGED_MODE) CoUninitialize();
    return result;
}

bool GUIManager::init() {
    if (!glfwInit()) return false;
    window = glfwCreateWindow(1280, 850, "RESEMBLE ENHANCE - ULTIMATE AUDIO SUITE", nullptr, nullptr);
    if (!window) return false;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    glfwSetDropCallback(window, [](GLFWwindow* w, int count, const char** paths) {
        if (count > 0 && g_instance) {
            strncpy(g_instance->input_path, paths[0], sizeof(g_instance->input_path) - 1);
            g_instance->addLog("[System] File dropped: " + std::string(paths[0]));
        }
    });

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    const char* font_path = "C:\\Windows\\Fonts\\meiryo.ttc";
    if (!std::filesystem::exists(font_path)) font_path = "C:\\Windows\\Fonts\\msgothic.ttc";
    if (std::filesystem::exists(font_path)) io.Fonts->AddFontFromFileTTF(font_path, 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Initialize Mic Device
    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    config.capture.format = ma_format_f32;
    config.capture.channels = 1;
    config.playback.format = ma_format_f32;
    config.playback.channels = 1;
    config.sampleRate = 44100;
    config.dataCallback = ma_data_callback;
    config.pUserData = this;

    if (ma_device_init(NULL, &config, &pimpl->device) == MA_SUCCESS) {
        pimpl->device_active = true;
    }

    addLog("Ultimate Dashboard Ready.");
    
    models_dir = "models_onnx";
    if (std::filesystem::exists(models_dir)) {
        pimpl->enhancer.setLogCallback([this](const std::string& msg) { addLog(msg); });
        pimpl->enhancer.loadModels(models_dir, use_gpu);
    }
    
    return true;
}

void GUIManager::renderWaveform(const char* label, const std::vector<float>& data, ImVec4 color) {
    if (ImPlot::BeginPlot(label, ImVec2(-1, 140), ImPlotFlags_NoMenus)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -1.1, 1.1, ImGuiCond_Always);
        ImPlot::SetNextLineStyle(color);
        std::lock_guard<std::mutex> lock(data_mutex);
        if (!data.empty()) ImPlot::PlotLine("Wave", data.data(), (int)data.size());
        ImPlot::EndPlot();
    }
}

void GUIManager::renderSpectrogram(const char* label, const std::vector<float>& data, int w, int h) {
    if (w <= 0 || h <= 0 || data.empty()) return;
    if (ImPlot::BeginPlot(label, ImVec2(-1, 180), ImPlotFlags_NoMenus)) {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::PlotHeatmap("Spec", data.data(), h, w, 0.0f, 1.0f, nullptr, ImPlotPoint(0,0), ImPlotPoint(w,h));
        ImPlot::EndPlot();
    }
}

void GUIManager::renderUI() {
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Dashboard", nullptr, ImGuiWindowFlags_NoDecoration);

    // Language Toggle
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 150, 5));
    if (ImGui::Button(UIStrings::get("lang_btn", current_lang).c_str(), ImVec2(140, 20))) {
        current_lang = (current_lang == Language::JP) ? Language::EN : Language::JP;
    }

    // Title and Hardware Status
    ImGui::SetCursorPos(ImVec2(10, 5));
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), UIStrings::get("app_title", current_lang).c_str());
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 350);
    if (use_gpu) {
        ImGui::TextColored(ImVec4(0.4f, 1, 0.4f, 1), UIStrings::get("device_gpu", current_lang).c_str());
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), UIStrings::get("device_cpu", current_lang).c_str());
    }

    ImGui::SameLine(ImGui::GetWindowWidth() - 250);
    if (is_processing) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), UIStrings::get("status_proc", current_lang).c_str());
    } else {
        ImGui::TextColored(ImVec4(0, 1, 0.5f, 1), UIStrings::get("status_ready", current_lang).c_str());
    }
    
    ImGui::Separator();

    if (ImGui::BeginTabBar("MainTabs")) {
        // TAB 1: SINGLE
        if (ImGui::BeginTabItem(UIStrings::get("tab_single", current_lang).c_str())) {
            ImGui::Spacing();
            ImGui::InputText("##path", input_path, sizeof(input_path));
            ImGui::SameLine();
            if (ImGui::Button(UIStrings::get("browse", current_lang).c_str())) {
                std::string s = openFileDialog();
                if (!s.empty()) strncpy(input_path, s.c_str(), sizeof(input_path)-1);
            }
            if (ImGui::Button(UIStrings::get("load", current_lang).c_str())) {
                std::vector<float> tmp;
                if (pimpl->processor.loadAudio(input_path, 44100, tmp)) {
                    std::lock_guard<std::mutex> lock(data_mutex);
                    original_waveform = std::move(tmp);
                    auto stft = DSPModule::extractSTFT(original_waveform, {2048, 420, 2048});
                    original_spectrogram = stft.magnitude;
                    spec_w = stft.n_frames; spec_h = stft.n_bins;
                    addLog(UIStrings::get("log_loaded", current_lang));
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(UIStrings::get("enhance", current_lang).c_str()) && !is_processing) processAudio();

            if (ImGui::BeginChild("Visuals", ImVec2(0, 380))) {
                if (!original_waveform.empty()) {
                    renderWaveform("##orig", original_waveform, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
                    renderSpectrogram("##orig_s", original_spectrogram, spec_w, spec_h);
                }
                if (!enhanced_waveform.empty()) {
                    renderWaveform("##enh", enhanced_waveform, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
                    renderSpectrogram("##enh_s", enhanced_spectrogram, spec_w, spec_h);
                }
                ImGui::EndChild();
            }
            ImGui::EndTabItem();
        }

        // TAB 2: BATCH
        if (ImGui::BeginTabItem(UIStrings::get("tab_batch", current_lang).c_str())) {
            if (ImGui::Button(UIStrings::get("add_queue", current_lang).c_str())) {
                std::string s = openFileDialog();
                if (!s.empty()) batch_queue.push_back(s);
            }
            ImGui::SameLine();
            if (ImGui::Button(UIStrings::get("clear_queue", current_lang).c_str())) batch_queue.clear();
            ImGui::SameLine();
            if (ImGui::Button(UIStrings::get("start_batch", current_lang).c_str()) && !is_processing) {
                std::thread([this]() {
                    is_processing = true;
                    for (size_t i = 0; i < batch_queue.size(); ++i) {
                        std::vector<float> wav, out;
                        if (pimpl->processor.loadAudio(batch_queue[i], 44100, wav)) {
                            if (pimpl->enhancer.process(wav, denoise_strength, out, nullptr)) {
                                pimpl->processor.saveAudio(batch_queue[i] + "_enh.wav", out, 44100, "wav");
                            }
                        }
                        progress = (float)(i+1)/batch_queue.size();
                    }
                    is_processing = false;
                }).detach();
            }
            ImGui::BeginChild("BatchList", ImVec2(0, 300), true);
            for (auto& f : batch_queue) ImGui::BulletText("%s", std::filesystem::path(f).filename().string().c_str());
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // TAB 3: LIVE
        if (ImGui::BeginTabItem(UIStrings::get("tab_live", current_lang).c_str())) {
            ImGui::TextUnformatted(UIStrings::get("mon_desc", current_lang).c_str());
            if (ImGui::Button(is_monitoring ? UIStrings::get("stop_mon", current_lang).c_str() : UIStrings::get("start_mon", current_lang).c_str(), ImVec2(200, 50))) {
                is_monitoring = !is_monitoring;
                if (is_monitoring) ma_device_start(&pimpl->device);
                else ma_device_stop(&pimpl->device);
            }
            if (!mic_visual_buffer.empty()) {
                renderWaveform("Live Mic", mic_visual_buffer, ImVec4(1, 0.5f, 0, 1));
            }
            ImGui::EndTabItem();
        }

        // TAB 4: SETTINGS
        if (ImGui::BeginTabItem("Settings")) {
            if (ImGui::Button(UIStrings::get("setup_btn", current_lang).c_str(), ImVec2(-1, 40))) {
                std::thread([this]() {
                    is_processing = true;
                    ModelDownloader::checkAndDownload(models_dir, [this](float p) { progress = p; });
                    pimpl->enhancer.loadModels(models_dir, use_gpu);
                    is_processing = false;
                }).detach();
            }
            ImGui::Spacing();
            ImGui::SliderFloat(UIStrings::get("denoise", current_lang).c_str(), &denoise_strength, 0.0f, 1.0f);
            if (ImGui::Checkbox(UIStrings::get("gpu_accel", current_lang).c_str(), &use_gpu)) {
                pimpl->enhancer.loadModels(models_dir, use_gpu);
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // Footer
    ImGui::SetCursorPos(ImVec2(10, ImGui::GetWindowHeight() - 210));
    ImGui::Separator();
    if (is_processing) ImGui::ProgressBar(progress, ImVec2(-1, 20));
    ImGui::BeginChild("Logs", ImVec2(0, 160), true);
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        for (auto& l : logs) ImGui::TextUnformatted(l.c_str());
        ImGui::SetScrollHereY(1.0);
    }
    ImGui::EndChild();
    ImGui::End();
}

void GUIManager::processAudio() {
    is_processing = true;
    std::thread([this]() {
        std::vector<float> in, out;
        { std::lock_guard<std::mutex> l(data_mutex); in = original_waveform; }
        if (pimpl->enhancer.process(in, denoise_strength, out, [this](float p){ progress = p; })) {
            std::lock_guard<std::mutex> l(data_mutex);
            enhanced_waveform = std::move(out);
            auto stft = DSPModule::extractSTFT(enhanced_waveform, {2048, 420, 2048});
            enhanced_spectrogram = stft.magnitude;
        }
        is_processing = false;
    }).detach();
}

void GUIManager::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        renderUI();
        ImGui::Render();
        int w, h; glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h); glClearColor(0.1f, 0.1f, 0.12f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}
void GUIManager::cleanup() {
    if (window) {
        ImPlot::DestroyContext(); ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(); glfwDestroyWindow(window); glfwTerminate();
    }
}
void GUIManager::renderLog() {}

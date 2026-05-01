#pragma once
#include <string>
#include <windows.h>
#include <vector>

enum class Language { EN, JP };

struct UIStrings {
    static std::string toUTF8(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
        std::vector<char> buffer(size);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buffer.data(), size, NULL, NULL);
        return std::string(buffer.data());
    }

    static std::string get(const std::string& id, Language lang) {
        bool is_jp = (lang == Language::JP);

        if (id == "app_title") return is_jp ? "RESEMBLE ENHANCE - ULTIMATE AUDIO SUITE" : "RESEMBLE ENHANCE - ULTIMATE AUDIO SUITE";
        if (id == "status_proc") return is_jp ? toUTF8(L"\u51e6\u7406\u4e2d...") : "PROCESSING...";
        if (id == "status_ready") return is_jp ? toUTF8(L"\u30b7\u30b9\u30c6\u30e0\u5f85\u6a5f\u4e2d") : "SYSTEM READY";
        if (id == "sec_1") return is_jp ? toUTF8(L"1. \u5165\u529b\u30bd\u30fc\u30b9") : "1. Input Audio Source";
        if (id == "browse") return is_jp ? toUTF8(L"\u53c2\u7167...") : "BROWSE...";
        if (id == "tip_dd") return is_jp ? toUTF8(L"(\u30d2\u30f3\u30c8: .wav\u30d5\u30a1\u30a4\u30eb\u3092\u30c9\u30e9\u30c3\u30b0\uff06\u30c9\u30ed\u30c3\u30d7\u3067\u304d\u307e\u3059)") : "(Tip: You can drag and drop audio files here)";
        if (id == "sec_2") return is_jp ? toUTF8(L"2. AI\u30d1\u30e9\u30e1\u30fc\u30bf\u8a2d\u5b9a") : "2. AI Parameters & Configuration";
        if (id == "denoise") return is_jp ? toUTF8(L"\u30ce\u30a4\u30ba\u9664\u53bb\u5f37\u5ea6") : "Denoise Strength";
        if (id == "steps") return is_jp ? toUTF8(L"\u63a8\u8ad6\u30b9\u30c6\u30c3\u30d7\u6570") : "Solver Steps";
        if (id == "tau") return is_jp ? toUTF8(L"\u6e29\u5ea6 (tau)") : "Temperature (tau)";
        if (id == "load") return is_jp ? toUTF8(L"\u97f3\u58f0\u3092\u8aad\u307f\u8fbc\u3080") : "LOAD AUDIO";
        if (id == "enhance") return is_jp ? toUTF8(L"AI\u9ad8\u97f3\u8cea\u5316\u3092\u5b9f\u884c") : "ENHANCE AUDIO";
        if (id == "sec_3") return is_jp ? toUTF8(L"3. \u97f3\u58f0\u89e3\u6790\u3068\u51fa\u529b") : "3. Audio Analysis & Output";
        if (id == "wave_orig") return is_jp ? toUTF8(L"\u30aa\u30ea\u30b8\u30ca\u30eb\u6ce2\u5f62") : "Original Waveform";
        if (id == "wave_enh") return is_jp ? toUTF8(L"\u51e6\u7406\u5f8c\u6ce2\u5f62") : "Enhanced Waveform";
        if (id == "save") return is_jp ? toUTF8(L"\u4fdd\u5b58 (.WAV)") : "SAVE (.WAV)";
        if (id == "play_enh") return is_jp ? toUTF8(L"\u51e6\u7406\u5f8c\u3092\u518d\u751f") : "PLAY ENHANCED";
        if (id == "play_orig") return is_jp ? toUTF8(L"\u30aa\u30ea\u30b8\u30ca\u30eb\u3092\u518d\u751f") : "PLAY ORIGINAL";
        if (id == "stop") return is_jp ? toUTF8(L"\u505c\u6b62") : "STOP";
        if (id == "sec_4") return is_jp ? toUTF8(L"4. \u30b7\u30b9\u30c6\u30e0\u30ed\u30b0") : "4. System Logs";
        if (id == "copy") return is_jp ? toUTF8(L"\u30ed\u30b0\u3092\u30b3\u30d4\u30fc") : "COPY LOGS";
        if (id == "log_loaded") return is_jp ? toUTF8(L"[System] \u97f3\u58f0\u3092\u8aad\u307f\u8fbc\u307f\u307e\u3057\u305f\u3002") : "[System] Audio loaded.";
        if (id == "log_err_load") return is_jp ? toUTF8(L"[Error] \u97f3\u58f0\u30d5\u30a1\u30a4\u30eb\u3092\u5148\u306b\u8aad\u307f\u8fbc\u3093\u3067\u304f\u3060\u3055\u3044\u3002") : "[Error] Load audio first.";
        if (id == "log_saved") return is_jp ? toUTF8(L"[System] \u30d5\u30a1\u30a4\u30eb\u3092\u4fdd\u5b58\u3057\u307e\u3057\u305f\u3002") : "[System] File saved.";
        if (id == "tab_single") return is_jp ? toUTF8(L"\u5358\u4f53\u30d5\u30a1\u30a4\u30eb") : "Single File";
        if (id == "tab_batch") return is_jp ? toUTF8(L"\u4e00\u62ec\u51e6\u7406") : "Batch Mode";
        if (id == "gpu_accel") return is_jp ? toUTF8(L"GPU \u52a0\u901f (DirectML)") : "GPU Acceleration (DirectML)";
        if (id == "add_queue") return is_jp ? toUTF8(L"\u30ad\u30e5\u30fc\u306b\u8ffd\u52a0") : "Add to Queue";
        if (id == "clear_queue") return is_jp ? toUTF8(L"\u30ad\u30e5\u30fc\u3092\u30af\u30ea\u30a2") : "Clear Queue";
        if (id == "start_batch") return is_jp ? toUTF8(L"\u4e00\u62ec\u51e6\u7406\u958b\u59cb") : "Start Batch";
        if (id == "device_cpu") return is_jp ? toUTF8(L"\u30c7\u30d0\u30a4\u30b9: CPU") : "Device: CPU";
        if (id == "device_gpu") return is_jp ? toUTF8(L"\u30c7\u30d0\u30a4\u30b9: GPU") : "Device: GPU";
        if (id == "sec_batch") return is_jp ? toUTF8(L"\u4e00\u62ec\u51e6\u7406\u30ea\u30b9\u30c8") : "Batch List";
        if (id == "tab_live") return is_jp ? toUTF8(L"\u30e9\u30a4\u30d6\u30e2\u30cb\u30bf\u30fc") : "Live Monitor";
        if (id == "start_mon") return is_jp ? toUTF8(L"\u30e2\u30cb\u30bf\u30fc\u958b\u59cb") : "Start Monitoring";
        if (id == "stop_mon") return is_jp ? toUTF8(L"\u30e2\u30cb\u30bf\u30fc\u505c\u6b62") : "Stop Monitoring";
        if (id == "mon_desc") return is_jp ? toUTF8(L"\u30de\u30a4\u30af\u97f3\u58f0\u3092\u30ea\u30a2\u30eb\u30bf\u30a4\u30e0\u3067AI\u51e6\u7406\u3057\u307e\u3059 (\u4f4e\u9045\u5ef6\u30e2\u30fc\u30c9)") : "AI Enhance mic input in real-time (Low Latency Mode)";
        if (id == "setup_btn") return is_jp ? toUTF8(L"AI\u30e2\u30c7\u30eb\u3092\u30bb\u30c3\u30c8\u30a2\u30c3\u30d7") : "Setup AI Models";
        if (id == "fmt_wav") return is_jp ? toUTF8(L"WAV\u5f62\u5f0f") : "WAV Format";
        if (id == "fmt_mp3") return is_jp ? toUTF8(L"MP3\u5f62\u5f0f") : "MP3 Format";
        if (id == "lang_btn") return is_jp ? "English" : toUTF8(L"\u65e5\u672c\u8a9e");

        return "???";
    }
};
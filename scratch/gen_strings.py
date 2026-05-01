import sys

def to_ucn(text):
    return "".join(f"\\u{ord(c):04x}" if ord(c) > 127 else c for c in text)

def safe_str(text, is_jp):
    ucn = to_ucn(text)
    if ucn == text: # pure ascii
        return f"\"{text}\""
    return f"toUTF8(L\"{ucn}\")"

data = {
    "app_title": "RESEMBLE ENHANCE - ULTIMATE AUDIO SUITE",
    "status_proc": "処理中...",
    "status_ready": "システム待機中",
    "sec_1": "1. 入力ソース",
    "browse": "参照...",
    "tip_dd": "(ヒント: .wavファイルをドラッグ＆ドロップできます)",
    "sec_2": "2. AIパラメータ設定",
    "denoise": "ノイズ除去強度",
    "steps": "推論ステップ数",
    "tau": "温度 (tau)",
    "load": "音声を読み込む",
    "enhance": "AI高音質化を実行",
    "sec_3": "3. 音声解析と出力",
    "wave_orig": "オリジナル波形",
    "wave_enh": "処理後波形",
    "save": "保存 (.WAV)",
    "play_enh": "処理後を再生",
    "play_orig": "オリジナルを再生",
    "stop": "停止",
    "sec_4": "4. システムログ",
    "copy": "ログをコピー",
    "log_loaded": "[System] 音声を読み込みました。",
    "log_err_load": "[Error] 音声ファイルを先に読み込んでください。",
    "log_saved": "[System] ファイルを保存しました。",
    "tab_single": "単体ファイル",
    "tab_batch": "一括処理",
    "gpu_accel": "GPU 加速 (DirectML)",
    "add_queue": "キューに追加",
    "clear_queue": "キューをクリア",
    "start_batch": "一括処理開始",
    "device_cpu": "デバイス: CPU",
    "device_gpu": "デバイス: GPU",
    "sec_batch": "一括処理リスト",
    "tab_live": "ライブモニター",
    "start_mon": "モニター開始",
    "stop_mon": "モニター停止",
    "mon_desc": "マイク音声をリアルタイムでAI処理します (低遅延モード)",
    "setup_btn": "AIモデルをセットアップ",
    "fmt_wav": "WAV形式",
    "fmt_mp3": "MP3形式"
}

en_map = {
    "app_title": "RESEMBLE ENHANCE - ULTIMATE AUDIO SUITE",
    "status_proc": "PROCESSING...",
    "status_ready": "SYSTEM READY",
    "sec_1": "1. Input Audio Source",
    "browse": "BROWSE...",
    "tip_dd": "(Tip: You can drag and drop audio files here)",
    "sec_2": "2. AI Parameters & Configuration",
    "denoise": "Denoise Strength",
    "steps": "Solver Steps",
    "tau": "Temperature (tau)",
    "load": "LOAD AUDIO",
    "enhance": "ENHANCE AUDIO",
    "sec_3": "3. Audio Analysis & Output",
    "wave_orig": "Original Waveform",
    "wave_enh": "Enhanced Waveform",
    "save": "SAVE (.WAV)",
    "play_enh": "PLAY ENHANCED",
    "play_orig": "PLAY ORIGINAL",
    "stop": "STOP",
    "sec_4": "4. System Logs",
    "copy": "COPY LOGS",
    "log_loaded": "[System] Audio loaded.",
    "log_err_load": "[Error] Load audio first.",
    "log_saved": "[System] File saved.",
    "tab_single": "Single File",
    "tab_batch": "Batch Mode",
    "gpu_accel": "GPU Acceleration (DirectML)",
    "add_queue": "Add to Queue",
    "clear_queue": "Clear Queue",
    "start_batch": "Start Batch",
    "device_cpu": "Device: CPU",
    "device_gpu": "Device: GPU",
    "sec_batch": "Batch List",
    "tab_live": "Live Monitor",
    "start_mon": "Start Monitoring",
    "stop_mon": "Stop Monitoring",
    "mon_desc": "AI Enhance mic input in real-time (Low Latency Mode)",
    "setup_btn": "Setup AI Models",
    "fmt_wav": "WAV Format",
    "fmt_mp3": "MP3 Format"
}

output = []
output.append("#pragma once")
output.append("#include <string>")
output.append("#include <windows.h>")
output.append("#include <vector>")
output.append("")
output.append("enum class Language { EN, JP };")
output.append("")
output.append("struct UIStrings {")
output.append("    static std::string toUTF8(const std::wstring& wstr) {")
output.append("        if (wstr.empty()) return \"\";")
output.append("        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);")
output.append("        std::vector<char> buffer(size);")
output.append("        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buffer.data(), size, NULL, NULL);")
output.append("        return std::string(buffer.data());")
output.append("    }")
output.append("")
output.append("    static std::string get(const std::string& id, Language lang) {")
output.append("        bool is_jp = (lang == Language::JP);")
output.append("")

for key, val in data.items():
    en_val = en_map.get(key, val)
    jp_s = safe_str(val, True)
    en_s = safe_str(en_val, False)
    output.append(f"        if (id == \"{key}\") return is_jp ? {jp_s} : {en_s};")

output.append("        if (id == \"lang_btn\") return is_jp ? \"English\" : " + safe_str("日本語", True) + ";")
output.append("")
output.append("        return \"???\";")
output.append("    }")
output.append("};")

with open(sys.argv[1], "w", encoding="utf-8") as f:
    f.write("\n".join(output))

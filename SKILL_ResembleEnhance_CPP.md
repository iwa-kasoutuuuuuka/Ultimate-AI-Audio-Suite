# 🛠️ Skill: Resemble Enhance C++ Porting Mission

## 🎯 Mission Statement
Python環境への依存を完全に排除し、C++によるネイティブ実装へと移行することで、**「起動0秒、推論速度2倍、配布サイズ90%削減」**を実現する超高性能音声補正ツールを構築する。

---

## 🛠️ Technology Stack

| Category | Technology | Reason |
| :--- | :--- | :--- |
| **Language** | C++20 (MSVC) | 最新の標準ライブラリと高い実行性能。 |
| **AI Inference** | **ONNX Runtime (C++ API)** | PyTorch不要。CUDA/DirectMLによる高速推論と小サイズ化。 |
| **Audio/Video** | **FFmpeg SDK (libav*)** | 業界標準のコーデック対応と高度なリサンプリング。 |
| **GUI** | **Qt 6 (QML)** または **ImGui** | 高いデザイン性とネイティブの高速レスポンス。 |
| **Build System** | **CMake** | マルチプラットフォーム展開と依存関係管理。 |
| **Package Mgr** | **vcpkg** | C++ライブラリの複雑な導入を自動化。 |

---

## 📐 Architecture

### 1. Core Logic (Native Library)
- **Model Runner**: ONNX Runtime をラップし、Denoiser/Enhancer の推論を管理。
- **Audio Pipeline**: FFmpeg を使用したデコード -> 浮動小数点処理 -> エンコード。
- **DSP Module**: STFT (短時間フーリエ変換) やメルスペクトログラム変換のネイティブ実装。

### 2. Frontend (UI Layer)
- Python 版のデザインを継承し、さらに滑らかなアニメーションとリアルタイム・スペクトログラム表示を統合。

---

## 📅 Implementation Roadmap


---

## ⚠️ Key Challenges
- **STFT の精度**: Python (Torchaudio) と C++ で数学的に同一の結果を出すための厳密な実装が必要。
- **非同期処理**: AI推論中に UI をフリーズさせないための高度なマルチスレッド制御。

---
© 2026 Antigravity C++ Migration Team

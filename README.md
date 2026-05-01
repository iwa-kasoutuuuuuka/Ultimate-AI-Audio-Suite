# Resemble Enhance Native - Ultimate Audio Suite

![Project Icon](assets/app_icon.png)

## 💎 究極の AI 音声プロセッシング・スイート

Resemble Enhance を C++ でフルスクラッチ再構築し、プロフェッショナルな機能をすべて統合した究極のネイティブ・アプリケーションです。

### 🚀 主要機能 (Ultimate Features)

1.  **爆速 GPU 加速 (DirectML)**
    *   ONNX Runtime と DirectML を統合。NVIDIA, AMD, Intel のあらゆる GPU で AI 推論を高速化します。
2.  **一括バッチ処理 (Batch Mode)**
    *   複数のファイルをドラッグ＆ドロップでキューに追加し、一括で高音質化。大量のデータも自動で連続処理します。
3.  **リアルタイム・マイクモニター (Live Monitor)**
    *   マイク入力をリアルタイムで AI 処理。自分の声をその場で高音質化して確認できます（低遅延モード搭載）。
4.  **マルチフォーマット対応 (FFmpeg Integration)**
    *   FFmpeg バックエンドを内蔵。`.wav` だけでなく `.mp3`, `.flac`, `.m4a`, `.ogg` など、あらゆる音声形式を直接読み込み・出力可能です。
5.  **2D スペクトログラム解析**
    *   波形と周波数分布（ヒートマップ）をリアルタイム表示。音響的な変化を視覚的に把握できます。

### 🧠 モデル・アーキテクチャ (Core Technology)

本プロジェクトは **Resemble AI** のオープンソースモデルに基づいています。
*   **公式リポジトリ**: [resemble-ai/resemble-enhance](https://github.com/resemble-ai/resemble-enhance)
*   **コア技術**: 
    *   **LCFM**: Latent Conditional Flow Matching による高品質な成分復元。
    *   **UnivNet**: GAN ベースの高品質ボコーダー。
    *   **CleanUNet**: 複素ドメインでの高度なノイズ除去。

### 🛠️ 技術スタック
*   **Engine**: C++17
*   **Inference**: ONNX Runtime (DirectML) / LibTorch
*   **UI/UX**: ImGui / ImPlot (Professional Dashboard)
*   **Audio I/O**: FFmpeg (File) / miniaudio (Real-time)
*   **Build**: CMake / MSVC

### 📦 ドキュメント (Documentation)
*   📖 [取扱説明書 (MANUAL_JA.md)](MANUAL_JA.md) - 一般ユーザー向けガイド
*   🛠️ [技術仕様書 (SPECIFICATION_JA.md)](SPECIFICATION_JA.md) - モデルアーキテクチャと内部仕様
*   📝 [開発記録 (WORK_LOG.md)](WORK_LOG.md) - プロジェクトの進捗とデバッグ記録

### 🚀 使用方法
1.  `build_cpp/Release/ResembleDebugV60.exe` を実行します。
2.  「Settings」タブを開き、「Setup AI Models」をクリックしてモデルを自動インストールします。

---
Produced by Antigravity AI

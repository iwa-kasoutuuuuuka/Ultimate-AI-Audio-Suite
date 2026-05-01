# 🛠️ Ultimate AI Audio Suite - 技術仕様書 (Technical Specification)

## 1. プロジェクト概要
本プロジェクトは、Resemble AI が開発したオープンソースの音声高音質化モデル **Resemble Enhance** を、高性能な C++ 環境で動作するように移植・拡張したものです。
Python 依存を完全に排除し、ONNX Runtime と DirectML を活用することで、Windows 環境における圧倒的な処理速度と汎用的な GPU 加速を実現しています。

---

## 2. 公式リソース & ライセンス
*   **公式リポジトリ**: [resemble-ai/resemble-enhance](https://github.com/resemble-ai/resemble-enhance)
*   **開発者**: Resemble AI
*   **ライセンス**: Apache License 2.0 (Model) / MIT (C++ Wrapper)
*   **公式デモ**: [Hugging Face Space](https://huggingface.co/spaces/resembleai/resemble-enhance)

---

## 3. モデル・アーキテクチャ詳細
本アプリケーションは、以下の 5 つの主要な AI モジュールをパイプラインとして統合しています。

### 3.1 Denoiser (ノイズ除去)
*   **アーキテクチャ**: UNet ベースの複素数スペクトログラム推論。
*   **役割**: 入力音声から背景ノイズや歪みを分離・除去します。
*   **ONNX モデル**: `denoiser_unet.onnx`

### 3.2 Enhancer (LCFM: Latent Conditional Flow Matching)
*   **アーキテクチャ**: IRMAE (Inductive Regularized Multi-Layer Autoencoder) + CFM (Conditional Flow Matching)。
*   **役割**: 圧縮された音声や低ビットレートの音声から失われた高域成分を、潜在空間（Latent Space）上でマッチング・復元します。
*   **ONNX モデル**:
    *   `enhancer_encoder.onnx` (Encoder)
    *   `enhancer_decoder.onnx` (Decoder)
    *   `cfm_wn.onnx` (Flow Matching Network)

### 3.3 Vocoder (ボコーダー)
*   **アーキテクチャ**: UnivNet (Multi-Resolution Spectrogram Discriminator 搭載)。
*   **役割**: 処理されたメルスペクトログラムを高品質な時間軸の波形データ（PCM）に変換します。
*   **ONNX モデル**: `enhancer_vocoder.onnx` (または `enhancer_vocoder.pt`)

---

## 4. 技術仕様 (Software Stack)

### 4.1 推論エンジン
*   **Engine**: ONNX Runtime v1.17+
*   **Execution Provider**: 
    *   **DirectML**: Windows 環境のあらゆる GPU (NVIDIA, AMD, Intel) をサポート。
    *   **CPU**: Fallback オプション。

### 4.2 音声入出力 (Audio I/O)
*   **FFmpeg**: マルチフォーマットデコード/エンコード（WAV, MP3, FLAC, M4A 等）。
*   **miniaudio**: リアルタイムマイクモニタリングと低遅延再生。

### 4.3 UI ツールキット
*   **Dear ImGui**: 高性能なプロフェッショナル・ダッシュボード。
*   **ImPlot**: リアルタイム波形・スペクトログラム可視化。

---

## 5. 内部パラメータ (HParams)
推論時の整合性を保つための主要な設定値：
*   **Sample Rate**: 44,100 Hz
*   **Mel Bins**: 128
*   **Hop Size**: 512
*   **Window Size**: 2,048
*   **Latent Dimension**: 64 (IRMAE)

---

## 6. ビルド・配布仕様
*   **Compiler**: MSVC 2022 (C++17)
*   **Build System**: CMake 3.25+
*   **依存関係管理**: NuGet / vcpkg
*   **パッケージング**: ポータブル実行ファイル形式（DLL 同梱）

---
Produced by **Antigravity AI**

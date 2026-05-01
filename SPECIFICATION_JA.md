# 🛠️ Ultimate AI Audio Suite - 技術仕様書 / Technical Specification

## 1. プロジェクト概要 / Project Overview
Resemble Enhance を C++ に移植し、ONNX Runtime と DirectML を活用して高速化した音声処理スイートです。
A high-performance audio suite porting Resemble Enhance to C++, leveraging ONNX Runtime and DirectML.

---

## 2. 公式リソース / Official Resources
*   **Original Repo**: [resemble-ai/resemble-enhance](https://github.com/resemble-ai/resemble-enhance)
*   **License**: Apache 2.0 (Model) / MIT (C++ Wrapper)

---

## 3. アーキテクチャ / Architecture

### 3.1 Denoiser
*   **Model**: `denoiser_unet.onnx`
*   **Role**: ノイズ除去 / Noise reduction.

### 3.2 Enhancer (LCFM)
*   **Models**: `enhancer_encoder.onnx`, `enhancer_decoder.onnx`, `cfm_wn.onnx`
*   **Role**: 高域成分の復元 / High-frequency restoration.

### 3.3 Vocoder
*   **Model**: `enhancer_vocoder.onnx`
*   **Role**: 波形合成 / Waveform synthesis.

---

## 4. 技術仕様 / Tech Specs
*   **Engine**: ONNX Runtime v1.17+
*   **Acceleration**: DirectML (Universal GPU support)
*   **Audio I/O**: FFmpeg & miniaudio
*   **UI**: ImGui & ImPlot

---

## 5. パラメータ / Parameters
*   **Sample Rate**: 44,100 Hz
*   **Mel Bins**: 128
*   **Hop Size**: 512
*   **Window Size**: 2,048

---
Produced by **Antigravity AI**

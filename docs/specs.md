# Resemble Enhance Portable Tool Specifications

This document lists the files required for the application to function and their official download sources. The application will attempt to download these automatically at startup if they are missing.

## Required Binaries

| File | Purpose | Source URL |
|------|---------|------------|
| `bin/ffmpeg.exe` | Audio extraction and conversion | [FFmpeg Release Essentials](https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip) |
| `bin/ffprobe.exe` | Media file analysis | Included in FFmpeg zip |

## AI Model Weights (Resemble AI)

The models are hosted on Hugging Face. The application expects them in the `models/` subdirectory.

| Model Component | Target Path | Download URL |
|-----------------|-------------|--------------|
| Denoiser | `models/denoiser/model.pt` | [HuggingFace - Denoiser](https://huggingface.co/resemble-ai/resemble-enhance/resolve/main/denoiser/model.pt) |
| Enhancer Stage 1 | `models/enhancer_stage1/model.pt` | [HuggingFace - Stage 1](https://huggingface.co/resemble-ai/resemble-enhance/resolve/main/enhancer_stage1/model.pt) |
| Enhancer Stage 2 | `models/enhancer_stage2/model.pt` | [HuggingFace - Stage 2](https://huggingface.co/resemble-ai/resemble-enhance/resolve/main/enhancer_stage2/model.pt) |

## Application Structure

- `ResembleEnhance.exe` (Main application)
- `bin/` (FFmpeg binaries)
- `models/` (AI model weights)
- `specs.md` (English documentation)
- `仕様書.md` (Japanese documentation)

## Key Features

- **AI Audio Enhancement**: Denoising and high-fidelity speech enhancement (44.1kHz).
- **Preview Mode**: Process only the first 10 seconds of a file to test settings.
- **Batch Processing**: Recursively process entire directories while preserving the folder structure.
- **Flexible Output**: Choose between WAV (Lossless) or MP3 (320kbps) formats and customize file suffixes.
- **Hardware Control**: Manually switch between GPU (CUDA) and CPU processing.
- **Drag & Drop**: Intuitive interface for both single files and directories.
- **Real-time Logging**: Comprehensive status updates and error reporting in the built-in console.

## License
The Resemble Enhance models are distributed by Resemble AI. Please refer to their GitHub repository for licensing details: [resemble-ai/resemble-enhance](https://github.com/resemble-ai/resemble-enhance)

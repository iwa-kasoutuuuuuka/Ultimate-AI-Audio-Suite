#pragma once
#include <vector>
#include <complex>

/**
 * @brief Digital Signal Processing module for audio features.
 * Matches Torchaudio/Resemble-Enhance implementation.
 */
class DSPModule {
public:
    struct STFTConfig {
        int n_fft = 2048;
        int hop_length = 512;
        int win_length = 2048;
        bool center = true;
    };

    struct STFTData {
        std::vector<float> magnitude;
        std::vector<float> real;
        std::vector<float> imag;
        int n_bins;
        int n_frames;
    };

    static STFTData extractSTFT(const std::vector<float>& waveform, const STFTConfig& cfg);
    static std::vector<float> stftToMel(const STFTData& stft, int sample_rate, int n_mels, const STFTConfig& cfg);

    static std::vector<float> extractMel(
        const std::vector<float>& waveform,
        int sample_rate,
        int n_mels,
        const STFTConfig& cfg);

    static std::vector<float> istft(const std::vector<std::vector<std::complex<float>>>& stft_matrix, const STFTConfig& cfg);
    static std::vector<float> griffinLim(const std::vector<std::vector<float>>& mel_spec, int iterations, int sample_rate, const STFTConfig& cfg);

    /**
     * @brief Pre-emphasis filter: y[n] = x[n] - coeff * x[n-1]
     */
    static void preemphasis(std::vector<float>& waveform, float coeff = 0.97f);

private:
    static std::vector<float> createHannWindow(int window_size);
    static std::vector<float> createMelFilterbank(int sample_rate, int n_fft, int n_mels);
};

#include "DSPModule.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <complex>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    void fft(std::vector<std::complex<double>>& x) {
        size_t n = x.size();
        if (n <= 1) return;
        std::vector<std::complex<double>> even(n / 2), odd(n / 2);
        for (size_t i = 0; i < n / 2; ++i) {
            even[i] = x[2 * i];
            odd[i] = x[2 * i + 1];
        }
        fft(even); fft(odd);
        for (size_t k = 0; k < n / 2; ++k) {
            std::complex<double> t = std::polar(1.0, -2 * M_PI * k / n) * odd[k];
            x[k] = even[k] + t;
            x[k + n / 2] = even[k] - t;
        }
    }

    void ifft(std::vector<std::complex<double>>& x) {
        for (auto& c : x) c = std::conj(c);
        fft(x);
        for (auto& c : x) {
            c = std::conj(c);
            c /= static_cast<double>(x.size());
        }
    }

    float hzToMel(float hz) {
        if (hz < 1000.0f) return hz * 3.0f / 200.0f;
        return 15.0f + 27.0f * std::log(hz / 1000.0f) / std::log(6.4f);
    }

    float melToHz(float mel) {
        if (mel < 15.0f) return mel * 200.0f / 3.0f;
        return 1000.0f * std::pow(6.4f, (mel - 15.0f) / 27.0f);
    }
}

void DSPModule::preemphasis(std::vector<float>& waveform, float coeff) {
    if (waveform.empty() || coeff <= 0.0f) return;
    for (size_t i = waveform.size() - 1; i > 0; --i) waveform[i] -= coeff * waveform[i - 1];
    waveform[0] *= (1.0f - coeff);
}

std::vector<float> DSPModule::createHannWindow(int window_size) {
    std::vector<float> window(window_size);
    for (int i = 0; i < window_size; ++i) window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / window_size));
    return window;
}

std::vector<float> DSPModule::createMelFilterbank(int sample_rate, int n_fft, int n_mels) {
    int n_freq = n_fft / 2 + 1;
    std::vector<float> filterbank(n_mels * n_freq, 0.0f);
    float mel_min = hzToMel(0.0f), mel_max = hzToMel(static_cast<float>(sample_rate) / 2.0f);
    std::vector<float> hz_points(n_mels + 2);
    for (int i = 0; i < n_mels + 2; ++i) {
        float m = mel_min + i * (mel_max - mel_min) / (n_mels + 1);
        hz_points[i] = melToHz(m);
    }
    std::vector<int> bin_points(n_mels + 2);
    for (int i = 0; i < n_mels + 2; ++i) bin_points[i] = std::floor((n_fft + 1) * hz_points[i] / sample_rate);
    for (int i = 0; i < n_mels; ++i) {
        for (int j = bin_points[i]; j < bin_points[i + 1]; ++j)
            filterbank[i * n_freq + j] = (j - bin_points[i]) / static_cast<float>(bin_points[i + 1] - bin_points[i]);
        for (int j = bin_points[i + 1]; j < bin_points[i + 2]; ++j)
            filterbank[i * n_freq + j] = (bin_points[i + 2] - j) / static_cast<float>(bin_points[i + 2] - bin_points[i + 1]);
    }
    for (int i = 0; i < n_mels; ++i) {
        float enorm = 2.0f / (hz_points[i + 2] - hz_points[i]);
        for (int j = 0; j < n_freq; ++j) filterbank[i * n_freq + j] *= enorm;
    }
    return filterbank;
}

DSPModule::STFTData DSPModule::extractSTFT(const std::vector<float>& waveform, const STFTConfig& cfg) {
    std::vector<float> padded_wav = waveform;
    int pad = cfg.n_fft / 2;
    padded_wav.insert(padded_wav.begin(), pad, 0.0f);
    padded_wav.insert(padded_wav.end(), pad, 0.0f);
    
    std::vector<float> window = createHannWindow(cfg.win_length);
    int n_freq = cfg.n_fft / 2 + 1;
    int n_frames = (int)((padded_wav.size() - cfg.n_fft) / cfg.hop_length + 1);
    
    STFTData data;
    data.n_bins = n_freq;
    data.n_frames = n_frames;
    data.magnitude.resize(n_freq * n_frames);
    data.real.resize(n_freq * n_frames);
    data.imag.resize(n_freq * n_frames);

    for (int f = 0; f < n_frames; ++f) {
        int start = f * cfg.hop_length;
        std::vector<std::complex<double>> frame(cfg.n_fft, 0.0);
        int copy_len = std::min(cfg.win_length, cfg.n_fft);
        for (int i = 0; i < copy_len; ++i) {
            frame[i] = padded_wav[start + i] * window[i];
        }
        fft(frame);
        
        for (int i = 0; i < n_freq; ++i) {
            data.magnitude[i * n_frames + f] = (float)std::abs(frame[i]);
            data.real[i * n_frames + f] = (float)frame[i].real();
            data.imag[i * n_frames + f] = (float)frame[i].imag();
        }
    }
    return data;
}

std::vector<float> DSPModule::stftToMel(const STFTData& stft, int sample_rate, int n_mels, const STFTConfig& cfg) {
    std::vector<float> filterbank = createMelFilterbank(sample_rate, cfg.n_fft, n_mels);
    int n_freq = stft.n_bins;
    int n_frames = stft.n_frames;
    std::vector<float> mel_spectrogram(n_mels * n_frames, 0.0f);

    for (int f = 0; f < n_frames; ++f) {
        for (int m = 0; m < n_mels; ++m) {
            float mel_val = 0.0f;
            for (int i = 0; i < n_freq; ++i) {
                mel_val += stft.magnitude[i * n_frames + f] * filterbank[m * n_freq + i];
            }
            // Torchaudio / Resemble-Enhance normalization
            float val = std::log10(std::max(mel_val, 1e-4f)) * 20.0f;
            mel_spectrogram[m * n_frames + f] = (val + 80.0f) / (80.0f + 15.0f);
        }
    }
    return mel_spectrogram;
}

std::vector<float> DSPModule::extractMel(const std::vector<float>& waveform, int sample_rate, int n_mels, const STFTConfig& cfg) {
    STFTData stft = extractSTFT(waveform, cfg);
    return stftToMel(stft, sample_rate, n_mels, cfg);
}

std::vector<float> DSPModule::istft(const std::vector<std::vector<std::complex<float>>>& stft_matrix, const STFTConfig& cfg) {
    int n_frames = static_cast<int>(stft_matrix.size());
    if (n_frames == 0) return {};
    int expected_len = (n_frames - 1) * cfg.hop_length + cfg.n_fft;
    std::vector<float> output(expected_len, 0.0f), window_sum(expected_len, 0.0f);
    std::vector<float> window = createHannWindow(cfg.win_length);
    for (int f = 0; f < n_frames; ++f) {
        std::vector<std::complex<double>> frame(cfg.n_fft, 0.0);
        for (int i = 0; i < cfg.n_fft / 2 + 1; ++i) {
            frame[i] = stft_matrix[f][i];
            if (i > 0 && i < cfg.n_fft / 2) frame[cfg.n_fft - i] = std::conj(frame[i]);
        }
        ifft(frame);
        int start = f * cfg.hop_length;
        int copy_len = std::min(cfg.win_length, cfg.n_fft);
        for (int i = 0; i < copy_len && (start + i) < expected_len; ++i) {
            output[start + i] += static_cast<float>(frame[i].real()) * window[i];
            window_sum[start + i] += window[i] * window[i];
        }
    }
    for (size_t i = 0; i < output.size(); ++i) if (window_sum[i] > 1e-6f) output[i] /= window_sum[i];
    if (cfg.center) {
        int pad = cfg.n_fft / 2;
        if (output.size() > 2 * pad) return std::vector<float>(output.begin() + pad, output.end() - pad);
    }
    return output;
}

std::vector<float> DSPModule::griffinLim(const std::vector<std::vector<float>>& mel_spec, int iterations, int sample_rate, const STFTConfig& cfg) {
    int n_mels = static_cast<int>(mel_spec.size()), n_frames = static_cast<int>(mel_spec[0].size()), n_freq = cfg.n_fft / 2 + 1;
    std::vector<float> filterbank = createMelFilterbank(sample_rate, cfg.n_fft, n_mels);
    std::vector<std::vector<float>> mag_spec(n_frames, std::vector<float>(n_freq, 0.0f));
    for (int f = 0; f < n_frames; ++f) {
        for (int i = 0; i < n_freq; ++i) {
            float val = 0.0f;
            for (int m = 0; m < n_mels; ++m) {
                float db = mel_spec[m][f] * (80.0f + 15.0f) - 80.0f;
                val += std::pow(10.0f, db / 20.0f) * filterbank[m * n_freq + i];
            }
            mag_spec[f][i] = val;
        }
    }
    std::vector<std::vector<std::complex<float>>> stft_matrix(n_frames, std::vector<std::complex<float>>(n_freq));
    for (int f = 0; f < n_frames; ++f) {
        for (int i = 0; i < n_freq; ++i) {
            float angle = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * M_PI;
            stft_matrix[f][i] = std::polar(mag_spec[f][i], angle);
        }
    }
    std::vector<float> signal;
    for (int it = 0; it < iterations; ++it) {
        signal = istft(stft_matrix, cfg);
        std::vector<float> window = createHannWindow(cfg.win_length), padded_wav = signal;
        if (cfg.center) {
            int pad = cfg.n_fft / 2;
            padded_wav.insert(padded_wav.begin(), pad, 0.0f);
            padded_wav.insert(padded_wav.end(), pad, 0.0f);
        }
        for (int f = 0; f < n_frames; ++f) {
            int start = f * cfg.hop_length;
            std::vector<std::complex<double>> frame(cfg.n_fft, 0.0);
            for (int i = 0; i < cfg.win_length && (start + i) < padded_wav.size(); ++i) frame[i] = padded_wav[start + i] * window[i];
            fft(frame);
            for (int i = 0; i < n_freq; ++i) stft_matrix[f][i] = std::polar(mag_spec[f][i], static_cast<float>(std::arg(frame[i])));
        }
    }
    return signal;
}

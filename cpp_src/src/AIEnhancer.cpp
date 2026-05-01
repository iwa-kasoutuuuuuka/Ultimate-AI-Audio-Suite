#include "AIEnhancer.hpp"
#include "InferenceEngine.hpp"
#include "DSPModule.hpp"
#include <torch/torch.h>
#include <torch/script.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

struct AIEnhancer::Impl {
    struct HParams {
        int wav_rate = 44100;
        int hop_length = 420;
        int win_length = 2048;
        int n_fft = 2048;
        int num_mels = 128;
        int latent_dim = 64;
        float z_scale = 6.0f;
        float stft_magnitude_min = 0.0001f;
    } hp;

    InferenceEngine denoiser_unet;
    InferenceEngine enhancer_encoder;
    InferenceEngine enhancer_decoder;
    InferenceEngine cfm_wn;
    torch::jit::script::Module vocoder;
    bool vocoder_loaded = false;
    
    // Log callback for GUI integration
    std::function<void(const std::string&)> log_fn;
    
    int n_steps = 32;
    float n_tau = 0.5f;

    void log(const std::string& msg) {
        std::cout << msg << std::endl;
        if (log_fn) log_fn(msg);
    }
    void logErr(const std::string& msg) {
        std::cerr << msg << std::endl;
        if (log_fn) log_fn("[Error] " + msg);
    }
};

AIEnhancer::AIEnhancer() : pimpl(std::make_unique<Impl>()) {}
AIEnhancer::~AIEnhancer() = default;

void AIEnhancer::setLogCallback(std::function<void(const std::string&)> cb) {
    pimpl->log_fn = std::move(cb);
}

void AIEnhancer::setParameters(int solver_steps, float n_tau) {
    pimpl->n_steps = solver_steps;
    pimpl->n_tau = n_tau;
}

bool AIEnhancer::loadModels(const std::string& models_dir, bool use_gpu) {
    try {
        // Parse hparams.txt (key: value format, NOT JSON)
        std::string hp_path = models_dir + "/hparams.txt";
        if (std::filesystem::exists(hp_path)) {
            std::ifstream f(hp_path);
            std::string line;
            while (std::getline(f, line)) {
                // Remove \r if present
                if (!line.empty() && line.back() == '\r') line.pop_back();
                auto colon = line.find(':');
                if (colon == std::string::npos) continue;
                std::string key = line.substr(0, colon);
                std::string val = line.substr(colon + 1);
                // Trim whitespace
                while (!key.empty() && key.back() == ' ') key.pop_back();
                while (!val.empty() && val.front() == ' ') val.erase(val.begin());
                if (val.empty()) continue;
                
                if (key == "wav_rate") pimpl->hp.wav_rate = std::stoi(val);
                else if (key == "num_mels") pimpl->hp.num_mels = std::stoi(val);
                else if (key == "hop_size") pimpl->hp.hop_length = std::stoi(val);
                else if (key == "win_size") pimpl->hp.win_length = std::stoi(val);
                else if (key == "n_fft") pimpl->hp.n_fft = std::stoi(val);
                else if (key == "latent_dim") pimpl->hp.latent_dim = std::stoi(val);
                else if (key == "z_scale") pimpl->hp.z_scale = std::stof(val);
                else if (key == "stft_magnitude_min") pimpl->hp.stft_magnitude_min = std::stof(val);
            }
            pimpl->log("HParams loaded: rate=" + std::to_string(pimpl->hp.wav_rate)
                + " mels=" + std::to_string(pimpl->hp.num_mels)
                + " hop=" + std::to_string(pimpl->hp.hop_length)
                + " nfft=" + std::to_string(pimpl->hp.n_fft));
        } else {
            pimpl->logErr("hparams.txt not found at: " + hp_path);
        }

        bool ok = true;
        auto load = [&](const std::string& name, InferenceEngine& engine, const std::string& path) -> bool {
            if (!std::filesystem::exists(path)) {
                pimpl->logErr(name + " model not found: " + path);
                return false;
            }
            bool res = engine.loadModel(path, use_gpu);
            if (res) pimpl->log("Loaded " + name + " OK");
            else pimpl->logErr("Failed to load " + name + " from " + path);
            return res;
        };

        ok &= load("Denoiser", pimpl->denoiser_unet, models_dir + "/denoiser_unet.onnx");
        ok &= load("Encoder", pimpl->enhancer_encoder, models_dir + "/enhancer_encoder.onnx");
        ok &= load("Decoder", pimpl->enhancer_decoder, models_dir + "/enhancer_decoder.onnx");
        ok &= load("CFM", pimpl->cfm_wn, models_dir + "/cfm_wn.onnx");

        std::string vocoder_path = models_dir + "/enhancer_vocoder.pt";
        if (std::filesystem::exists(vocoder_path)) {
            pimpl->log("Loading vocoder from " + vocoder_path + " (this may take a moment)...");
            pimpl->vocoder = torch::jit::load(vocoder_path);
            if (use_gpu) {
                try {
                    if (torch::cuda::is_available()) {
                        pimpl->vocoder.to(torch::kCUDA);
                        pimpl->log("[System] Vocoder moved to CUDA GPU.");
                    } else {
                        pimpl->log("[System] CUDA not available for Vocoder, staying on CPU.");
                    }
                } catch (...) {
                    pimpl->log("[System] Failed to move Vocoder to GPU, staying on CPU.");
                }
            }
            pimpl->vocoder.eval();
            pimpl->vocoder_loaded = true;
            pimpl->log("Vocoder loaded OK");
        } else {
            pimpl->logErr("Vocoder not found at: " + vocoder_path);
        }

        if (!ok) pimpl->logErr("One or more ONNX models failed to load.");
        if (!pimpl->vocoder_loaded) pimpl->logErr("Vocoder (LibTorch) failed to load.");

        return ok && pimpl->vocoder_loaded;
    } catch (const std::exception& e) {
        pimpl->logErr("Exception in loadModels: " + std::string(e.what()));
        return false;
    }
}

bool AIEnhancer::process(const std::vector<float>& input_waveform, float denoise_strength, std::vector<float>& output_waveform, std::function<void(float)> progress_cb) {
    auto report = [&](float p) { if (progress_cb) progress_cb(p); };
    try {
        if (input_waveform.empty()) {
            pimpl->logErr("Input waveform is empty.");
            return false;
        }

        report(0.05f);
        pimpl->log("[AI] Step 1: Preemphasis & STFT Extraction...");
        
        // Apply preemphasis
        std::vector<float> processed_wav = input_waveform;
        DSPModule::preemphasis(processed_wav, 0.97f);

        DSPModule::STFTConfig stft_cfg = { pimpl->hp.n_fft, pimpl->hp.hop_length, pimpl->hp.win_length };
        DSPModule::STFTData stft = DSPModule::extractSTFT(processed_wav, stft_cfg);
        
        int n_frames = stft.n_frames;
        int n_bins = stft.n_bins; // 1025
        if (n_frames <= 0) {
            pimpl->logErr("Failed to extract STFT.");
            return false;
        }

        pimpl->log("[AI] Step 2: Denoising (UNet)...");
        std::vector<float> combined_mel;
        
        if (pimpl->denoiser_unet.isLoaded()) {
            pimpl->log("[AI] Preparing UNet input: " + std::to_string(model_channels) + "ch x " + std::to_string(n_bins) + "bins x " + std::to_string(n_frames) + "frames");
            
            if (n_bins != 1025) {
                pimpl->logErr("STFT bins mismatch! Expected 1025 for Denoiser, but got " + std::to_string(n_bins));
                combined_mel = DSPModule::stftToMel(stft, pimpl->hp.wav_rate, pimpl->hp.num_mels, stft_cfg);
            } else {
                std::vector<float> denoiser_input(model_channels * n_bins * n_frames);
                for (int i = 0; i < n_bins * n_frames; ++i) {
                    denoiser_input[i] = stft.magnitude[i];
                    denoiser_input[n_bins * n_frames + i] = stft.real[i];
                    denoiser_input[2 * n_bins * n_frames + i] = stft.imag[i];
                }

                std::map<std::string, std::vector<float>> d_inputs = { {"input", denoiser_input} };
                std::map<std::string, std::vector<int64_t>> d_shapes = { {"input", {1, (int64_t)model_channels, (int64_t)n_bins, (int64_t)n_frames}} };
                std::map<std::string, std::vector<float>> d_outputs;
                
                if (pimpl->denoiser_unet.run(d_inputs, d_shapes, {"output"}, d_outputs)) {
                    const auto& denoised_raw = d_outputs.at("output");
                    pimpl->log("[AI] Denoiser output received: " + std::to_string(denoised_raw.size()) + " samples.");
                    
                    DSPModule::STFTData clean_stft = stft;
                    int copy_count = std::min((int)denoised_raw.size(), n_bins * n_frames);
                    for (int i = 0; i < copy_count; ++i) {
                        float denoised_mag = denoised_raw[i];
                        clean_stft.magnitude[i] = stft.magnitude[i] * (1.0f - denoise_strength) + denoised_mag * denoise_strength;
                    }
                    combined_mel = DSPModule::stftToMel(clean_stft, pimpl->hp.wav_rate, pimpl->hp.num_mels, stft_cfg);
                    pimpl->log("[AI] Denoising applied successfully.");
                } else {
                    pimpl->log("[AI] Denoiser model execution failed.");
                    combined_mel = DSPModule::stftToMel(stft, pimpl->hp.wav_rate, pimpl->hp.num_mels, stft_cfg);
                }
            }
        } else {
            pimpl->log("[AI] Denoiser model not loaded. Skipping.");
            combined_mel = DSPModule::stftToMel(stft, pimpl->hp.wav_rate, pimpl->hp.num_mels, stft_cfg);
        }
        report(0.2f);

        pimpl->log("[AI] Step 3: Encoding & Scaling...");
        std::map<std::string, std::vector<float>> encoder_inputs = { {"mel", combined_mel} };
        std::map<std::string, std::vector<int64_t>> encoder_shapes = { {"mel", {1, pimpl->hp.num_mels, n_frames}} };
        std::map<std::string, std::vector<float>> encoder_outputs;
        if (!pimpl->enhancer_encoder.run(encoder_inputs, encoder_shapes, {"latent"}, encoder_outputs)) {
            pimpl->logErr("Encoder failed. Check if model matches num_mels=" + std::to_string(pimpl->hp.num_mels));
            return false;
        }
        report(0.3f);
        
        // Scale latent and inject prior noise (tau from pimpl->n_tau)
        std::vector<float> psi_t = encoder_outputs.at("latent");
        float z_scale = pimpl->hp.z_scale;
        float tau = pimpl->n_tau;
        
        for (float& val : psi_t) {
            val *= z_scale; // Scale to model range
            float noise = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 1.5f; // Rough Gaussian approx
            val = tau * noise + (1.0f - tau) * val;
        }

        int latent_dim = (int)psi_t.size() / n_frames;
        pimpl->log("[AI] Step 4: CFM Solver (NFE=" + std::to_string(pimpl->n_steps) + ", LatentDim=" + std::to_string(latent_dim) + ")...");
        
        auto get_time_emb = [](float t, int dim) {
            std::vector<float> emb(dim);
            for (int i = 0; i < dim / 2; ++i) {
                float p = (float)i * 4.0f / (float)(dim / 2 - 1);
                float freq = std::pow(10.0f, p);
                emb[i] = std::sin(t * freq);
                emb[i + dim / 2] = std::cos(t * freq);
            }
            return emb;
        };

        int n_steps = pimpl->n_steps;
        float dt = 1.0f / n_steps;
        for (int i = 0; i < n_steps; ++i) {
            float t = (float)i * dt;
            auto t_emb = get_time_emb(t, 128);
            
            std::map<std::string, std::vector<float>> inputs_k1 = { 
                {"psi_t", psi_t}, {"cond", combined_mel}, {"time_emb", t_emb} 
            };
            std::map<std::string, std::vector<int64_t>> shapes_k1 = { 
                {"psi_t", {1, (int64_t)latent_dim, (int64_t)n_frames}}, {"cond", {1, (int64_t)pimpl->hp.num_mels, (int64_t)n_frames}}, {"time_emb", {1, 128}} 
            };
            std::map<std::string, std::vector<float>> outputs_k1;
            if (!pimpl->cfm_wn.run(inputs_k1, shapes_k1, {"velocity"}, outputs_k1)) {
                pimpl->logErr("CFM Solver failed at k1, step " + std::to_string(i));
                return false;
            }
            const auto& v1 = outputs_k1.at("velocity");
            
            std::vector<float> psi_mid(psi_t.size());
            for(size_t j=0; j<psi_t.size(); ++j) psi_mid[j] = psi_t[j] + v1[j] * (dt * 0.5f);
            
            auto t_emb_mid = get_time_emb(t + dt * 0.5f, 128);
            std::map<std::string, std::vector<float>> inputs_k2 = { 
                {"psi_t", psi_mid}, {"cond", combined_mel}, {"time_emb", t_emb_mid} 
            };
            std::map<std::string, std::vector<float>> outputs_k2;
            if (!pimpl->cfm_wn.run(inputs_k2, shapes_k1, {"velocity"}, outputs_k2)) {
                pimpl->logErr("CFM Solver failed at k2, step " + std::to_string(i));
                return false;
            }
            const auto& v2 = outputs_k2.at("velocity");
            
            for(size_t j=0; j<psi_t.size(); ++j) psi_t[j] += v2[j] * dt;
            if (i % 8 == 0) report(0.3f + 0.4f * (float)(i + 1) / n_steps);
        }

        pimpl->log("[AI] Step 5: Unscaling & Decoding...");
        for (float& val : psi_t) val /= z_scale;

        std::map<std::string, std::vector<float>> decoder_inputs = { {"latent", psi_t} };
        std::map<std::string, std::vector<int64_t>> decoder_shapes = { {"latent", {1, (int64_t)latent_dim, (int64_t)n_frames}} };
        std::map<std::string, std::vector<float>> decoder_outputs;
        if (!pimpl->enhancer_decoder.run(decoder_inputs, decoder_shapes, {"mel"}, decoder_outputs)) {
            pimpl->logErr("Decoder failed.");
            return false;
        }
        report(0.8f);
        const auto& enhanced_mel = decoder_outputs.at("mel");

        pimpl->log("[AI] Step 6: Neural Vocoder (Waveform Synthesis)...");
        int64_t out_channels = (int64_t)enhanced_mel.size() / n_frames;
        pimpl->log("[AI] Vocoder Input: " + std::to_string(out_channels) + " channels, " + std::to_string(n_frames) + " frames.");

        auto options = torch::TensorOptions().dtype(torch::kFloat32);
        torch::Tensor mel_tensor = torch::from_blob(const_cast<float*>(enhanced_mel.data()), {1, out_channels, (int64_t)n_frames}, options).clone();
        
        std::vector<torch::jit::IValue> torch_inputs;
        if (pimpl->vocoder_loaded) {
            torch::Device device = torch::kCPU;
            try {
                auto params = pimpl->vocoder.parameters();
                if (params.begin() != params.end()) device = (*params.begin()).device();
            } catch(...) {}
            pimpl->log("[AI] Vocoder running on device: " + (device.is_cuda() ? std::string("CUDA") : std::string("CPU")));
            torch_inputs.push_back(mel_tensor.to(device));
        } else {
            pimpl->logErr("Vocoder model not loaded.");
            return false;
        }
        
        at::Tensor audio_out = pimpl->vocoder.forward(torch_inputs).toTensor();
        audio_out = audio_out.view({-1}).cpu();
        
        float* audio_ptr = audio_out.data_ptr<float>();
        output_waveform.assign(audio_ptr, audio_ptr + audio_out.numel());

        report(1.0f);
        pimpl->log("[AI] Processing complete!");
        return true;
    } catch (const std::exception& e) {
        pimpl->logErr("CRITICAL AI ERROR: " + std::string(e.what()));
        return false;
    } catch (...) {
        pimpl->logErr("CRITICAL AI ERROR: Unknown exception.");
        return false;
    }
}

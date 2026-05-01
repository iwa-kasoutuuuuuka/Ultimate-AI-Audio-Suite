#pragma once
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <functional>

/**
 * @brief High-level class to manage the Resemble Enhance AI pipeline.
 */
class AIEnhancer {
public:
    AIEnhancer();
    ~AIEnhancer();

    /**
     * @brief Load all required ONNX models from a directory.
     * @param models_dir Path to directory containing .onnx files and hparams.txt.
     * @return true if successful.
     */
    bool loadModels(const std::string& models_dir, bool use_gpu = false);
    void setLogCallback(std::function<void(const std::string&)> cb);

    /**
     * @brief Process audio through the full pipeline.
     * @param input_waveform Mono samples at 44.1kHz.
     * @param denoise_strength Lambda parameter [0, 1].
     * @param out_waveform Output samples.
     * @return true if successful.
     */
    bool process(const std::vector<float>& input_waveform, float denoise_strength, std::vector<float>& out_waveform, std::function<void(float)> progress_cb = nullptr);
    
    void setParameters(int solver_steps, float n_tau);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

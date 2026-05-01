#pragma once
#include <vector>
#include <string>
#include <memory>
#include <map>

/**
 * @brief Generic wrapper for ONNX Runtime sessions.
 */
class InferenceEngine {
public:
    InferenceEngine();
    ~InferenceEngine();

    bool loadModel(const std::string& model_path, bool use_gpu = false);

    /**
     * @brief Run inference with multiple inputs and outputs.
     */
    bool run(const std::map<std::string, std::vector<float>>& inputs,
             const std::map<std::string, std::vector<int64_t>>& input_shapes,
             const std::vector<std::string>& output_names,
             std::map<std::string, std::vector<float>>& outputs);

    bool isLoaded() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

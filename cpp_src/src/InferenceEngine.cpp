#include "InferenceEngine.hpp"
#include <onnxruntime_cxx_api.h>
#include <dml_provider_factory.h>
#include <iostream>
#include <numeric>

// Global shared ONNX environment (must be singleton)
static Ort::Env& getOrtEnv() {
    static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ResembleEnhance");
    return env;
}

struct InferenceEngine::Impl {
    std::unique_ptr<Ort::Session> session;
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
};

InferenceEngine::InferenceEngine() : pimpl(std::make_unique<Impl>()) {}
InferenceEngine::~InferenceEngine() = default;

bool InferenceEngine::loadModel(const std::string& model_path, bool use_gpu) {
    try {
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
        
        if (use_gpu) {
            try {
                // Use DirectML (best compatibility for Windows)
                // Append DML EP
                OrtSessionOptionsAppendExecutionProvider_DML(session_options, 0);
                std::cout << "[Inference] GPU (DirectML) enabled for model: " << model_path << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[Inference] Warning: Failed to enable DirectML: " << e.what() << ". Falling back to CPU." << std::endl;
            } catch (...) {
                std::cerr << "[Inference] Warning: Failed to enable DirectML. Falling back to CPU." << std::endl;
            }
        }

#ifdef _WIN32
        std::wstring w_path(model_path.begin(), model_path.end());
        pimpl->session = std::make_unique<Ort::Session>(getOrtEnv(), w_path.c_str(), session_options);
#else
        pimpl->session = std::make_unique<Ort::Session>(getOrtEnv(), model_path.c_str(), session_options);
#endif
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load model: " << e.what() << std::endl;
        return false;
    }
}

bool InferenceEngine::run(const std::map<std::string, std::vector<float>>& inputs,
                          const std::map<std::string, std::vector<int64_t>>& input_shapes,
                          const std::vector<std::string>& output_names,
                          std::map<std::string, std::vector<float>>& outputs) {
    if (!pimpl->session) return false;

    try {
        std::vector<const char*> input_node_names;
        std::vector<Ort::Value> input_tensors;

        for (auto const& [name, data] : inputs) {
            input_node_names.push_back(name.c_str());
            auto const& shape = input_shapes.at(name);
            input_tensors.push_back(Ort::Value::CreateTensor<float>(
                pimpl->memory_info, const_cast<float*>(data.data()), data.size(), shape.data(), shape.size()));
        }

        std::vector<const char*> output_node_names;
        for (auto const& name : output_names) {
            output_node_names.push_back(name.c_str());
        }

        auto output_tensors = pimpl->session->Run(
            Ort::RunOptions{nullptr}, 
            input_node_names.data(), input_tensors.data(), input_tensors.size(), 
            output_node_names.data(), output_node_names.size());

        for (size_t i = 0; i < output_tensors.size(); ++i) {
            float* float_arr = output_tensors[i].GetTensorMutableData<float>();
            auto output_info = output_tensors[i].GetTensorTypeAndShapeInfo();
            size_t output_size = output_info.GetElementCount();
            
            std::vector<float> out_data(float_arr, float_arr + output_size);
            outputs[output_names[i]] = std::move(out_data);
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Inference failed: " << e.what() << std::endl;
        return false;
    }
}

bool InferenceEngine::isLoaded() const {
    return pimpl && pimpl->session != nullptr;
}

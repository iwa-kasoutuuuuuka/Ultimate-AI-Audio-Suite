#pragma once

#include <string>
#include <vector>
#include <memory>

/**
 * @brief AudioProcessor class for handling audio I/O using FFmpeg.
 */
class AudioProcessor {
public:
    AudioProcessor();
    ~AudioProcessor();

    /**
     * @brief Load audio file and convert to float PCM.
     * @param path Path to the input file.
     * @param target_sr Target sample rate (e.g., 44100).
     * @return Vector of float samples (mono).
     */
    bool loadAudio(const std::string& path, int target_sr, std::vector<float>& out_samples);

    /**
     * @brief Save float PCM to audio file.
     * @param path Path to the output file.
     * @param samples Vector of float samples.
     * @param sr Sample rate of the samples.
     * @param format Output format ("mp3" or "wav").
     * @return True if successful.
     */
    bool saveAudio(const std::string& path, const std::vector<float>& samples, int sr, const std::string& format);

private:
    // FFmpeg internal handles would go here in the implementation
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

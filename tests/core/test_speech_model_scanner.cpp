#include <gtest/gtest.h>

#include "core/speech/model_scanner.h"

#include <filesystem>
#include <fstream>

namespace {

std::filesystem::path makeTempDirectory(const std::string& name)
{
    const auto path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(path);
    std::filesystem::create_directories(path);
    return path;
}

}  // namespace

TEST(SpeechModelScannerTest, FiltersUnsupportedFiles)
{
    const auto directory = makeTempDirectory("links_speech_model_scanner_filters");
    std::ofstream(directory / "ggml-base.bin").put('\n');
    std::ofstream(directory / "ggml-small.en.bin").put('\n');
    std::ofstream(directory / "large-v3.gguf").put('\n');
    std::ofstream(directory / "readme.txt").put('\n');
    std::ofstream(directory / "model.bin").put('\n');

    const auto models = links::speech::ModelScanner::scanDirectory(directory.string());

    ASSERT_EQ(models.size(), 3u);
    EXPECT_EQ(models[0].displayName, "ggml-base.bin");
    EXPECT_EQ(models[1].displayName, "ggml-small.en.bin");
    EXPECT_EQ(models[2].displayName, "large-v3.gguf");
}

TEST(SpeechModelScannerTest, ReturnsEmptyForMissingDirectory)
{
    const auto models = links::speech::ModelScanner::scanDirectory("/path/that/does/not/exist");
    EXPECT_TRUE(models.empty());
}

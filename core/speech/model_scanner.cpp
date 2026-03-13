#include "model_scanner.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <utility>

namespace links::speech {
namespace {

std::string toLowerCopy(const std::string& value)
{
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return lower;
}

bool startsWith(const std::string& value, const std::string& prefix)
{
    return value.size() >= prefix.size()
        && std::equal(prefix.begin(), prefix.end(), value.begin());
}

}  // namespace

std::vector<ModelDescriptor> ModelScanner::scanDirectory(const std::string& directoryPath)
{
    std::vector<ModelDescriptor> models;
    if (directoryPath.empty()) {
        return models;
    }

    std::error_code ec;
    const std::filesystem::path directory(directoryPath);
    if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec)) {
        return models;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
        if (ec || !entry.is_regular_file(ec)) {
            continue;
        }

        const std::string fileName = entry.path().filename().string();
        if (!isSupportedModelFile(fileName)) {
            continue;
        }

        ModelDescriptor descriptor;
        descriptor.path = entry.path().string();
        descriptor.displayName = fileName;
        descriptor.format = entry.path().extension().string();
        if (!descriptor.format.empty() && descriptor.format.front() == '.') {
            descriptor.format.erase(descriptor.format.begin());
        }
        models.push_back(std::move(descriptor));
    }

    std::sort(models.begin(), models.end(), [](const ModelDescriptor& lhs, const ModelDescriptor& rhs) {
        return lhs.displayName < rhs.displayName;
    });
    return models;
}

bool ModelScanner::isSupportedModelFile(const std::string& fileName)
{
    const std::string lowerName = toLowerCopy(fileName);
    if (startsWith(lowerName, "ggml-") && lowerName.size() > 9) {
        return lowerName.rfind(".bin") == lowerName.size() - 4;
    }
    return lowerName.rfind(".gguf") == lowerName.size() - 5;
}

}  // namespace links::speech

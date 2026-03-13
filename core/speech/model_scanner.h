#ifndef LINKS_CORE_SPEECH_MODEL_SCANNER_H_
#define LINKS_CORE_SPEECH_MODEL_SCANNER_H_

#include "speech_types.h"

#include <string>
#include <vector>

namespace links::speech {

class ModelScanner {
public:
    static std::vector<ModelDescriptor> scanDirectory(const std::string& directoryPath);
    static bool isSupportedModelFile(const std::string& fileName);
};

}  // namespace links::speech

#endif  // LINKS_CORE_SPEECH_MODEL_SCANNER_H_

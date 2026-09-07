#include "participant_metadata_parser.h"

#include <nlohmann/json.hpp>

namespace links::conference {

bool parseIsHostFromParticipantMetadata(const std::string& metadataRaw)
{
    if (metadataRaw.empty()) {
        return false;
    }

    // Matches the previous QJsonDocument behaviour: malformed input, a
    // non-object document, a missing key, or a non-boolean value all yield
    // false (QJsonValue::toBool(false)).
    const auto doc = nlohmann::json::parse(metadataRaw, nullptr, /*allow_exceptions=*/false);
    if (doc.is_discarded() || !doc.is_object()) {
        return false;
    }

    const auto it = doc.find("isHost");
    if (it == doc.end() || !it->is_boolean()) {
        return false;
    }
    return it->get<bool>();
}

} // namespace links::conference

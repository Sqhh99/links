#include "participant_metadata_parser.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace links::conference {

bool parseIsHostFromParticipantMetadata(const std::string& metadataRaw)
{
    if (metadataRaw.empty()) {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument metadataDoc =
        QJsonDocument::fromJson(QByteArray::fromStdString(metadataRaw), &parseError);
    if (parseError.error != QJsonParseError::NoError || !metadataDoc.isObject()) {
        return false;
    }

    return metadataDoc.object().value(QStringLiteral("isHost")).toBool(false);
}

} // namespace links::conference

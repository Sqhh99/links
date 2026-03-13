#ifndef CORE_CONFERENCE_PARTICIPANT_METADATA_PARSER_H
#define CORE_CONFERENCE_PARTICIPANT_METADATA_PARSER_H

#include <string>

namespace links::conference {

bool parseIsHostFromParticipantMetadata(const std::string& metadataRaw);

} // namespace links::conference

#endif // CORE_CONFERENCE_PARTICIPANT_METADATA_PARSER_H

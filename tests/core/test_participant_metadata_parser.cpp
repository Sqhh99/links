#include <gtest/gtest.h>

#include <string>

#include "core/conference/participant_metadata_parser.h"

TEST(ParticipantMetadataParserTest, ReturnsFalseForEmptyMetadata)
{
    EXPECT_FALSE(links::conference::parseIsHostFromParticipantMetadata(std::string{}));
}

TEST(ParticipantMetadataParserTest, ReturnsFalseForInvalidJson)
{
    EXPECT_FALSE(links::conference::parseIsHostFromParticipantMetadata("{invalid json"));
}

TEST(ParticipantMetadataParserTest, ReturnsFalseWhenKeyMissing)
{
    EXPECT_FALSE(links::conference::parseIsHostFromParticipantMetadata(R"({"name":"alice"})"));
}

TEST(ParticipantMetadataParserTest, ParsesTrueAndFalseValues)
{
    EXPECT_TRUE(links::conference::parseIsHostFromParticipantMetadata(R"({"isHost":true})"));
    EXPECT_FALSE(links::conference::parseIsHostFromParticipantMetadata(R"({"isHost":false})"));
}

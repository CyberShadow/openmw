#ifndef OPENMW_ESM_DIALOGUESTATE_H
#define OPENMW_ESM_DIALOGUESTATE_H

#include <string>
#include <vector>
#include <map>
#include <unordered_set>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct DialogueState
    {
        // must be lower case topic IDs
        std::vector<std::string> mKnownTopics;

        std::unordered_set<std::string> mSeenResponses;

        // must be lower case faction IDs
        std::map<std::string, std::map<std::string, int> > mChangedFactionReaction;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif

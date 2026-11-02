#pragma once

#include "core/objects.h"
#include "engine/reader.h"

#include <string>
#include <vector>

namespace dx::extension {
    class UConAudioList : public UObject {
    public:
        void serialize(Reader& reader, Linker& source) override
        {
            int32_t count = reader.index();
            for (int32_t i = 0; i < count; i++) {
                sounds.push_back(source.readObject(reader));
            }
        }

        std::vector<UObject*> sounds;
    };

    class UConEventRandomLabel : public UObject {
    public:
        void serialize(Reader& reader, Linker&) override
        {
            int32_t count = reader.index();
            for (int32_t i = 0; i < count; i++) {
                labels.push_back(reader.string());
            }
        }

        std::vector<std::string> labels;
    };
}

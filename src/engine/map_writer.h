#pragma once

#include "core/package_writer.h"
#include "engine/resources.h"

#include <filesystem>
#include <string>

namespace dx {
    namespace mapwriter {
        void writePrimitive(Writer& writer, const UPrimitive& primitive);
        void writeModel(PackageWriter& package, const UModel& model, Writer& writer);
        void writePolys(PackageWriter& package, const UPolys& polys, Writer& writer);
        void writeLevel(PackageWriter& package, const ULevel& level, Writer& writer);
        bool saveMap(ULevel* level, const std::filesystem::path& file, std::string* error);
    }
}

#pragma once

#include "engine/resources.h"

#include <string>
#include <vector>

namespace dx {
    namespace brushbuilder {
        struct ExtraParameter {
            std::string name;
            double value = 0;
            bool integer = false;
            bool boolean = false;
        };

        struct ExtraBuilder {
            std::string name;
            std::vector<ExtraParameter> parameters;
        };

        std::vector<ExtraBuilder> extraBuilders();
        bool buildExtra(const ExtraBuilder& builder, UObject* brushActor, std::string* error);
    }
}

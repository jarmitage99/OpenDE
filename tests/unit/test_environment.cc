#include <cstdlib>
#include <filesystem>
#include <string>

#include <unistd.h>

namespace {
    struct IsolatedUserData {
        IsolatedUserData()
        {
            directory = std::filesystem::temp_directory_path() / ("deusex-unit-tests-" + std::to_string(getpid()));
            std::error_code error;
            std::filesystem::remove_all(directory, error);
            std::filesystem::create_directories(directory, error);
#ifdef _WIN32
            _putenv_s("XDG_DATA_HOME", directory.string().c_str());
#else
            setenv("XDG_DATA_HOME", directory.string().c_str(), 1);
#endif
        }

        ~IsolatedUserData()
        {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
        }

        std::filesystem::path directory;
    };

    const IsolatedUserData isolatedUserData;
}

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace dx::editor {
    class LogCapture {
    public:
        LogCapture() = default;
        ~LogCapture();
        LogCapture(const LogCapture&) = delete;
        LogCapture& operator=(const LogCapture&) = delete;

        bool start();
        void stop();
        void add(const std::string& line);
        std::vector<std::string> lines();
        void clear();
        int originalStderr() const;

    private:
        void pump();

        std::mutex mutex;
        std::thread reader;
        std::vector<std::string> captured;
        std::string partial;
        std::atomic<bool> stopping { false };
        int pipeRead = -1;
        int original = -1;
    };
}

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <algorithm>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <sstream>
#include <csignal>

namespace fs = std::filesystem;

constexpr long MY_SYS_pidfd_open = 434;
constexpr long MY_SYS_pidfd_getfd = 438;

bool debug_mode = false;

#define DEBUG(x) do { if (debug_mode) { std::cerr << x << std::endl; } } while(0)

int pidfd_open(pid_t pid, unsigned int flags) {
    int res = syscall(MY_SYS_pidfd_open, pid, flags);
    if (res == -1) DEBUG("[ERROR] pidfd_open(" << pid << ") failed: " << strerror(errno));
    else DEBUG("[DEBUG] pidfd_open(" << pid << ") = " << res);
    return res;
}

int pidfd_getfd(int pidfd, int targetfd, unsigned int flags) {
    int res = syscall(MY_SYS_pidfd_getfd, pidfd, targetfd, flags);
    if (res == -1) DEBUG("[ERROR] pidfd_getfd(pidfd=" << pidfd << ", fd=" << targetfd << ") failed: " << strerror(errno));
    else DEBUG("[DEBUG] pidfd_getfd(pidfd=" << pidfd << ", fd=" << targetfd << ") = " << res);
    return res;
}

std::vector<pid_t> list_pids() {
    std::vector<pid_t> pids;
    for (const auto& entry : fs::directory_iterator("/proc")) {
        if (entry.is_directory()) {
            std::string name = entry.path().filename();
            if (std::all_of(name.begin(), name.end(), ::isdigit)) {
                try {
                    pids.push_back(std::stoi(name));
                } catch (...) {}
            }
        }
    }
    DEBUG("[DEBUG] Found " << pids.size() << " PIDs");
    return pids;
}

std::vector<int> list_fds(pid_t pid) {
    std::vector<int> fds;
    std::string path = "/proc/" + std::to_string(pid) + "/fd";
    if (!fs::exists(path)) {
        DEBUG("[WARN] PID " << pid << " has no /fd path (process gone?)");
        return fds;
    }

    for (const auto& entry : fs::directory_iterator(path)) {
        std::string name = entry.path().filename();
        if (std::all_of(name.begin(), name.end(), ::isdigit)) {
            try {
                fds.push_back(std::stoi(name));
            } catch (...) {}
        }
    }
    DEBUG("[DEBUG] PID " << pid << " has " << fds.size() << " fds");
    return fds;
}

std::vector<char> random_data(std::mt19937& rng, size_t min_size, size_t max_size) {
    std::uniform_int_distribution<size_t> len_dist(min_size, max_size);
    std::uniform_int_distribution<> byte_dist(0, 255);
    size_t len = len_dist(rng);

    std::vector<char> data(len);
    for (size_t i = 0; i < len; ++i)
        data[i] = static_cast<char>(byte_dist(rng));

    DEBUG("[DEBUG] Generated " << len << " random bytes");
    return data;
}

void spam_process(pid_t pid, std::mt19937& rng, size_t min_size, size_t max_size) {
    DEBUG("[INFO] Spamming PID: " << pid);
    int pidfd = pidfd_open(pid, 0);
    if (pidfd < 0) return;

    auto fds = list_fds(pid);
    std::shuffle(fds.begin(), fds.end(), rng);

    for (int targetfd : fds) {
        int newfd = pidfd_getfd(pidfd, targetfd, 0);
        if (newfd < 0) continue;

        auto data = random_data(rng, min_size, max_size);
        ssize_t written = write(newfd, data.data(), data.size());

        if (written == -1)
            DEBUG("[ERROR] write(" << newfd << ") failed: " << strerror(errno));
        else
            DEBUG("[DEBUG] Wrote " << written << " bytes to fd=" << newfd);

        close(newfd);
    }

    close(pidfd);
}

std::vector<pid_t> parse_pid_list(const std::string& arg) {
    std::vector<pid_t> pids;
    std::stringstream ss(arg);
    std::string pidstr;
    while (std::getline(ss, pidstr, ',')) {
        try {
            pid_t pid = std::stoi(pidstr);
            pids.push_back(pid);
            DEBUG("[DEBUG] Parsed PID: " << pid);
        } catch (...) {
            std::cerr << "[ERROR] Invalid PID in list: " << pidstr << "\n";
        }
    }
    return pids;
}

void print_help(const char* progname) {
    std::cout << "Usage:\n"
              << "  " << progname << " <pid[,pid2,...]> [--min N] [--max N] [--debug] [--limit N] [--delay S]\n"
              << "  " << progname << " --lol [--min N] [--max N] [--debug] [--limit N] [--delay S]\n\n"
              << "Options:\n"
              << "  --lol         Target all PIDs on system\n"
              << "  --min <N>     Minimum size of random data (default: 1)\n"
              << "  --max <N>     Maximum size of random data (default: 1048576 = 1 MiB)\n"
              << "  --limit <N>   Limit number of iterations (default: unlimited)\n"
              << "  --delay <S>   Delay between iterations in seconds (default: 0)\n"
              << "  --debug       Enable debug output\n"
              << "  -h, --help    Show this help message\n";
}

int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE to avoid process kill on broken pipe

    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    bool all = false;
    std::vector<pid_t> targets;
    size_t min_size = 1;
    size_t max_size = 1048576;
    size_t limit_iterations = SIZE_MAX; // infinite by default
    double delay_seconds = 0.0;
    size_t iteration_count = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--lol") {
            all = true;
        } else if (arg == "--min" && i + 1 < argc) {
            min_size = std::stoul(argv[++i]);
            DEBUG("[DEBUG] Set min_size = " << min_size);
        } else if (arg == "--max" && i + 1 < argc) {
            max_size = std::stoul(argv[++i]);
            DEBUG("[DEBUG] Set max_size = " << max_size);
        } else if (arg == "--limit" && i + 1 < argc) {
            limit_iterations = std::stoul(argv[++i]);
            DEBUG("[DEBUG] Set iteration limit = " << limit_iterations);
        } else if (arg == "--delay" && i + 1 < argc) {
            delay_seconds = std::stod(argv[++i]);
            DEBUG("[DEBUG] Set delay = " << delay_seconds << " seconds");
        } else if (arg == "--debug") {
            debug_mode = true;
        } else if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (arg.rfind("--", 0) != 0) {
            targets = parse_pid_list(arg);
        } else {
            std::cerr << "[ERROR] Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    if (min_size > max_size) {
        std::cerr << "[ERROR] min_size > max_size\n";
        return 1;
    }

    std::random_device rd;
    std::mt19937 rng(rd());

    while (iteration_count < limit_iterations) {
        DEBUG("=== Iteration " << iteration_count + 1 << " ===");
        if (all) {
            auto pids = list_pids();
            for (pid_t pid : pids) {
                spam_process(pid, rng, min_size, max_size);
            }
        } else {
            for (pid_t pid : targets) {
                spam_process(pid, rng, min_size, max_size);
            }
        }

        ++iteration_count;
        if (delay_seconds > 0.0) {
            DEBUG("[DEBUG] Sleeping for " << delay_seconds << " seconds...");
            std::this_thread::sleep_for(std::chrono::duration<double>(delay_seconds));
        }
    }

    DEBUG("[INFO] Reached iteration limit, exiting.");
    return 0;
}

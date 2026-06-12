#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace linux_net_agent {

struct CommandRequest {
    std::string command;
    std::chrono::seconds timeout{30};
    bool interactive{false};
    std::optional<std::uint64_t> memory_limit_bytes;
    std::optional<std::uint64_t> cpu_seconds;
};

struct CommandResult {
    int exit_code{-1};
    bool timed_out{false};
    std::string stdout_data;
    std::string stderr_data;
    std::chrono::milliseconds duration{0};
};

struct ProcessInfo {
    int pid{0};
    int ppid{0};
    std::string state;
    std::string command;
};

struct HostStats {
    double load1{0};
    double load5{0};
    double load15{0};
    std::uint64_t mem_total_kb{0};
    std::uint64_t mem_available_kb{0};
    std::uint64_t uptime_seconds{0};
    std::string hostname;
    std::string kernel_release;
    std::string cgroup_path;
    std::vector<std::string> namespaces;
};

class ProcessManager {
public:
    CommandResult execute(const CommandRequest& request);
    std::vector<ProcessInfo> list_processes() const;

private:
    CommandResult execute_with_pipes(const CommandRequest& request);
    CommandResult execute_with_pty(const CommandRequest& request);
};

class HostInspector {
public:
    HostStats read_stats() const;
};

class AgentServer {
public:
    AgentServer(std::string host, std::uint16_t port);
    void run(std::atomic_bool& running);

private:
    std::string host_;
    std::uint16_t port_;
    ProcessManager process_manager_;
    HostInspector host_inspector_;
    std::mutex request_mutex_;

    int create_listener() const;
    void handle_client(int client_fd);
    std::string route(const std::string& method, const std::string& path, const std::string& body);
};

}  // namespace linux_net_agent

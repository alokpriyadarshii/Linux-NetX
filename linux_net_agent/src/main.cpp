#include "linux_net_agent/agent.hpp"

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pty.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace linux_net_agent {
namespace {

std::atomic_bool g_running{true};

void signal_handler(int) {
    g_running.store(false);
}

std::string json_escape(std::string_view input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char ch : input) {
        switch (ch) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    out += ' ';
                } else {
                    out += ch;
                }
        }
    }
    return out;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string read_link(const std::filesystem::path& path) {
    std::array<char, 512> buf{};
    const ssize_t len = ::readlink(path.c_str(), buf.data(), buf.size() - 1);
    if (len <= 0) {
        return {};
    }
    return std::string(buf.data(), static_cast<std::size_t>(len));
}

void set_nonblocking(int fd) {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("fcntl(F_GETFL) failed");
    }
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("fcntl(F_SETFL) failed");
    }
}

std::optional<std::string> json_string_field(const std::string& body, std::string_view name) {
    const std::string needle = "\"" + std::string(name) + "\"";
    std::size_t pos = body.find(needle);
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    pos = body.find(':', pos + needle.size());
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    pos = body.find('"', pos + 1);
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    std::string value;
    bool escaped = false;
    for (std::size_t i = pos + 1; i < body.size(); ++i) {
        const char ch = body[i];
        if (escaped) {
            if (ch == 'n') {
                value += '\n';
            } else if (ch == 't') {
                value += '\t';
            } else {
                value += ch;
            }
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            return value;
        }
        value += ch;
    }
    return std::nullopt;
}

std::optional<std::uint64_t> json_uint_field(const std::string& body, std::string_view name) {
    const std::string needle = "\"" + std::string(name) + "\"";
    std::size_t pos = body.find(needle);
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    pos = body.find(':', pos + needle.size());
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    ++pos;
    while (pos < body.size() && std::isspace(static_cast<unsigned char>(body[pos])) != 0) {
        ++pos;
    }
    std::size_t end = pos;
    while (end < body.size() && std::isdigit(static_cast<unsigned char>(body[end])) != 0) {
        ++end;
    }
    if (end == pos) {
        return std::nullopt;
    }
    return std::stoull(body.substr(pos, end - pos));
}

bool json_bool_field(const std::string& body, std::string_view name) {
    const std::string needle = "\"" + std::string(name) + "\"";
    std::size_t pos = body.find(needle);
    if (pos == std::string::npos) {
        return false;
    }
    pos = body.find(':', pos + needle.size());
    if (pos == std::string::npos) {
        return false;
    }
    ++pos;
    while (pos < body.size() && std::isspace(static_cast<unsigned char>(body[pos])) != 0) {
        ++pos;
    }
    return body.compare(pos, 4, "true") == 0;
}

void apply_limits(const CommandRequest& request) {
    if (request.memory_limit_bytes) {
        rlimit limit{};
        limit.rlim_cur = *request.memory_limit_bytes;
        limit.rlim_max = *request.memory_limit_bytes;
        ::setrlimit(RLIMIT_AS, &limit);
    }
    if (request.cpu_seconds) {
        rlimit limit{};
        limit.rlim_cur = *request.cpu_seconds;
        limit.rlim_max = *request.cpu_seconds + 1;
        ::setrlimit(RLIMIT_CPU, &limit);
    }
}

void append_fd(int fd, std::string& target) {
    std::array<char, 4096> buffer{};
    while (true) {
        const ssize_t n = ::read(fd, buffer.data(), buffer.size());
        if (n > 0) {
            target.append(buffer.data(), static_cast<std::size_t>(n));
            continue;
        }
        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }
        break;
    }
}

std::string command_result_json(const CommandResult& result) {
    std::ostringstream out;
    out << "{\"exit_code\":" << result.exit_code << ",\"timed_out\":"
        << (result.timed_out ? "true" : "false") << ",\"duration_ms\":"
        << result.duration.count() << ",\"stdout\":\"" << json_escape(result.stdout_data)
        << "\",\"stderr\":\"" << json_escape(result.stderr_data) << "\"}";
    return out.str();
}

std::string stats_json(const HostStats& stats) {
    std::ostringstream out;
    out << "{\"hostname\":\"" << json_escape(stats.hostname) << "\",\"kernel_release\":\""
        << json_escape(stats.kernel_release) << "\",\"load\":{\"one\":" << stats.load1
        << ",\"five\":" << stats.load5 << ",\"fifteen\":" << stats.load15
        << "},\"memory\":{\"total_kb\":" << stats.mem_total_kb
        << ",\"available_kb\":" << stats.mem_available_kb << "},\"uptime_seconds\":"
        << stats.uptime_seconds << ",\"cgroup\":\"" << json_escape(stats.cgroup_path)
        << "\",\"namespaces\":[";
    for (std::size_t i = 0; i < stats.namespaces.size(); ++i) {
        if (i != 0) {
            out << ',';
        }
        out << '"' << json_escape(stats.namespaces[i]) << '"';
    }
    out << "]}";
    return out.str();
}

}  // namespace

CommandResult ProcessManager::execute(const CommandRequest& request) {
    if (request.interactive) {
        return execute_with_pty(request);
    }
    return execute_with_pipes(request);
}

CommandResult ProcessManager::execute_with_pipes(const CommandRequest& request) {
    int out_pipe[2]{};
    int err_pipe[2]{};
    if (::pipe(out_pipe) == -1 || ::pipe(err_pipe) == -1) {
        throw std::runtime_error("pipe failed");
    }
    set_nonblocking(out_pipe[0]);
    set_nonblocking(err_pipe[0]);

    const auto start = std::chrono::steady_clock::now();
    const pid_t pid = ::fork();
    if (pid == -1) {
        throw std::runtime_error("fork failed");
    }

    if (pid == 0) {
        ::setsid();
        ::close(out_pipe[0]);
        ::close(err_pipe[0]);
        ::dup2(out_pipe[1], STDOUT_FILENO);
        ::dup2(err_pipe[1], STDERR_FILENO);
        ::close(out_pipe[1]);
        ::close(err_pipe[1]);
        apply_limits(request);
        ::execl("/bin/sh", "sh", "-lc", request.command.c_str(), nullptr);
        ::_exit(127);
    }

    ::close(out_pipe[1]);
    ::close(err_pipe[1]);

    CommandResult result;
    int status = 0;
    bool child_done = false;
    while (!child_done) {
        append_fd(out_pipe[0], result.stdout_data);
        append_fd(err_pipe[0], result.stderr_data);

        const pid_t wait_result = ::waitpid(pid, &status, WNOHANG);
        if (wait_result == pid) {
            child_done = true;
            break;
        }
        if (wait_result == -1) {
            child_done = true;
            break;
        }

        const auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed > request.timeout) {
            result.timed_out = true;
            ::kill(-pid, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            ::kill(-pid, SIGKILL);
            ::waitpid(pid, &status, 0);
            child_done = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    append_fd(out_pipe[0], result.stdout_data);
    append_fd(err_pipe[0], result.stderr_data);
    ::close(out_pipe[0]);
    ::close(err_pipe[0]);

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    }
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    );
    return result;
}

CommandResult ProcessManager::execute_with_pty(const CommandRequest& request) {
    int master = -1;
    int slave = -1;
    if (::openpty(&master, &slave, nullptr, nullptr, nullptr) == -1) {
        throw std::runtime_error("openpty failed");
    }
    set_nonblocking(master);

    const auto start = std::chrono::steady_clock::now();
    const pid_t pid = ::fork();
    if (pid == -1) {
        throw std::runtime_error("fork failed");
    }

    if (pid == 0) {
        ::setsid();
        ::close(master);
        ::dup2(slave, STDIN_FILENO);
        ::dup2(slave, STDOUT_FILENO);
        ::dup2(slave, STDERR_FILENO);
        ::close(slave);
        apply_limits(request);
        ::execl("/bin/sh", "sh", "-lc", request.command.c_str(), nullptr);
        ::_exit(127);
    }

    ::close(slave);

    CommandResult result;
    int status = 0;
    while (true) {
        append_fd(master, result.stdout_data);
        const pid_t wait_result = ::waitpid(pid, &status, WNOHANG);
        if (wait_result == pid || wait_result == -1) {
            break;
        }
        if (std::chrono::steady_clock::now() - start > request.timeout) {
            result.timed_out = true;
            ::kill(-pid, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            ::kill(-pid, SIGKILL);
            ::waitpid(pid, &status, 0);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    append_fd(master, result.stdout_data);
    ::close(master);

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    }
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    );
    return result;
}

std::vector<ProcessInfo> ProcessManager::list_processes() const {
    std::vector<ProcessInfo> processes;
    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
        if (!entry.is_directory()) {
            continue;
        }
        const std::string pid_text = entry.path().filename().string();
        if (pid_text.empty()
            || !std::all_of(pid_text.begin(), pid_text.end(), [](unsigned char ch) {
                return std::isdigit(ch) != 0;
            })) {
            continue;
        }

        const std::string stat = read_file(entry.path() / "stat");
        if (stat.empty()) {
            continue;
        }

        const std::size_t open = stat.find('(');
        const std::size_t close = stat.rfind(')');
        if (open == std::string::npos || close == std::string::npos || close + 4 >= stat.size()) {
            continue;
        }

        ProcessInfo info;
        info.pid = std::stoi(pid_text);
        info.command = stat.substr(open + 1, close - open - 1);
        info.state = stat.substr(close + 2, 1);
        std::istringstream rest(stat.substr(close + 4));
        rest >> info.ppid;
        processes.push_back(std::move(info));
        if (processes.size() >= 256) {
            break;
        }
    }
    return processes;
}

HostStats HostInspector::read_stats() const {
    HostStats stats;

    std::array<double, 3> loads{};
    if (::getloadavg(loads.data(), static_cast<int>(loads.size())) == 3) {
        stats.load1 = loads[0];
        stats.load5 = loads[1];
        stats.load15 = loads[2];
    }

    std::istringstream meminfo(read_file("/proc/meminfo"));
    std::string key;
    std::uint64_t value = 0;
    std::string unit;
    while (meminfo >> key >> value >> unit) {
        if (key == "MemTotal:") {
            stats.mem_total_kb = value;
        } else if (key == "MemAvailable:") {
            stats.mem_available_kb = value;
        }
    }

    std::istringstream uptime(read_file("/proc/uptime"));
    double uptime_seconds = 0;
    uptime >> uptime_seconds;
    stats.uptime_seconds = static_cast<std::uint64_t>(uptime_seconds);

    std::array<char, 256> hostname{};
    if (::gethostname(hostname.data(), hostname.size() - 1) == 0) {
        stats.hostname = hostname.data();
    }

    utsname uname_buf{};
    if (::uname(&uname_buf) == 0) {
        stats.kernel_release = uname_buf.release;
    }

    stats.cgroup_path = read_file("/proc/self/cgroup");
    for (const std::string& ns : {"mnt", "pid", "net", "uts", "ipc", "user", "cgroup"}) {
        const std::string target = read_link(std::filesystem::path("/proc/self/ns") / ns);
        if (!target.empty()) {
            stats.namespaces.push_back(ns + ":" + target);
        }
    }
    return stats;
}

AgentServer::AgentServer(std::string host, std::uint16_t port)
    : host_(std::move(host)), port_(port) {}

int AgentServer::create_listener() const {
    const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd == -1) {
        throw std::runtime_error("socket failed");
    }

    int yes = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port_);
    if (::inet_pton(AF_INET, host_.c_str(), &address.sin_addr) != 1) {
        ::close(fd);
        throw std::runtime_error("invalid listen address");
    }
    if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
        ::close(fd);
        throw std::runtime_error("bind failed");
    }
    if (::listen(fd, SOMAXCONN) == -1) {
        ::close(fd);
        throw std::runtime_error("listen failed");
    }
    set_nonblocking(fd);
    return fd;
}

void AgentServer::run(std::atomic_bool& running) {
    const int listener = create_listener();
    const int epoll_fd = ::epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        ::close(listener);
        throw std::runtime_error("epoll_create1 failed");
    }

    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = listener;
    if (::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &event) == -1) {
        ::close(listener);
        ::close(epoll_fd);
        throw std::runtime_error("epoll_ctl failed");
    }

    std::array<epoll_event, 16> events{};
    while (running.load()) {
        const int count = ::epoll_wait(epoll_fd, events.data(), events.size(), 500);
        if (count == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        for (int i = 0; i < count; ++i) {
            if (events[static_cast<std::size_t>(i)].data.fd != listener) {
                continue;
            }
            while (true) {
                const int client = ::accept4(listener, nullptr, nullptr, SOCK_CLOEXEC);
                if (client == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }
                    break;
                }
                handle_client(client);
                ::close(client);
            }
        }
    }

    ::close(listener);
    ::close(epoll_fd);
}

void AgentServer::handle_client(int client_fd) {
    std::string request;
    std::array<char, 8192> buffer{};
    const ssize_t n = ::read(client_fd, buffer.data(), buffer.size());
    if (n > 0) {
        request.assign(buffer.data(), static_cast<std::size_t>(n));
    }

    const std::size_t first_line_end = request.find("\r\n");
    const std::string first_line = first_line_end == std::string::npos
        ? request
        : request.substr(0, first_line_end);

    std::istringstream first(first_line);
    std::string method;
    std::string path;
    first >> method >> path;

    const std::size_t body_start = request.find("\r\n\r\n");
    const std::string body = body_start == std::string::npos ? "" : request.substr(body_start + 4);

    std::string payload;
    try {
        payload = route(method, path, body);
    } catch (const std::exception& e) {
        payload = std::string("{\"error\":\"") + json_escape(e.what()) + "\"}";
    }

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: application/json\r\n"
             << "Content-Length: " << payload.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << payload;
    const std::string wire = response.str();
    ::write(client_fd, wire.data(), wire.size());
}

std::string AgentServer::route(
    const std::string& method,
    const std::string& path,
    const std::string& body
) {
    std::lock_guard<std::mutex> guard(request_mutex_);
    if (method == "GET" && path == "/health") {
        return "{\"status\":\"ok\",\"component\":\"linux-net-agent\"}";
    }
    if (method == "GET" && path == "/stats") {
        return stats_json(host_inspector_.read_stats());
    }
    if (method == "GET" && path == "/processes") {
        const auto processes = process_manager_.list_processes();
        std::ostringstream out;
        out << "{\"processes\":[";
        for (std::size_t i = 0; i < processes.size(); ++i) {
            if (i != 0) {
                out << ',';
            }
            out << "{\"pid\":" << processes[i].pid << ",\"ppid\":" << processes[i].ppid
                << ",\"state\":\"" << json_escape(processes[i].state) << "\",\"command\":\""
                << json_escape(processes[i].command) << "\"}";
        }
        out << "]}";
        return out.str();
    }
    if (method == "POST" && path == "/execute") {
        const auto command = json_string_field(body, "command");
        if (!command || command->empty()) {
            return "{\"error\":\"missing command\"}";
        }
        CommandRequest request;
        request.command = *command;
        request.timeout = std::chrono::seconds(json_uint_field(body, "timeout_seconds").value_or(30));
        request.interactive = json_bool_field(body, "interactive");
        request.memory_limit_bytes = json_uint_field(body, "memory_limit_bytes");
        request.cpu_seconds = json_uint_field(body, "cpu_seconds");
        return command_result_json(process_manager_.execute(request));
    }
    return "{\"error\":\"not found\"}";
}

}  // namespace linux_net_agent

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    std::uint16_t port = 7070;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = static_cast<std::uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--help") {
            std::cout << "linux-net-agent [--host 127.0.0.1] [--port 7070]\n";
            return 0;
        }
    }

    ::signal(SIGTERM, linux_net_agent::signal_handler);
    ::signal(SIGINT, linux_net_agent::signal_handler);

    try {
        linux_net_agent::AgentServer server(host, port);
        std::cout << "linux-net-agent listening on " << host << ':' << port << '\n';
        server.run(linux_net_agent::g_running);
    } catch (const std::exception& e) {
        std::cerr << "linux-net-agent failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

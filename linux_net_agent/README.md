# Linux Net Agent

`linux_net_agent/` is a standalone C++20 Linux host agent for OS-level execution and telemetry.
It is intentionally separate from the Python controller so Linux host features can evolve without
coupling the controller to privileged process APIs.

## Capabilities

- HTTP control plane over a TCP socket.
- Health and heartbeat endpoint.
- CPU, memory, load, cgroup, namespace, and uptime stats.
- Process list from `/proc`.
- Command execution through `fork`, `exec`, `waitpid`, pipes, process groups, and signals.
- Timeout based killing with `SIGTERM` followed by `SIGKILL`.
- PTY-backed interactive command path in the process manager.
- Log capture from child stdout/stderr.
- Resource limit hooks with `setrlimit`.
- `epoll` based server loop.
- Systemd service file.

## Build

```bash
cmake -S linux_net_agent -B linux_net_agent/build
cmake --build linux_net_agent/build
```

## Run

```bash
./linux_net_agent/build/linux-net-agent --host 0.0.0.0 --port 7070
```

## API

```bash
curl http://127.0.0.1:7070/health
curl http://127.0.0.1:7070/stats
curl http://127.0.0.1:7070/processes
curl -X POST http://127.0.0.1:7070/execute -d '{"command":"uname -a","timeout_seconds":5}'
```

The controller can call this agent over HTTP today. The same process manager can also be wrapped
behind a Unix socket, TCP socket, or gRPC transport later without changing the OS execution core.

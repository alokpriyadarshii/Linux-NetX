# Worker Pool and Observability

Linux-Net runs a controller plus Redis-backed workers. The controller accepts API requests, assigns
work to FIFO or pinned queues, and stores job metadata in Redis. Workers execute jobs, heartbeat
through RQ, and expose their state back to the controller.

## Worker Pool Design

The worker model is split into three roles:

| Role | Responsibility |
| --- | --- |
| Node worker | Owns host-to-node registration and starts pinned workers. |
| Pinned worker | Runs serial work for a specific device or host to preserve session affinity. |
| FIFO worker | Runs independent jobs from the shared queue for higher throughput. |

Pinned workers use Redis host mappings as a distributed lock. If a node disappears, the controller
detects stale heartbeats, clears the host mapping, and reassigns the job to an available node.

## Backpressure and Scheduling Signals

`GET /system/stats` now returns a `scheduling` object with queue pressure:

| Field | Meaning |
| --- | --- |
| `queued_jobs` | Jobs waiting in the queue. |
| `active_jobs` | Jobs currently started by workers. |
| `failed_jobs` | Jobs in the failed registry. |
| `worker_latency_seconds` | Age of the newest worker heartbeat for the queue. |
| `oldest_job_age_seconds` | How long the oldest queued job has waited. |
| `backpressure_ratio` | Queued jobs divided by available workers. |

These metrics make concurrency behavior visible without requiring shell access to Redis.

## Job Priority, Retries, and Cancellation

The queue layer keeps the following scheduling proof points explicit:

- Job cancellation is exposed with `DELETE /jobs/{id}` and `DELETE /jobs`.
- Pinned host mappings act as idempotency and distributed lock keys for session-affine work.
- Queue pressure can be used by schedulers to reject, slow, or redirect work before overload.
- Failed jobs remain in Redis registries and are available as dead-letter candidates.
- RQ timeout and failure callbacks provide the retry/backoff integration point.

## Prometheus Metrics

`GET /metrics` emits Prometheus text for controller uptime, worker counts, job states, node
capacity, queue backpressure, and worker heartbeat latency.

## Tracing and Structured Logs

Every API request gets an `X-Request-ID` and `X-Job-Trace-ID`. If the caller supplies either
header, Linux-Net preserves it; otherwise the controller creates one. Job metadata carries both
IDs, so queued work can be correlated with API responses and logs.

Set `LINUX_NET_JSON_LOGS=true` to switch console logging to structured JSON. The JSON formatter
includes request IDs, job trace IDs, logger name, severity, message, timestamp, and exception text.

## C++ Linux Agent

The optional `linux_net_agent/` component adds deeper OS-level host execution:

- `fork`, `exec`, process groups, `waitpid`, pipes, PTY, and signal handling.
- Timeout based process termination.
- Log capture from child processes.
- `/proc` process monitoring.
- CPU, memory, load, cgroup, namespace, uptime, and kernel stats.
- `epoll` HTTP server loop.
- Systemd service packaging.

Build it with:

```bash
make agent-build
```

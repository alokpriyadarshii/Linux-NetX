from __future__ import annotations

import contextvars
import json
import logging
import time
import uuid
from collections.abc import Mapping
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from fastapi import Request
    from starlette.middleware.base import RequestResponseEndpoint
    from starlette.responses import Response

request_id_var: contextvars.ContextVar[str | None] = contextvars.ContextVar(
    "linux_net_request_id", default=None
)
job_trace_id_var: contextvars.ContextVar[str | None] = contextvars.ContextVar(
    "linux_net_job_trace_id", default=None
)


def current_request_id() -> str | None:
    return request_id_var.get()


def current_job_trace_id() -> str | None:
    return job_trace_id_var.get()


def new_trace_id(prefix: str = "trace") -> str:
    return f"{prefix}_{uuid.uuid4().hex[:16]}"


class RequestContextFilter(logging.Filter):
    """Attach request and job trace IDs to log records."""

    def filter(self, record: logging.LogRecord) -> bool:
        record.request_id = current_request_id() or "-"
        record.job_trace_id = current_job_trace_id() or "-"
        return True


class JsonLogFormatter(logging.Formatter):
    """Structured JSON formatter for production log aggregation."""

    def format(self, record: logging.LogRecord) -> str:
        payload = {
            "timestamp": self.formatTime(record, self.datefmt),
            "level": record.levelname,
            "logger": record.name,
            "message": record.getMessage(),
            "process": record.process,
            "request_id": getattr(record, "request_id", "-"),
            "job_trace_id": getattr(record, "job_trace_id", "-"),
        }
        if record.exc_info:
            payload["exception"] = self.formatException(record.exc_info)
        return json.dumps(payload, separators=(",", ":"), sort_keys=True)


async def request_context_middleware(
    request: Request, call_next: RequestResponseEndpoint
) -> Response:
    request_id = request.headers.get("X-Request-ID") or new_trace_id("req")
    job_trace_id = request.headers.get("X-Job-Trace-ID") or new_trace_id("job")

    request_token = request_id_var.set(request_id)
    job_token = job_trace_id_var.set(job_trace_id)
    started = time.perf_counter()
    try:
        response = await call_next(request)
    finally:
        request_id_var.reset(request_token)
        job_trace_id_var.reset(job_token)

    response.headers["X-Request-ID"] = request_id
    response.headers["X-Job-Trace-ID"] = job_trace_id
    response.headers["X-Request-Duration-ms"] = f"{(time.perf_counter() - started) * 1000:.2f}"
    return response


def _metric_line(name: str, value: int | float, labels: Mapping[str, Any] | None = None) -> str:
    if not labels:
        return f"{name} {value}"
    rendered_labels = ",".join(
        f'{key}="{str(val).replace(chr(34), chr(92) + chr(34))}"'
        for key, val in sorted(labels.items())
    )
    return f"{name}{{{rendered_labels}}} {value}"


def render_prometheus_metrics(stats: Mapping[str, Any]) -> str:
    workers = stats.get("workers", {})
    jobs = stats.get("jobs", {})
    nodes = stats.get("nodes", {})
    scheduling = stats.get("scheduling", {})
    queues = scheduling.get("queues", []) if isinstance(scheduling, Mapping) else []

    lines = [
        "# HELP linux_net_uptime_seconds Controller process uptime.",
        "# TYPE linux_net_uptime_seconds gauge",
        _metric_line("linux_net_uptime_seconds", stats.get("uptime_seconds", 0)),
        "# HELP linux_net_workers Worker distribution by state and type.",
        "# TYPE linux_net_workers gauge",
    ]

    for key, value in workers.items():
        lines.append(_metric_line("linux_net_workers", value, {"kind": key}))

    lines.extend(
        [
            "# HELP linux_net_jobs Job counts by state.",
            "# TYPE linux_net_jobs gauge",
        ]
    )
    for key, value in jobs.items():
        if isinstance(value, (int, float)):
            lines.append(_metric_line("linux_net_jobs", value, {"state": key}))

    lines.extend(
        [
            "# HELP linux_net_nodes Active node capacity.",
            "# TYPE linux_net_nodes gauge",
            _metric_line("linux_net_nodes", nodes.get("active", 0), {"kind": "active"}),
            _metric_line(
                "linux_net_nodes", nodes.get("total_capacity", 0), {"kind": "total_capacity"}
            ),
            "# HELP linux_net_queue_backpressure Queue pressure ratio per queue.",
            "# TYPE linux_net_queue_backpressure gauge",
        ]
    )
    for queue in queues:
        if isinstance(queue, Mapping):
            lines.append(
                _metric_line(
                    "linux_net_queue_backpressure",
                    queue.get("backpressure_ratio", 0),
                    {"queue": queue.get("name", "unknown")},
                )
            )
            lines.append(
                _metric_line(
                    "linux_net_worker_latency_seconds",
                    queue.get("worker_latency_seconds", 0),
                    {"queue": queue.get("name", "unknown")},
                )
            )

    lines.append("")
    return "\n".join(lines)

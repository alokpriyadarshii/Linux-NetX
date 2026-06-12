import logging

from linux_net.models.common import JobAdditionalData
from linux_net.services.observability import (
    JsonLogFormatter,
    job_trace_id_var,
    render_prometheus_metrics,
    request_id_var,
)


def test_job_metadata_carries_trace_fields():
    meta = JobAdditionalData(
        request_id="req_123",
        job_trace_id="job_456",
        device_name="router1",
        command=["show version"],
    )

    assert meta.request_id == "req_123"
    assert meta.job_trace_id == "job_456"


def test_json_log_formatter_includes_trace_context():
    request_token = request_id_var.set("req_abc")
    job_token = job_trace_id_var.set("job_def")
    try:
        record = logging.LogRecord(
            name="linux_net.test",
            level=logging.INFO,
            pathname=__file__,
            lineno=1,
            msg="worker heartbeat",
            args=(),
            exc_info=None,
        )
        record.request_id = request_id_var.get()
        record.job_trace_id = job_trace_id_var.get()

        rendered = JsonLogFormatter().format(record)
    finally:
        request_id_var.reset(request_token)
        job_trace_id_var.reset(job_token)

    assert '"request_id":"req_abc"' in rendered
    assert '"job_trace_id":"job_def"' in rendered
    assert '"message":"worker heartbeat"' in rendered


def test_render_prometheus_metrics_includes_queue_pressure():
    output = render_prometheus_metrics(
        {
            "uptime_seconds": 10,
            "workers": {"total": 2, "busy": 1},
            "jobs": {"queued": 3, "failed": 1},
            "nodes": {"active": 1, "total_capacity": 4},
            "scheduling": {
                "queues": [
                    {
                        "name": "FifoQ",
                        "backpressure_ratio": 1.5,
                        "worker_latency_seconds": 0.25,
                    }
                ]
            },
        }
    )

    assert 'linux_net_queue_backpressure{queue="FifoQ"} 1.5' in output
    assert 'linux_net_worker_latency_seconds{queue="FifoQ"} 0.25' in output

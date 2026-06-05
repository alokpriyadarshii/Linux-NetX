import os
import time

import requests

BASE_URL = "http://localhost:9000"
API_KEY = os.getenv("LINUX_NET_SERVER__API_KEY", "")
WEBHOOK_URL = os.getenv("LINUX_NET_VERIFY_WEBHOOK_URL", "http://localhost:8888/webhook")
TARGET_HOST = os.getenv("LINUX_NET_VERIFY_HOST", "127.0.0.1")
TARGET_USERNAME = os.getenv("LINUX_NET_VERIFY_USERNAME", "root")
TARGET_PASSWORD = os.getenv("LINUX_NET_VERIFY_PASSWORD", "")


def test_new_detach_flow():
    if not API_KEY or not TARGET_PASSWORD:
        raise RuntimeError(
            "Set LINUX_NET_SERVER__API_KEY and LINUX_NET_VERIFY_PASSWORD before running this helper."
        )

    headers = {"X-API-Key": API_KEY}

    # 1. Launch a detached task with 10s push interval
    payload = {
        "driver": "paramiko",
        "connection_args": {
            "host": TARGET_HOST,
            "username": TARGET_USERNAME,
            "password": TARGET_PASSWORD,
        },
        "command": "ping 8.8.8.8",
        "detach": True,
        "push_interval": 10,  # 每10秒向 Webhook 发送一次结果
        "webhook": {"url": WEBHOOK_URL, "name": "basic"},
    }

    print(f"Step 1: Launching detached task (push_interval: 10s, target: {WEBHOOK_URL})...")
    resp = requests.post(f"{BASE_URL}/device/exec", json=payload, headers=headers)
    print(f"Launch Response: {resp.status_code}, {resp.json()}")

    if resp.status_code != 201:
        print("Launch failed!")
        return

    job_data = resp.json()
    task_id = job_data.get("task_id")

    print(f"Task ID: {task_id} successfully launched.")

    # 2. Wait for a few cycles of webhooks to trigger
    print(
        "\nStep 2: Monitoring task... "
        "(Waiting 25 seconds to see periodic webhooks at your listener)"
    )
    for i in range(5):
        time.sleep(5)
        # Check task status via API
        resp = requests.get(f"{BASE_URL}/tasks/{task_id}", headers=headers)
        data = resp.json()
        print(
            f"Check {i + 1} (API Status): {data.get('status')}, Last Sync: {data.get('last_sync')}"
        )

    # 3. Final Query
    print("\nStep 3: Final synchronous log query...")
    resp = requests.get(f"{BASE_URL}/tasks/{task_id}", headers=headers)
    output = resp.json().get("query", {}).get("output", "")
    print(f"Output received: {len(output)} bytes")

    # 4. Kill task
    print("\nStep 4: Killing task...")
    resp = requests.delete(f"{BASE_URL}/tasks/{task_id}", headers=headers)
    print(f"Kill Response: {resp.status_code}, {resp.json()}")


if __name__ == "__main__":
    test_new_detach_flow()

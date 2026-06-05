# Linux-Net

Linux-Net is a distributed **FastAPI based network and Linux automation platform** for executing commands, pushing configuration, transferring files, and managing long running tasks across network devices and Linux hosts.

It uses a controller/worker architecture with **Redis + RQ** for queueing, pluggable execution drivers such as **Netmiko**, **Paramiko**, **NAPALM**, and **PyEAPI**, plus optional **Vault KV** credential storage and **MongoDB** audit archiving.

---

## Key Features

- Unified REST API for device command execution and configuration deployment.
- Single-device and bulk-device execution APIs.
- Pluggable driver system for `netmiko`, `paramiko`, `napalm`, and `pyeapi`.
- FIFO and pinned queue strategies for scalable execution.
- Persistent SSH sessions and keepalive support for low-latency repeated operations.
- Detached/background Linux task execution through the Paramiko driver.
- File upload/download workflow with staged storage and download URLs.
- Template rendering with Jinja2.
- Output parsing with TextFSM and TTP.
- Webhook callbacks with retry support.
- API-key authentication through `X-API-KEY`.
- Optional Vault KV credential provider to avoid sending passwords in every request.
- Optional MongoDB archiver worker for audit/compliance records.
- Docker Compose, Kubernetes YAML, and Helm deployment assets.
- Postman collection included under `postman/`.

---

## Project Structure

```text
Linux-Net/
├── .github/
│   └── workflows/
│       ├── docs.yml
│       ├── ruff.yml
│       └── test-containerlab.yml
├── config/
│   ├── config.yaml
│   └── log-config.yaml
├── deploy/
│   ├── charts/
│   │   └── linux-net/
│   │       ├── Chart.yaml
│   │       ├── values.yaml
│   │       └── templates/
│   │           ├── NOTES.txt
│   │           ├── _helpers.tpl
│   │           ├── config.yaml
│   │           ├── controller.yaml
│   │           ├── ingress.yaml
│   │           ├── redis.yaml
│   │           ├── storage.yaml
│   │           ├── vault-init.yaml
│   │           ├── vault.yaml
│   │           └── workers.yaml
│   ├── k8s/
│   │   ├── scripts/
│   │   │   ├── distribute_images.sh
│   │   │   ├── k8s_setup_secrets.sh
│   │   │   └── vault_auto_init.sh
│   │   ├── 00-secrets.yaml
│   │   ├── 01-linux-net-storage.yaml
│   │   ├── 01-redis-operator.yaml
│   │   ├── 01-redis.yaml
│   │   ├── 02-linux-net.yaml
│   │   ├── 02-redis-operator.yaml
│   │   ├── 03-ingress.yaml
│   │   ├── 03-redis-cluster.yaml
│   │   ├── 04-nodeport.yaml
│   │   ├── 04-vault.yaml
│   │   ├── 05-vault-init.yaml
│   │   ├── 05-vault.yaml
│   │   ├── 06-linux-net-core.yaml
│   │   ├── 07-ingress.yaml
│   │   ├── 08-nodeport.yaml
│   │   ├── 09-mongodb.yaml
│   │   ├── calico-ippool-v2.yaml
│   │   └── README.md
│   └── README.md
├── docker/
│   ├── archiver_worker.dockerfile
│   ├── controller.dockerfile
│   ├── fifo_worker.dockerfile
│   ├── node_worker.dockerfile
│   └── ssh_config
├── docs/
│   ├── assets/
│   ├── en/
│   ├── overrides/
│   │   └── main.html
│   └── stylesheets/
│       ├── code-pro.css
│       ├── extra.css
│       ├── menu-enhanced.css
│       └── minimal.css
├── images/
│   ├── preview-1.png
│   ├── preview-2.png
│   ├── preview-3.png
│   └── preview-4.png
├── linux_net/
│   ├── cli/
│   │   ├── csv/
│   │   │   └── devices.csv
│   │   ├── examples/
│   │   │   ├── test_devices.csv
│   │   │   ├── test_empty_devices.csv
│   │   │   └── test_template.textfsm
│   │   └── main.py
│   ├── models/
│   │   ├── __init__.py
│   │   ├── common.py
│   │   ├── driver.py
│   │   ├── request.py
│   │   └── response.py
│   ├── plugins/
│   │   ├── credentials/
│   │   │   └── vault_kv/
│   │   ├── drivers/
│   │   │   ├── napalm/
│   │   │   ├── netmiko/
│   │   │   ├── paramiko/
│   │   │   └── pyeapi/
│   │   ├── schedulers/
│   │   │   ├── greedy/
│   │   │   ├── least_load/
│   │   │   ├── least_load_random/
│   │   │   └── load_weighted_random/
│   │   ├── templates/
│   │   │   ├── jinja2/
│   │   │   ├── textfsm/
│   │   │   └── ttp/
│   │   └── webhooks/
│   │       └── basic/
│   ├── routes/
│   │   ├── __init__.py
│   │   ├── detached_task.py
│   │   ├── device.py
│   │   ├── manage.py
│   │   ├── storage.py
│   │   └── template.py
│   ├── server/
│   │   ├── __init__.py
│   │   └── common.py
│   ├── services/
│   │   ├── __init__.py
│   │   ├── audit.py
│   │   ├── manager.py
│   │   ├── rediz.py
│   │   ├── rpc.py
│   │   └── supervisor.py
│   ├── utils/
│   │   ├── __init__.py
│   │   ├── config.py
│   │   ├── exceptions.py
│   │   └── logger.py
│   ├── worker/
│   │   ├── __init__.py
│   │   ├── archiver.py
│   │   ├── common.py
│   │   ├── fifo.py
│   │   ├── node.py
│   │   └── pinned.py
│   ├── __init__.py
│   └── controller.py
├── postman/
│   └── Linux-Net.postman_collection.json
├── redis/
│   ├── tls/
│   │   ├── ca.crt
│   │   ├── ca.key
│   │   ├── ca.txt
│   │   ├── redis.crt
│   │   ├── redis.dh
│   │   └── redis.key
│   └── redis.conf
├── scripts/
│   ├── docker_auto_deploy.sh
│   ├── export_openapi.py
│   ├── generate_redis_certs.sh
│   ├── k8s_setup_secrets.sh
│   ├── setup_env.sh
│   ├── setup_git_template.sh
│   └── vault_auto_init.sh
├── tests/
│   ├── clab/
│   │   ├── README.md
│   │   └── e2e.clab.yaml
│   ├── data/
│   │   ├── config.e2e.yaml
│   │   └── config.test.yaml
│   ├── e2e/
│   │   ├── __init__.py
│   │   ├── conftest.py
│   │   ├── settings.py
│   │   ├── test_api.py
│   │   ├── test_driver_netmiko.py
│   │   └── test_driver_paramiko.py
│   ├── unit/
│   │   ├── conftest.py
│   │   ├── test_api_integration.py
│   │   ├── test_audit.py
│   │   ├── test_credentials.py
│   │   ├── test_driver_netmiko.py
│   │   ├── test_driver_paramiko.py
│   │   ├── test_driver_paramiko_advanced.py
│   │   ├── test_driver_paramiko_detach.py
│   │   ├── test_driver_paramiko_detached_advanced.py
│   │   ├── test_driver_paramiko_jinja2.py
│   │   ├── test_driver_paramiko_proxy.py
│   │   ├── test_infra.py
│   │   ├── test_logger.py
│   │   ├── test_manager.py
│   │   ├── test_models.py
│   │   ├── test_plugin_loader.py
│   │   ├── test_routes.py
│   │   ├── test_routes_storage.py
│   │   ├── test_rpc.py
│   │   ├── test_schedulers.py
│   │   ├── test_security.py
│   │   ├── test_template.py
│   │   ├── test_webhooks.py
│   │   └── test_worker.py
│   ├── README.md
│   ├── __init__.py
│   ├── conftest.py
│   └── verify_new_detach.py
├── tools/
│   └── webhook/
│       ├── README.md
│       └── webhook_server.py
├── vault/
│   └── config/
│       └── vault.hcl
├── .dockerignore
├── .env.example
├── .gitignore
├── .gitmessage
├── .python-version
├── .readthedocs.yaml
├── docker-compose.dev.yaml
├── docker-compose.yaml
├── gunicorn.conf.py
├── LICENSE
├── llms.txt
├── Makefile
├── mkdocs.yml
├── pyproject.toml
├── README.md
├── repomix.config.json
└── worker.py

---

## Security Notice Before Publishing

This project archive may contain generated runtime secrets from a local deployment. Before uploading to GitHub, sending to anyone, or using in production, remove and rotate:

```text
.env
redis/tls/*.key
redis/tls/*.crt
redis/tls/*.dh
vault/data/
*.token
*.key
```

Use `.env.example` as the committed template and generate fresh secrets per environment.

Recommended cleanup before publishing:

```bash
rm -f .env
rm -rf vault/data vault/logs
rm -rf redis/tls redis/data
```

Then regenerate secrets only on the target machine:

```bash
bash scripts/setup_env.sh generate
bash scripts/generate_redis_certs.sh
```

---

## Tech Stack

| Category | Technologies |
|---|---|
| Language | Python 3.12+ |
| API Framework | FastAPI |
| ASGI / Server | Uvicorn, Gunicorn, Uvloop |
| Data Validation | Pydantic, Pydantic Settings |
| Task Queue | Redis Queue, RQ |
| Cache / Broker | Redis |
| Database | MongoDB, PyMongo |
| Secrets Management | HashiCorp Vault, HVAC |
| Network Automation | Netmiko, Paramiko, NAPALM, ncclient, puresnmp, pyeapi |
| Template Engines | Jinja2, TextFSM, NTC Templates, TTP |
| CLI Tools | Rich, Pandas, OpenPyXL |
| Deployment | Docker, Docker Compose, Kubernetes, Helm |
| Kubernetes Components | Ingress, NodePort, Persistent Storage, Redis Operator |
| Testing | Pytest, Pytest-Cov, Fakeredis, HTTPX |
| Code Quality | Ruff |
| Documentation | MkDocs, MkDocs Material, Read the Docs |
| API Testing | Postman |
| CI/CD | GitHub Actions |

## Prerequisites

For Docker Compose deployment:

- Docker
- Docker Compose v2
- OpenSSL
- Bash-compatible shell

For local Python development:

- Python 3.12+
- Redis reachable from the application
- Optional: Vault and MongoDB if those features are enabled

For Kubernetes deployment:

- Kubernetes 1.24+
- `kubectl`
- Helm 3.x, if using the Helm chart

---

## Quick Start with Docker Compose

The easiest way to run the complete stack is Docker Compose.

```bash
unzip "Linux-Net(2).zip"
cd Linux-Net

# If this archive came from another machine, remove generated secrets first.
rm -f .env
rm -rf vault/data redis/tls redis/data

# One-click setup: generates .env, generates Redis TLS certificates,
# builds images, and starts controller/workers/Redis/Vault.
make deploy
```

After deployment, check that the services are running:

```bash
docker compose ps
```

Load the generated API key and call the health endpoint:

```bash
export API_KEY="$(grep '^LINUX_NET_SERVER__API_KEY=' .env | cut -d= -f2-)"

curl -H "X-API-KEY: $API_KEY" http://localhost:9000/health
curl -H "X-API-KEY: $API_KEY" http://localhost:9000/system/stats
```

Open the interactive API docs:

```text
http://localhost:9000/docs
```

---

## Manual Docker Compose Commands

```bash
# Generate .env and Redis TLS certificates
bash scripts/setup_env.sh generate
bash scripts/generate_redis_certs.sh

# Build and start
make build
make up

# Logs and status
make logs
make ps
make health

# Restart / stop
make restart
make down

# Scale workers
make scale NODE=2 FIFO=1
```

Development compose commands:

```bash
make dev-build
make dev-up
make dev-logs
make dev-down
```

---

## Configuration

Main configuration file:

```text
config/config.yaml
```

Important environment variables:

| Variable | Purpose |
|---|---|
| `LINUX_NET_SERVER__API_KEY` | API key required by protected endpoints |
| `LINUX_NET_SERVER__EXTERNAL_URL` | External URL used for generated download links |
| `LINUX_NET_REDIS__PASSWORD` | Redis authentication password |
| `LINUX_NET_LOG__LEVEL` | Runtime log level |
| `LINUX_NET_CREDENTIAL__ENABLED` | Enables/disables credential provider support |
| `LINUX_NET_CREDENTIAL__NAME` | Credential provider name, default `vault_kv` |
| `LINUX_NET_CREDENTIAL__ADDR` | Vault address |
| `LINUX_NET_VAULT_TOKEN` | Vault token for local/dev operation |
| `LINUX_NET_MONGODB__ENABLED` | Enables MongoDB audit archiving |
| `LINUX_NET_MONGODB__URI` | MongoDB connection URI |

Use `.env.example` as the template for local environment configuration.

---

## API Authentication

Most endpoints require the configured API key.

```bash
-H "X-API-KEY: $API_KEY"
```

The header name is configurable in `config/config.yaml` through:

```yaml
server:
  api_key_name: X-API-KEY
```

---

## Main API Endpoints

| Method | Endpoint | Purpose |
|---|---|---|
| `GET` | `/health` | Basic health check |
| `GET` | `/system/stats` | System/worker/job statistics |
| `POST` | `/device/exec` | Execute command/config/file operation on one device |
| `POST` | `/device/bulk` | Execute command/config on many devices |
| `POST` | `/device/test` | Test connectivity to a device |
| `GET` | `/jobs` | List jobs |
| `GET` | `/jobs/{id}` | Get job result/status |
| `DELETE` | `/jobs/{id}` | Cancel queued job |
| `GET` | `/workers` | List workers |
| `DELETE` | `/workers/{name}` | Stop a worker |
| `POST` | `/template/render` | Render template |
| `POST` | `/template/render/{name}` | Render with named renderer |
| `POST` | `/template/parse` | Parse output |
| `POST` | `/template/parse/{name}` | Parse with named parser |
| `GET` | `/detached-tasks` | List detached tasks |
| `GET` | `/detached-tasks/{task_id}` | Query detached task logs/status |
| `DELETE` | `/detached-tasks/{task_id}` | Kill detached task |
| `POST` | `/detached-tasks/discover` | Discover active detached tasks on a host |
| `GET` | `/storage/fetch/{file_id}` | Fetch staged download file |

---

## API Examples

### 1. Execute a Network Command

```bash
curl -X POST http://localhost:9000/device/exec \
  -H "Content-Type: application/json" \
  -H "X-API-KEY: $API_KEY" \
  -d '{
    "driver": "netmiko",
    "connection_args": {
      "device_type": "cisco_ios",
      "host": "192.168.1.10",
      "username": "admin",
      "password": "admin"
    },
    "command": ["show version"],
    "queue_strategy": "pinned"
  }'
```

The API returns a queued job:

```json
{
  "id": "job-id",
  "status": "queued",
  "queue": "HostQ_192.168.1.10"
}
```

Poll the result:

```bash
curl -H "X-API-KEY: $API_KEY" http://localhost:9000/jobs/job-id
```

---

### 2. Push Configuration

```bash
curl -X POST http://localhost:9000/device/exec \
  -H "Content-Type: application/json" \
  -H "X-API-KEY: $API_KEY" \
  -d '{
    "driver": "netmiko",
    "connection_args": {
      "device_type": "cisco_ios",
      "host": "192.168.1.10",
      "username": "admin",
      "password": "admin"
    },
    "config": [
      "interface GigabitEthernet0/1",
      "description Managed by Linux-Net"
    ],
    "save": true
  }'
```

---

### 3. Bulk Command Execution

```bash
curl -X POST http://localhost:9000/device/bulk \
  -H "Content-Type: application/json" \
  -H "X-API-KEY: $API_KEY" \
  -d '{
    "driver": "netmiko",
    "connection_args": {
      "username": "admin",
      "password": "admin"
    },
    "command": ["show ip interface brief"],
    "devices": [
      {"host": "192.168.1.10", "device_type": "cisco_ios"},
      {"host": "192.168.1.11", "device_type": "arista_eos"}
    ]
  }'
```

---

### 4. Test Device Connectivity

```bash
curl -X POST http://localhost:9000/device/test \
  -H "Content-Type: application/json" \
  -H "X-API-KEY: $API_KEY" \
  -d '{
    "driver": "netmiko",
    "connection_args": {
      "device_type": "cisco_ios",
      "host": "192.168.1.10",
      "username": "admin",
      "password": "admin"
    }
  }'
```

---

### 5. Render a Jinja2 Template

```bash
curl -X POST http://localhost:9000/template/render/jinja2 \
  -H "Content-Type: application/json" \
  -H "X-API-KEY: $API_KEY" \
  -d '{
    "template": "hostname {{ hostname }}\ninterface {{ intf }}\n description {{ desc }}",
    "context": {
      "hostname": "router1",
      "intf": "GigabitEthernet0/1",
      "desc": "Configured by Linux-Net"
    }
  }'
```

---

### 6. Detached Linux Task with Paramiko

Detached mode is currently designed for the `paramiko` driver.

```bash
curl -X POST http://localhost:9000/device/exec \
  -H "Content-Type: application/json" \
  -H "X-API-KEY: $API_KEY" \
  -d '{
    "driver": "paramiko",
    "connection_args": {
      "host": "192.168.1.50",
      "username": "linuxuser",
      "password": "linuxpass"
    },
    "command": "sleep 60 && echo done",
    "detach": true,
    "push_interval": 10
  }'
```

List detached tasks:

```bash
curl -H "X-API-KEY: $API_KEY" http://localhost:9000/detached-tasks
```

Query logs/status:

```bash
curl -H "X-API-KEY: $API_KEY" http://localhost:9000/detached-tasks/<task_id>
```

Kill a detached task:

```bash
curl -X DELETE -H "X-API-KEY: $API_KEY" http://localhost:9000/detached-tasks/<task_id>
```

---

### 7. File Upload with Multipart Request

`/device/exec` supports multipart mode with:

- `request`: JSON string containing the execution request.
- `file`: uploaded file to stage for the worker.

```bash
curl -X POST http://localhost:9000/device/exec \
  -H "X-API-KEY: $API_KEY" \
  -F 'request={
    "driver":"paramiko",
    "connection_args":{
      "host":"192.168.1.50",
      "username":"linuxuser",
      "password":"linuxpass"
    },
    "file_transfer":{
      "operation":"upload",
      "remote_path":"/tmp/uploaded.bin",
      "overwrite":true
    }
  }' \
  -F 'file=@./local-file.bin'
```

---

## Queue Strategies

| Strategy | Value | Use case |
|---|---|---|
| FIFO | `fifo` | Stateless jobs and simple fan-out execution |
| Pinned | `pinned` | Reuse a worker/session for a specific host |

Default behavior:

- `netmiko` defaults to pinned mode.
- Detached tasks default to pinned mode.
- Other drivers generally default to FIFO unless overridden.

---

## Supported Driver Plugins

| Driver | Target | Typical Use |
|---|---|---|
| `netmiko` | Network CLI devices | Cisco, HP/Comware, Huawei, Fortinet, Arista CLI workflows |
| `paramiko` | Linux/SSH hosts | Linux shell commands, SFTP, detached tasks |
| `napalm` | Network automation drivers | Normalized device operations and config workflows |
| `pyeapi` | Arista EOS | EOS API/JSON-RPC command and config execution |

---

## Template Plugins

| Plugin | Direction | Purpose |
|---|---|---|
| `jinja2` | Data → text | Render commands/config from structured context |
| `textfsm` | Text → data | Parse CLI output into structured data |
| `ttp` | Text → data | Parse network output using TTP templates |

---

## Credential Provider: Vault KV

Credential support is configured through `config/config.yaml` and `.env`.

Enable Vault credential resolution:

```env
LINUX_NET_CREDENTIAL__ENABLED=true
LINUX_NET_CREDENTIAL__NAME=vault_kv
LINUX_NET_CREDENTIAL__ADDR=http://vault:8200
LINUX_NET_CREDENTIAL__ALLOWED_PATHS=kv/linux_net
LINUX_NET_VAULT_TOKEN=<your-token>
```

Then send a credential reference instead of raw username/password:

```json
{
  "driver": "netmiko",
  "connection_args": {
    "device_type": "cisco_ios",
    "host": "192.168.1.10"
  },
  "credential": {
    "name": "vault_kv",
    "ref": "linux_net/router-01",
    "mount": "kv",
    "field_mapping": {
      "username": "user",
      "password": "pass"
    }
  },
  "command": "show version"
}
```

---

## CLI Usage

Install the package locally with CLI dependencies:

```bash
python3.12 -m venv .venv
source .venv/bin/activate
pip install -e ".[api,tool]"
```

Prepare a device list CSV:

```csv
Selected,IP,Port,Username,Password,Vendor,Keepalive
true,192.168.1.10,22,admin,admin,CISCO,60
true,192.168.1.11,22,admin,admin,ARISTA,60
```

Run a command against selected devices:

```bash
linux-net-cli \
  --endpoint http://localhost:9000 \
  --api-key "$API_KEY" \
  exec devices.csv \
  --command "show version" \
  --driver netmiko \
  --force
```

Push configuration:

```bash
linux-net-cli \
  --endpoint http://localhost:9000 \
  --api-key "$API_KEY" \
  exec devices.csv \
  --config "interface Loopback100\ndescription Managed by Linux-Net" \
  --driver netmiko \
  --save \
  --force
```

The CLI saves monitored results to a timestamped CSV file such as:

```text
result_YYYYMMDD_HHMMSS.csv
```

---

## Testing

Install test dependencies:

```bash
python3.12 -m venv .venv
source .venv/bin/activate
pip install -e ".[api,dev,test]"
```

Run unit/API tests:

```bash
pytest -q
pytest -m api
pytest -m "not e2e"
```

Run e2e tests with ContainerLab topology:

```bash
SRL_EULA_ACCEPT=true containerlab deploy -t tests/clab/e2e.clab.yaml
pytest --e2e -q -m e2e
containerlab destroy -t tests/clab/e2e.clab.yaml
```

---

## Kubernetes / Helm Deployment

Helm chart path:

```text
deploy/charts/linux-net
```

Install with Helm:

```bash
helm dependency update ./deploy/charts/linux-net

helm install linux-net ./deploy/charts/linux-net \
  --namespace linux-net \
  --create-namespace \
  --set secrets.redisPassword=<YOUR_REDIS_PASSWORD> \
  --set secrets.apiKey=<YOUR_API_KEY> \
  --set secrets.vaultToken=<YOUR_VAULT_TOKEN>
```

Raw Kubernetes manifests are available under:

```text
deploy/k8s/
```

See `deploy/README.md` and `deploy/k8s/README.md` for deployment details.

---

## Useful Make Commands

| Command | Description |
|---|---|
| `make help` | Show available operations |
| `make deploy` | One-click Docker deployment |
| `make build` | Build Docker images |
| `make up` | Start production compose stack |
| `make down` | Stop production compose stack |
| `make logs` | Follow logs |
| `make ps` | Show service status |
| `make restart` | Restart services |
| `make scale NODE=2 FIFO=1` | Scale worker counts |
| `make lint` | Run Ruff lint/fix |
| `make format` | Format code |
| `make clean` | Stop services while preserving data |
| `make clean-all` | Stop and remove volumes/data |
| `make ai-docs` | Generate AI-ready context artifacts |

---

## Troubleshooting

### 401 or 403 API errors

Check that the request includes the correct API key:

```bash
grep LINUX_NET_SERVER__API_KEY .env
curl -H "X-API-KEY: $API_KEY" http://localhost:9000/health
```

### Redis TLS connection errors

Regenerate certificates and restart the stack:

```bash
rm -rf redis/tls
bash scripts/generate_redis_certs.sh
docker compose down
docker compose --env-file .env up -d
```

### Jobs stay queued

Check workers and logs:

```bash
docker compose ps
docker compose logs -f fifo-worker node-worker controller
curl -H "X-API-KEY: $API_KEY" http://localhost:9000/workers
```

### Vault credential resolution fails

Check Vault state and configuration:

```bash
docker compose ps vault vault-init
docker compose logs vault vault-init
grep LINUX_NET_CREDENTIAL .env
```

### CLI says no devices selected
Ensure the input CSV/XLSX has a `Selected` column and selected rows are set to `true`.

# OCPE — Online C Program Evaluation

A distributed online judge for evaluating programs with correctness and resource-usage measurement. Built as a C++ backend with Redis-based async job routing, an 8-worker dequeue pipeline, Linux sandbox isolation, user authentication, and multi-language support (C, C++, Python).

## Architecture

```
┌─────────┐     HTTP      ┌──────────┐    LPUSH/BRPOP    ┌─────────┐
│  Web UI │ ────────────► │ API Server│ ◄──────────────► │  Redis  │
└─────────┘               └──────────┘                   └────┬────┘
                                                              │
                         ┌────────────────────────────────────┼────────────────────────────────────┐
                         │                                    │                                    │
                    ┌────▼────┐  ┌─────────┐  ┌─────────┐  ┌─▼───────┐  ... (8 workers total)
                    │ Worker 0│  │ Worker 1│  │ Worker 2│  │ Worker 7│
                    └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘
                         │            │            │            │
                         └────────────┴────────────┴────────────┘
                                              │
                                    ┌─────────▼─────────┐
                                    │  Linux Sandbox    │
                                    │  fork + setrlimit │
                                    │  wait4 + rusage   │
                                    └───────────────────┘
```

### Components

| Component | Description |
|-----------|-------------|
| **API Server** | C++ HTTP server (cpp-httplib) accepting code submissions, enqueueing jobs to Redis |
| **Redis Queue** | Async routing via `LPUSH` / `BRPOP` on `ocpe:submission_queue` |
| **Worker Pool** | 8 concurrent worker threads dequeuing and evaluating submissions |
| **Sandbox** | `fork()` + `setrlimit(RLIMIT_AS)` for 256MB memory cap, `RLIMIT_CPU` for time limits, `wait4()` for CPU time and peak RSS |
| **Web UI** | Static frontend with login, language selector, and submission history |

## Quick Start (Docker)

**Requires:** Docker and Docker Compose. The sandbox uses Linux-specific syscalls (`fork`, `setrlimit`, `wait4`) and must run in a Linux container.

```bash
docker compose up --build
```

| Service | URL |
|---------|-----|
| Web UI | http://localhost:3000 |
| API | http://localhost:8080 |
| Redis | localhost:6379 |

## API Endpoints

| Method | Path | Auth | Description |
|--------|------|------|-------------|
| GET | `/health` | No | Health check |
| GET | `/languages` | No | List supported languages |
| POST | `/auth/register` | No | Create account (`username`, `password`) |
| POST | `/auth/login` | No | Login, returns `token` |
| GET | `/auth/me` | Yes | Current user info |
| GET | `/auth/submissions` | Yes | Submission history |
| GET | `/problems` | No | List all problems |
| GET | `/problems/:id` | No | Get problem details |
| POST | `/submit` | Yes | Submit code (`problem_id`, `source_code`, `language`) |
| GET | `/submissions/:id` | Yes | Get submission status/result |
| GET | `/stats` | No | Queue length and worker count |

Authenticated requests require header: `Authorization: Bearer <token>`

### Supported Languages

| ID | Compiler / Runtime |
|----|-------------------|
| `c` | gcc -std=c11 |
| `cpp` | g++ -std=c++17 |
| `python` | python3 |

## Sample Problems

| ID | Title | Test Cases |
|----|-------|------------|
| `sum_two` | Sum of Two Numbers | 3 |
| `fibonacci` | Fibonacci Number | 3 |
| `stress_120` | Stress Test — 120 Cases | 120 |

## Load Testing (100+ Concurrent Submissions)

With the stack running via Docker:

```bash
pip install -r scripts/requirements.txt
python scripts/load_test.py --api http://localhost:8080 --count 120 --workers 50
```

This registers a test user, fires 120 concurrent submissions through the Redis queue, and reports throughput plus p50/p95 latency.

## Sandbox Details

Each test case runs in an isolated child process:

- **Memory:** `setrlimit(RLIMIT_AS, 256MB)` — address space capped at 256MB
- **CPU time:** `setrlimit(RLIMIT_CPU, time_limit)` — hard CPU limit
- **Isolation:** `fork()` creates separate process; `setpgid()` for process group; drops to `nobody` (uid 65534)
- **Metrics:** `wait4()` with `struct rusage` captures CPU time (`ru_utime` + `ru_stime`) and peak memory (`ru_maxrss`)
- **Verdicts:** ACCEPTED, WRONG_ANSWER, TIME_LIMIT_EXCEEDED, MEMORY_LIMIT_EXCEEDED, RUNTIME_ERROR, COMPILATION_ERROR

## Building Locally (Linux)

```bash
sudo apt install build-essential cmake libhiredis-dev libssl-dev gcc g++ python3
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

Set environment variables:

```bash
export REDIS_HOST=127.0.0.1
export REDIS_PORT=6379
export PROBLEMS_DIR=./problems
export SUBMISSIONS_DIR=./data/submissions
export WORKER_COUNT=8
```

Run Redis, then start the API and worker:

```bash
redis-server &
./build/api/ocpe_api &
./build/worker/ocpe_worker
```

## Project Structure

```
OCPE/
├── api/           # HTTP API server
├── worker/        # 8-thread worker pool with Redis dequeue
├── sandbox/       # fork/setrlimit/wait4 execution engine
├── shared/        # Redis client, models, problem loader
├── problems/      # Problem definitions and test cases
├── web/           # Frontend UI
├── docker/        # Dockerfile and nginx config
└── scripts/       # Test generators and load test
```

## License

MIT

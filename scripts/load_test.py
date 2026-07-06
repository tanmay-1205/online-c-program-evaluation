#!/usr/bin/env python3
"""
OCPE load test — submits 100+ concurrent code evaluations and reports throughput.

Usage:
  python scripts/load_test.py --api http://localhost:8080 --count 120 --workers 50

Requires: pip install aiohttp
"""

import argparse
import asyncio
import random
import string
import time
from dataclasses import dataclass, field

try:
    import aiohttp
except ImportError:
    raise SystemExit("Install aiohttp: pip install aiohttp")

SOLUTION = """#include <stdio.h>
int main() {
    int a, b;
    scanf("%d %d", &a, &b);
    printf("%d\\n", a + b);
    return 0;
}"""

TERMINAL = {
    "ACCEPTED", "WRONG_ANSWER", "TIME_LIMIT_EXCEEDED",
    "MEMORY_LIMIT_EXCEEDED", "RUNTIME_ERROR", "COMPILATION_ERROR", "INTERNAL_ERROR",
}


@dataclass
class Stats:
    submitted: int = 0
    completed: int = 0
    accepted: int = 0
    errors: int = 0
    latencies: list = field(default_factory=list)


async def register_and_login(session: aiohttp.ClientSession, api: str, suffix: str) -> str:
    username = f"loadtest_{suffix}"
    password = "loadtest123"
    async with session.post(f"{api}/auth/register", json={"username": username, "password": password}) as resp:
        if resp.status not in (200, 201):
            async with session.post(f"{api}/auth/login", json={"username": username, "password": password}) as login_resp:
                login_resp.raise_for_status()
                data = await login_resp.json()
                return data["token"]
        data = await resp.json()
        return data["token"]


async def submit_one(session: aiohttp.ClientSession, api: str, token: str, problem_id: str, stats: Stats):
    headers = {"Authorization": f"Bearer {token}"}
    t0 = time.perf_counter()
    try:
        async with session.post(
            f"{api}/submit",
            json={"problem_id": problem_id, "source_code": SOLUTION, "language": "c"},
            headers=headers,
        ) as resp:
            if resp.status != 202:
                stats.errors += 1
                return
            data = await resp.json()
            sub_id = data["submission_id"]
            stats.submitted += 1

        while True:
            await asyncio.sleep(0.5)
            async with session.get(f"{api}/submissions/{sub_id}", headers=headers) as resp:
                result = await resp.json()
                verdict = result.get("verdict", "PENDING")
                if verdict in TERMINAL:
                    elapsed = time.perf_counter() - t0
                    stats.latencies.append(elapsed)
                    stats.completed += 1
                    if verdict == "ACCEPTED":
                        stats.accepted += 1
                    return
    except Exception:
        stats.errors += 1


async def run_load_test(api: str, count: int, concurrency: int, problem_id: str):
    suffix = "".join(random.choices(string.ascii_lowercase + string.digits, k=8))
    stats = Stats()

    connector = aiohttp.TCPConnector(limit=concurrency)
    timeout = aiohttp.ClientTimeout(total=300)
    async with aiohttp.ClientSession(connector=connector, timeout=timeout) as session:
        token = await register_and_login(session, api, suffix)
        print(f"Authenticated as loadtest_{suffix}")
        print(f"Submitting {count} jobs with concurrency={concurrency}...")

        sem = asyncio.Semaphore(concurrency)
        start = time.perf_counter()

        async def bounded_submit():
            async with sem:
                await submit_one(session, api, token, problem_id, stats)

        await asyncio.gather(*[bounded_submit() for _ in range(count)])

        total_time = time.perf_counter() - start

    print("\n=== Load Test Results ===")
    print(f"Total submissions:  {count}")
    print(f"Completed:          {stats.completed}")
    print(f"Accepted:           {stats.accepted}")
    print(f"Errors:             {stats.errors}")
    print(f"Total wall time:    {total_time:.2f}s")
    print(f"Throughput:         {stats.completed / total_time:.2f} submissions/sec")
    if stats.latencies:
        stats.latencies.sort()
        p50 = stats.latencies[len(stats.latencies) // 2]
        p95 = stats.latencies[int(len(stats.latencies) * 0.95)]
        print(f"Latency p50:        {p50:.2f}s")
        print(f"Latency p95:        {p95:.2f}s")
        print(f"Latency min/max:    {min(stats.latencies):.2f}s / {max(stats.latencies):.2f}s")


def main():
    parser = argparse.ArgumentParser(description="OCPE concurrent submission load test")
    parser.add_argument("--api", default="http://localhost:8080", help="API base URL")
    parser.add_argument("--count", type=int, default=120, help="Number of submissions")
    parser.add_argument("--workers", type=int, default=50, help="Concurrent submitters")
    parser.add_argument("--problem", default="sum_two", help="Problem ID")
    args = parser.parse_args()

    asyncio.run(run_load_test(args.api.rstrip("/"), args.count, args.workers, args.problem))


if __name__ == "__main__":
    main()

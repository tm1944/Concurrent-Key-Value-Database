#!/usr/bin/env python3
"""Measure one request at a time on persistent localhost connections."""

import argparse
import concurrent.futures
import math
import socket
import statistics
import subprocess
import tempfile
import threading
import time
from pathlib import Path

ADDRESS = ("127.0.0.1", 6379)
TIMEOUT = 30
VALUE = "benchmark-value"


def command(connection, reader, request, expected):
    connection.sendall(request)
    reply = reader.readline(1024)
    if reply != expected:
        raise RuntimeError(f"Expected {expected!r}, received {reply!r}")


def client(workload, count, barrier):
    samples = []
    expected = (VALUE + "\n").encode() if workload == "GET" else b"OK\n"
    requests = [
        (f"GET k{i}\n" if workload == "GET" else f"SET k{i} {VALUE}\n").encode()
        for i in range(100)
    ]
    try:
        with socket.create_connection(ADDRESS, timeout=TIMEOUT) as connection:
            with connection.makefile("rb") as reader:
                for i in range(1000):
                    command(connection, reader, requests[i % 100], expected)
                # The last client releases everyone and starts the shared clock.
                barrier.wait(timeout=TIMEOUT)
                for i in range(count):
                    start = time.perf_counter_ns()
                    command(connection, reader, requests[i % 100], expected)
                    samples.append(time.perf_counter_ns() - start)
                finished = time.perf_counter_ns()
        return samples, finished
    except Exception:
        barrier.abort()
        raise


def prepare(process):
    deadline = time.monotonic() + 10
    while True:
        if process.poll() is not None:
            raise RuntimeError("Server exited during startup")
        try:
            connection = socket.create_connection(ADDRESS, timeout=1)
            break
        except ConnectionRefusedError:
            if time.monotonic() >= deadline:
                raise RuntimeError("Server did not start within 10 seconds")
            time.sleep(0.02)
    with connection:
        connection.settimeout(TIMEOUT)
        with connection.makefile("rb") as reader:
            for i in range(100):
                command(connection, reader, f"SET k{i} {VALUE}\n".encode(), b"OK\n")


def scenario(server, workload, clients, operations):
    # Check availability before launching so we never benchmark someone else's server.
    with socket.socket() as probe:
        probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            probe.bind(ADDRESS)
        except OSError as error:
            raise RuntimeError("Port 6379 is occupied; stop the existing server first") from error

    with tempfile.TemporaryDirectory(prefix="kv-bench-") as directory:
        with tempfile.TemporaryFile(mode="w+") as output:
            process = subprocess.Popen(
                [str(server)], cwd=directory, stdout=output, stderr=output
            )
            try:
                prepare(process)
                clock = {}
                barrier = threading.Barrier(
                    clients, action=lambda: clock.update(start=time.perf_counter_ns())
                )
                with concurrent.futures.ThreadPoolExecutor(max_workers=clients) as pool:
                    jobs = [
                        pool.submit(client, workload, operations // clients, barrier)
                        for _ in range(clients)
                    ]
                    results = [job.result() for job in jobs]
                samples = sorted(sample for values, _ in results for sample in values)
                if len(samples) != operations:
                    raise RuntimeError("Successful command count does not match requested count")
                seconds = (max(end for _, end in results) - clock["start"]) / 1e9
                # Nearest-rank percentiles, converted from nanoseconds to microseconds.
                p50 = samples[math.ceil(len(samples) * 0.50) - 1] / 1000
                p99 = samples[math.ceil(len(samples) * 0.99) - 1] / 1000
                return operations / seconds, p50, p99
            except Exception:
                output.seek(0)
                print(output.read())
                raise
            finally:
                if process.poll() is None:
                    process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, default=Path("build/server"))
    parser.add_argument("--ops", type=int, default=50000, help="total measured commands per run")
    parser.add_argument("--repeats", type=int, default=3)
    args = parser.parse_args()
    if args.ops <= 0 or args.ops % 4 or args.repeats <= 0:
        parser.error("--ops must be positive and divisible by 4; --repeats must be positive")
    server = args.server.resolve()
    if not server.is_file():
        parser.error(f"Server executable does not exist: {server}")
    print(f"WAL directories: {tempfile.gettempdir()}/kv-bench-*", flush=True)
    rows = []
    for clients in (1, 4):
        for workload in ("GET", "SET"):
            runs = []
            for repeat in range(args.repeats):
                result = scenario(server, workload, clients, args.ops)
                runs.append(result)
                print(
                    f"{workload} clients={clients} run={repeat + 1}: "
                    f"{args.ops} successful, {result[0]:.0f} ops/s, "
                    f"p50={result[1]:.1f} us, p99={result[2]:.1f} us",
                    flush=True,
                )
            rows.append((workload, clients, *(statistics.median(v) for v in zip(*runs))))
    print("\n| Workload | Clients | Ops/s | p50 (us) | p99 (us) |")
    print("| --- | ---: | ---: | ---: | ---: |")
    for workload, clients, throughput, p50, p99 in rows:
        print(f"| {workload} | {clients} | {throughput:.0f} | {p50:.1f} | {p99:.1f} |")


if __name__ == "__main__":
    main()

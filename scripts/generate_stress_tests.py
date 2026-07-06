#!/usr/bin/env python3
"""Generate 120 test cases for stress_120 problem."""
import os

def solve(a, b):
    return a * b + a - b

tests_dir = os.path.join(os.path.dirname(__file__), "..", "problems", "stress_120", "tests")
os.makedirs(tests_dir, exist_ok=True)

cases = []
for i in range(120):
    a = (i * 17 + 3) % 1000 - 500
    b = (i * 13 + 7) % 1000 - 500
    cases.append((a, b))

for i, (a, b) in enumerate(cases):
    with open(os.path.join(tests_dir, f"{i}.in"), "w") as f:
        f.write(f"{a} {b}\n")
    with open(os.path.join(tests_dir, f"{i}.out"), "w") as f:
        f.write(f"{solve(a, b)}\n")

print(f"Generated {len(cases)} test cases in {tests_dir}")

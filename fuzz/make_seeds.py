#!/usr/bin/env python3
"""Regenerate fuzz/seeds/. Each seed is template-text + 0x00 + context-text.

Provenance for every seed is in the header comment of fuzz/fuzz_permuto.cpp.
Run from the repo root:  python3 fuzz/make_seeds.py
"""
import os

SEEDS = {
    # README.md "Quick Start / Basic Usage" + "Reverse Operations".
    "01-basic": (
        '{"user_id":"${/user/id}","user_name":"${/user/name}",'
        '"theme":"${/preferences/theme}","notifications":"${/preferences/notifications}"}',
        '{"user":{"id":123,"name":"Alice"},'
        '"preferences":{"theme":"dark","notifications":true}}',
    ),
    # README.md "Examples / API Payload Generation" (the OpenAI template).
    "02-api-payload": (
        '{"model":"${/config/model}","messages":[{"role":"user","content":"${/prompt}"}],'
        '"max_tokens":"${/config/max_tokens}","temperature":"${/config/temperature}"}',
        '{"config":{"model":"gpt-4","max_tokens":1000,"temperature":0.7},'
        '"prompt":"Explain quantum computing"}',
    ),
    # README.md "Examples / Configuration Templates" (redis dropped: interpolation).
    "03-nested-config": (
        '{"database":{"host":"${/env/DB_HOST}","port":"${/env/DB_PORT}",'
        '"name":"${/app/database_name}"},"logging":{"level":"${/app/log_level}"}}',
        '{"env":{"DB_HOST":"localhost","DB_PORT":5432},'
        '"app":{"database_name":"appdb","log_level":"info"}}',
    ),
    # REQUIREMENTS.md FR-3.1: "Empty path ${} refers to root context".
    "04-root-placeholder": (
        '"${}"',
        '42',
    ),
    # examples/mixed_mode_example.cpp, reduced to the fields its context provides.
    "05-mixed-mode": (
        '{"llm_api_request":{"temperature":"${/config/temperature}",'
        '"user_id":"${/user/id}","user_name":"${/user/name}"},'
        '"position":{"x":"${/coordinates/x}","y":"${/coordinates/y}"}}',
        '{"user":{"name":"Alice","id":123},"coordinates":{"x":10.5,"y":20.3},'
        '"config":{"temperature":0.7}}',
    ),
    # Deliberately invalid: truncated template. Seeds the parse-rejection path.
    "99-invalid-truncated": (
        '{"user_id":"${/user/id}"',
        '{"user":{"id":123}}',
    ),
}


def main() -> None:
    here = os.path.dirname(os.path.abspath(__file__))
    out = os.path.join(here, "seeds")
    os.makedirs(out, exist_ok=True)
    for name, (template, context) in SEEDS.items():
        blob = template.encode("utf-8") + b"\x00" + context.encode("utf-8")
        with open(os.path.join(out, name), "wb") as handle:
            handle.write(blob)
        print(f"{name}: {len(blob)} bytes")


if __name__ == "__main__":
    main()

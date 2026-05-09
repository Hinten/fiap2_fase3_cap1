# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project context

FIAP coursework — **Fase 3, Cap. 1**: the *CardioIA* IoT health-monitoring prototype. The assignment brief (in Portuguese) is in `enunciado.md` and is the source of truth for scope, deliverables, and grading. As of this writing the repo contains **only** that brief — no code has been committed yet.

The deliverable is a simulated wearable that:
1. Reads vital signs on an **ESP32 in Wokwi** (mandatory **DHT22** for temperature/humidity + one free-choice sensor — a button is suggested to simulate BPM).
2. Buffers readings locally for **offline resilience** (Edge). SPIFFS is volatile in Wokwi/PlatformIO, so the brief explicitly accepts the **Serial Monitor** as the offline-storage stand-in.
3. Publishes to a cloud broker via **MQTT** (HiveMQ Cloud is the suggested broker) when a simulated Wi-Fi boolean is `true`, then clears the local buffer.
4. Visualises in **Node-RED** (chart + gauge + threshold alert, e.g. BPM > 120 or temp > 38 °C); Grafana Cloud is optional.

Two optional "Ir Além" extensions: a Python REST + email-alert client, and an AI time-series notebook comparing logistic regression vs. a neuromorphic (LIF/FHN) model.

## Expected stack & layout

Nothing is scaffolded yet — when adding code, prefer creating these directories at the repo root rather than nesting under a single umbrella folder:

- `firmware/` (or `esp32/`) — Arduino/PlatformIO C++ for the ESP32 sketch (Wi-Fi sim flag, sensor reads, MQTT publish, offline buffer).
- `node-red/` — exported `flows.json` for the dashboard.
- `python/` (only if "Ir Além 1" is attempted) — REST client + email automation.
- `notebook/` (only if "Ir Além 2" is attempted) — Jupyter notebook + README + report.
- `docs/` or `relatorio/` — the required PDFs/markdown reports (Parte 1: ≥1 page on flow + resilience; Parte 2: ≥2 pages on MQTT + dashboard).

## Working notes

- **Language**: the brief, reports, and code comments are expected in **Portuguese (pt-BR)**. Match that tone when editing or generating documentation unless the user asks otherwise.
- **Wokwi limitation**: do not waste effort trying to make SPIFFS persist across simulator runs — the brief acknowledges it won't, and grading skips the SPIFFS criterion. Use an in-memory ring buffer (or Serial dump) for the resilience demo and document the chosen sample cap.
- **Grading is split 2/2/2/2/2** across: sensor reads, offline resilience, MQTT integration, dashboard + alerts, documentation. Keep changes balanced across these axes rather than over-investing in one.
- **No build tooling exists yet**. Once a PlatformIO project or Python venv is added, update this file with the actual build/test commands.

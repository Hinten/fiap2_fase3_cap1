"""
Servidor REST simulado — Flask
Lê o cenário padrão da variável de ambiente SCENARIO (definida no .env).

Rotas:
    GET  /health                 → health check para o Docker
    GET  /patients/<id>/vitals   → sinais vitais simulados
    POST /patients/<id>/alerts   → recebe e armazena alertas
    GET  /alerts                 → lista todos os alertas
"""

import os
import random
from datetime import datetime
from flask import Flask, jsonify, request

app = Flask(__name__)

# Cenário padrão vem do .env via Docker Compose
DEFAULT_SCENARIO = os.getenv("SCENARIO", "normal")

alert_log: list[dict] = []


def generate_vitals(scenario: str) -> dict:
    base = {
        "heart_rate": random.randint(60, 90),
        "temperature": round(random.uniform(36.0, 37.2), 1),
        "spo2": random.randint(95, 99),
        "systolic_bp": random.randint(100, 130),
        "seconds_without_movement": random.randint(0, 120),
        "timestamp": datetime.utcnow().isoformat(),
    }
    overrides = {
        "tachycardia": {"heart_rate": random.randint(105, 140)},
        "fever":       {"temperature": round(random.uniform(38.2, 40.0), 1)},
        "hypoxia":     {"spo2": random.randint(80, 91)},
        "no_movement": {"seconds_without_movement": random.randint(360, 900)},
        "critical": {
            "heart_rate": random.randint(110, 145),
            "temperature": round(random.uniform(38.5, 40.5), 1),
            "spo2": random.randint(78, 89),
            "seconds_without_movement": random.randint(400, 1000),
        },
    }
    base.update(overrides.get(scenario, {}))
    return base


@app.get("/health")
def health():
    return jsonify({"status": "ok"}), 200


@app.get("/patients/<patient_id>/vitals")
def get_vitals(patient_id: str):
    # query param sobrepõe o cenário padrão (útil para testes manuais)
    scenario = request.args.get("scenario", DEFAULT_SCENARIO)
    return jsonify(generate_vitals(scenario)), 200


@app.post("/patients/<patient_id>/alerts")
def receive_alert(patient_id: str):
    payload = request.get_json(force=True)
    payload["received_at"] = datetime.utcnow().isoformat()
    alert_log.append(payload)
    print(f"[API] Alerta registrado: {payload}", flush=True)
    return jsonify({"status": "ok", "total_alerts": len(alert_log)}), 201


@app.get("/alerts")
def list_alerts():
    return jsonify(alert_log), 200


if __name__ == "__main__":
    print(f"[API] Iniciando com cenário padrão: {DEFAULT_SCENARIO}", flush=True)
    app.run(host="0.0.0.0", port=8000)

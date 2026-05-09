"""
mock_publisher.py — Gerador de telemetria fake do CardioIA.

Publica no mesmo broker / mesmos tópicos / mesmo schema JSON que o ESP32
real. Permite validar a Parte 2 (Node-RED + Grafana) sem precisar do
firmware da Parte 1.

Cenários disponíveis (ver --help):
    normal          — bpm/temp dentro da faixa saudável
    taquicardia     — bpm sobe gradual de 80 -> 140 (alerta > 120)
    febre           — temp sobe de 37 -> 39 (alerta > 38)
    tudo            — taquicardia + febre simultâneos
    offline-flush   — rajada de 10 mensagens com buffered=true
    ruido           — 1% de payloads malformados (testa robustez do parser)

Uso:
    pip install -r requirements.txt
    cp .env.example .env  # editar com creds do HiveMQ
    python mock_publisher.py --scenario normal --duration 30
"""

from __future__ import annotations

import argparse
import json
import os
import random
import ssl
import sys
import time
from dataclasses import dataclass
from pathlib import Path

import paho.mqtt.client as mqtt
from dotenv import load_dotenv


# -----------------------------------------------------------------------------
# Config (env)
# -----------------------------------------------------------------------------
load_dotenv(Path(__file__).parent / ".env")


@dataclass
class Config:
    host: str
    port: int
    user: str
    password: str
    device_id: str

    @classmethod
    def from_env(cls) -> "Config":
        missing = [k for k in ("MQTT_HOST", "MQTT_USER", "MQTT_PASSWORD") if not os.getenv(k)]
        if missing:
            sys.exit(
                f"[erro] variáveis ausentes no .env: {', '.join(missing)}\n"
                f"       copie scripts/.env.example para scripts/.env e preencha."
            )
        return cls(
            host=os.environ["MQTT_HOST"],
            port=int(os.getenv("MQTT_PORT", "8883")),
            user=os.environ["MQTT_USER"],
            password=os.environ["MQTT_PASSWORD"],
            device_id=os.getenv("DEVICE_ID", "cardioia-01"),
        )


# -----------------------------------------------------------------------------
# Geradores de cenário (cada um é um iterador de payloads JSON-serializáveis)
# -----------------------------------------------------------------------------
def _base_payload(cfg: Config, *, temp: float, hum: float, bpm: int, buffered: bool = False) -> dict:
    return {
        "ts": int(time.time()),
        "deviceId": cfg.device_id,
        "temp": round(temp, 2),
        "hum": round(hum, 2),
        "bpm": int(bpm),
        "buffered": buffered,
    }


def gen_normal(cfg, total):
    for _ in range(total):
        yield _base_payload(
            cfg,
            temp=random.uniform(36.4, 37.0),
            hum=random.uniform(45, 60),
            bpm=random.randint(68, 88),
        )


def gen_taquicardia(cfg, total):
    """BPM cresce linearmente de 80 a 140 ao longo do envio."""
    for i in range(total):
        progress = i / max(total - 1, 1)
        bpm = 80 + int(progress * 60)  # 80..140
        yield _base_payload(
            cfg,
            temp=random.uniform(36.5, 37.0),
            hum=random.uniform(45, 60),
            bpm=bpm + random.randint(-2, 2),
        )


def gen_febre(cfg, total):
    """Temp cresce de 37.0 a 39.0."""
    for i in range(total):
        progress = i / max(total - 1, 1)
        temp = 37.0 + progress * 2.0
        yield _base_payload(
            cfg,
            temp=temp + random.uniform(-0.05, 0.05),
            hum=random.uniform(40, 55),
            bpm=random.randint(75, 95),
        )


def gen_tudo(cfg, total):
    for i in range(total):
        progress = i / max(total - 1, 1)
        yield _base_payload(
            cfg,
            temp=37.0 + progress * 2.0,
            hum=random.uniform(40, 55),
            bpm=80 + int(progress * 60),
        )


def gen_offline_flush(cfg, total):
    """Rajada de 10 leituras com buffered=true (timestamps no passado)."""
    burst = 10
    base_ts = int(time.time()) - burst * 5
    for i in range(burst):
        yield {
            "ts": base_ts + i * 5,
            "deviceId": cfg.device_id,
            "temp": round(random.uniform(36.4, 37.2), 2),
            "hum": round(random.uniform(45, 60), 2),
            "bpm": random.randint(70, 95),
            "buffered": True,
        }


def gen_ruido(cfg, total):
    """1% de mensagens malformadas misturadas com normais."""
    for i, p in enumerate(gen_normal(cfg, total)):
        if random.random() < 0.01:
            yield {"isso_eh_lixo": True, "no_schema": "x"}  # malformada
        else:
            yield p


SCENARIOS = {
    "normal": gen_normal,
    "taquicardia": gen_taquicardia,
    "febre": gen_febre,
    "tudo": gen_tudo,
    "offline-flush": gen_offline_flush,
    "ruido": gen_ruido,
}


# -----------------------------------------------------------------------------
# Conexão MQTT
# -----------------------------------------------------------------------------
def build_client(cfg: Config) -> mqtt.Client:
    client = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id=f"mock-{cfg.device_id}-{os.getpid()}",
    )
    client.username_pw_set(cfg.user, cfg.password)
    # tls_set() sem args usa o bundle de CAs padrão do sistema — funciona com
    # certificados Let's Encrypt do HiveMQ Cloud sem precisar baixar PEM.
    client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1_2)

    def on_connect(c, _userdata, _flags, reason_code, _props=None):
        if reason_code == 0 or (hasattr(reason_code, "is_failure") and not reason_code.is_failure):
            print(f"[mqtt] conectado em {cfg.host}:{cfg.port}")
        else:
            print(f"[mqtt] falha na conexão: rc={reason_code}", file=sys.stderr)

    client.on_connect = on_connect
    return client


# -----------------------------------------------------------------------------
# Loop principal
# -----------------------------------------------------------------------------
def run(scenario: str, rate_hz: float, duration_s: int) -> None:
    cfg = Config.from_env()
    if scenario not in SCENARIOS:
        sys.exit(f"[erro] cenário desconhecido: {scenario}. Disponíveis: {', '.join(SCENARIOS)}")

    client = build_client(cfg)
    client.connect(cfg.host, cfg.port, keepalive=30)
    client.loop_start()

    topic = f"cardioia/{cfg.device_id}/telemetry"
    interval = 1.0 / rate_hz if rate_hz > 0 else 1.0

    # offline-flush ignora --duration: sempre são 10 mensagens em rajada.
    total = 10 if scenario == "offline-flush" else max(int(rate_hz * duration_s), 1)

    print(f"[run] cenário={scenario} total={total} rate={rate_hz}hz topic={topic}")

    try:
        for i, payload in enumerate(SCENARIOS[scenario](cfg, total), start=1):
            body = json.dumps(payload)
            info = client.publish(topic, body, qos=1)
            info.wait_for_publish(timeout=5)
            print(f"  #{i}/{total} {body}")
            # offline-flush manda em rajada (sem sleep)
            if scenario != "offline-flush":
                time.sleep(interval)
    except KeyboardInterrupt:
        print("\n[run] interrompido pelo usuário")
    finally:
        client.loop_stop()
        client.disconnect()
        print("[run] fim.")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--scenario", required=True, choices=sorted(SCENARIOS.keys()))
    ap.add_argument("--rate", type=float, default=1.0, help="mensagens por segundo (default 1)")
    ap.add_argument("--duration", type=int, default=30, help="duração total em segundos (default 30)")
    args = ap.parse_args()
    run(args.scenario, args.rate, args.duration)


if __name__ == "__main__":
    main()

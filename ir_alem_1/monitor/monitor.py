"""
monitor.py — Sistema de Monitoramento de Sinais Vitais
Consome API REST, detecta riscos clínicos e envia alertas por e-mail.
Todas as configurações vêm de variáveis de ambiente (via config.py).
"""

import requests
import smtplib
import logging
import time
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from datetime import datetime
from shared.config import (
    API_BASE_URL, EMAIL_CONFIG, RISK_THRESHOLDS,
    CHECK_INTERVAL_SECONDS, PATIENT_ID
)

# ── Logging (terminal + arquivo em /app/logs/) ────────────────
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers=[
        logging.FileHandler("/app/logs/monitor.log"),
        logging.StreamHandler()
    ]
)
log = logging.getLogger(__name__)


# ─────────────────────────────────────────────
# 1. CLIENTE REST
# ─────────────────────────────────────────────

def fetch_vitals(patient_id: str) -> dict | None:
    url = f"{API_BASE_URL}/patients/{patient_id}/vitals"
    try:
        r = requests.get(url, timeout=5)
        r.raise_for_status()
        data = r.json()
        log.info(f"Sinais recebidos [{patient_id}]: {data}")
        return data
    except requests.exceptions.RequestException as e:
        log.error(f"Erro ao consultar API: {e}")
        return None


def post_alert_log(patient_id: str, alerts: list[str]) -> None:
    url = f"{API_BASE_URL}/patients/{patient_id}/alerts"
    payload = {
        "patient_id": patient_id,
        "timestamp": datetime.utcnow().isoformat(),
        "alerts": alerts,
    }
    try:
        r = requests.post(url, json=payload, timeout=5)
        r.raise_for_status()
        log.info("Alerta registrado na API.")
    except requests.exceptions.RequestException as e:
        log.error(f"Erro ao registrar alerta na API: {e}")


# ─────────────────────────────────────────────
# 2. DETECÇÃO DE RISCO
# ─────────────────────────────────────────────

def check_risks(vitals: dict) -> list[str]:
    alerts = []
    t = RISK_THRESHOLDS

    hr = vitals.get("heart_rate")
    if hr is not None:
        if hr > t["heart_rate_max"]:
            alerts.append(f"TAQUICARDIA: FC = {hr} bpm (limite: {t['heart_rate_max']} bpm)")
        elif hr < t["heart_rate_min"]:
            alerts.append(f"BRADICARDIA: FC = {hr} bpm (limite: {t['heart_rate_min']} bpm)")

    temp = vitals.get("temperature")
    if temp is not None:
        if temp > t["temp_max"]:
            alerts.append(f"FEBRE: {temp} °C (limite: {t['temp_max']} °C)")
        elif temp < t["temp_min"]:
            alerts.append(f"HIPOTERMIA: {temp} °C (limite: {t['temp_min']} °C)")

    spo2 = vitals.get("spo2")
    if spo2 is not None and spo2 < t["spo2_min"]:
        alerts.append(f"HIPÓXIA: SpO₂ = {spo2}% (limite: {t['spo2_min']}%)")

    sbp = vitals.get("systolic_bp")
    if sbp is not None:
        if sbp > t["systolic_bp_max"]:
            alerts.append(f"HIPERTENSÃO: PAS = {sbp} mmHg (limite: {t['systolic_bp_max']} mmHg)")
        elif sbp < t["systolic_bp_min"]:
            alerts.append(f"HIPOTENSÃO: PAS = {sbp} mmHg (limite: {t['systolic_bp_min']} mmHg)")

    secs = vitals.get("seconds_without_movement")
    if secs is not None and secs > t["no_movement_max_seconds"]:
        alerts.append(f"AUSÊNCIA DE MOVIMENTO: {secs // 60} min (limite: {t['no_movement_max_seconds'] // 60} min)")

    return alerts


# ─────────────────────────────────────────────
# 3. ENVIO DE E-MAIL
# ─────────────────────────────────────────────

def build_email_body(patient_id: str, vitals: dict, alerts: list[str]) -> str:
    lines = [
        f"⚠️  ALERTA DE MONITORAMENTO — Paciente {patient_id}",
        f"Data/hora: {datetime.now().strftime('%d/%m/%Y %H:%M:%S')}",
        "",
        "── SINAIS VITAIS ATUAIS ──────────────────────",
        f"  Frequência cardíaca : {vitals.get('heart_rate', 'N/A')} bpm",
        f"  Temperatura         : {vitals.get('temperature', 'N/A')} °C",
        f"  SpO₂                : {vitals.get('spo2', 'N/A')} %",
        f"  Pressão sistólica   : {vitals.get('systolic_bp', 'N/A')} mmHg",
        f"  Sem movimento há    : {vitals.get('seconds_without_movement', 'N/A')} s",
        "",
        "── ALERTAS DETECTADOS ────────────────────────",
    ]
    for a in alerts:
        lines.append(f"  • {a}")
    lines += [
        "",
        "Mensagem automática — Sistema de Monitoramento de Sinais Vitais.",
        "Verifique imediatamente o estado do paciente.",
    ]
    return "\n".join(lines)


def send_alert_email(patient_id: str, vitals: dict, alerts: list[str]) -> None:
    cfg = EMAIL_CONFIG
    if not cfg["sender"] or not cfg["password"]:
        log.warning("Credenciais de e-mail não configuradas — alerta não enviado.")
        return

    subject = f"[ALERTA] Sinais vitais críticos — Paciente {patient_id}"
    body = build_email_body(patient_id, vitals, alerts)

    msg = MIMEMultipart("alternative")
    msg["Subject"] = subject
    msg["From"] = cfg["sender"]
    msg["To"] = cfg["recipient"]
    msg.attach(MIMEText(body, "plain", "utf-8"))

    try:
        with smtplib.SMTP(cfg["smtp_host"], cfg["smtp_port"]) as server:
            server.ehlo()
            server.starttls()
            server.login(cfg["sender"], cfg["password"])
            server.sendmail(cfg["sender"], cfg["recipient"], msg.as_string())
        log.info(f"✅ E-mail enviado para {cfg['recipient']}")
    except smtplib.SMTPException as e:
        log.error(f"Falha ao enviar e-mail: {e}")


# ─────────────────────────────────────────────
# 4. LOOP PRINCIPAL
# ─────────────────────────────────────────────

def monitor_patient(patient_id: str):
    log.info(f"=== Monitoramento iniciado — Paciente {patient_id} ===")
    log.info(f"    API: {API_BASE_URL} | Intervalo: {CHECK_INTERVAL_SECONDS}s")

    while True:
        vitals = fetch_vitals(patient_id)

        if vitals:
            alerts = check_risks(vitals)
            if alerts:
                log.warning(f"🚨 {len(alerts)} alerta(s) detectado(s): {alerts}")
                post_alert_log(patient_id, alerts)
                send_alert_email(patient_id, vitals, alerts)
            else:
                log.info("✅ Sinais vitais normais.")
        else:
            log.warning("⚠️  Sem dados da API neste ciclo.")

        log.info(f"Próxima verificação em {CHECK_INTERVAL_SECONDS}s...")
        time.sleep(CHECK_INTERVAL_SECONDS)


if __name__ == "__main__":
    monitor_patient(PATIENT_ID)

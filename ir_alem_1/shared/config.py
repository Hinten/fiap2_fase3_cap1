"""
config.py — todas as configurações lidas de variáveis de ambiente.
Os valores vêm do .env via docker-compose.yml.
Nunca coloque senhas ou credenciais diretamente aqui.
"""

import os

# ── API ───────────────────────────────────────────────────────
API_BASE_URL = os.getenv("API_BASE_URL", "http://localhost:8000")

# ── Loop de monitoramento ─────────────────────────────────────
CHECK_INTERVAL_SECONDS = int(os.getenv("CHECK_INTERVAL_SECONDS", "30"))
PATIENT_ID = os.getenv("PATIENT_ID", "P001")

# ── Limiares clínicos ─────────────────────────────────────────
RISK_THRESHOLDS = {
    "heart_rate_max":         int(os.getenv("HEART_RATE_MAX", "100")),
    "heart_rate_min":         int(os.getenv("HEART_RATE_MIN", "50")),
    "temp_max":               float(os.getenv("TEMP_MAX", "37.8")),
    "temp_min":               float(os.getenv("TEMP_MIN", "35.0")),
    "spo2_min":               int(os.getenv("SPO2_MIN", "92")),
    "systolic_bp_max":        int(os.getenv("SYSTOLIC_BP_MAX", "140")),
    "systolic_bp_min":        int(os.getenv("SYSTOLIC_BP_MIN", "90")),
    "no_movement_max_seconds":int(os.getenv("NO_MOVEMENT_MAX_SECONDS", "300")),
}

# ── E-mail ────────────────────────────────────────────────────
EMAIL_CONFIG = {
    "smtp_host": os.getenv("SMTP_HOST", "smtp.gmail.com"),
    "smtp_port": int(os.getenv("SMTP_PORT", "587")),
    "sender":    os.getenv("EMAIL_SENDER", ""),
    "password":  os.getenv("EMAIL_PASSWORD", ""),
    "recipient": os.getenv("EMAIL_RECIPIENT", ""),
}

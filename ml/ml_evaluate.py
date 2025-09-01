# ml_evaluate.py — оценка качества модели по последним прогнозам
# ---------------------------------------------------------------
# Сравнивает predicted_alarm с vibration_alarm,
# вычисляет Precision, Recall, F1 и сохраняет в таблицу model_metrics.

import psycopg2
import pandas as pd
from sklearn.metrics import precision_score, recall_score, f1_score
from datetime import datetime

# --- Конфигурация PostgreSQL ---
db_config = {
    "dbname": "postgres",
    "user": "postgres",
    "password": "postgres",
    "host": "localhost",
    "port": 5432
}

# --- Подключение ---
conn = psycopg2.connect(**db_config)
conn.autocommit = True
cur = conn.cursor()

# --- Чтение последних 200 прогнозов ---
query = """
    SELECT p.ts, p.predicted_alarm, s.vibration_alarm
    FROM sensor_predictions p
    JOIN sensor_data2 s ON p.ts = s.ts
    ORDER BY p.ts DESC
    LIMIT 200
"""
df = pd.read_sql(query, conn)

# --- Метрики ---
y_true = df["vibration_alarm"].astype(int)
y_pred = df["predicted_alarm"].astype(int)

precision = precision_score(y_true, y_pred, zero_division=0)
recall = recall_score(y_true, y_pred, zero_division=0)
f1 = f1_score(y_true, y_pred, zero_division=0)
ts = datetime.now()

print(f"📊 Precision: {precision:.3f}  Recall: {recall:.3f}  F1: {f1:.3f}")

# --- Таблица для метрик ---
cur.execute("""
    CREATE TABLE IF NOT EXISTS public.model_metrics (
        ts TIMESTAMPTZ NOT NULL,
        precision DOUBLE PRECISION,
        recall DOUBLE PRECISION,
        f1 DOUBLE PRECISION
    )
""")

# --- Запись ---
cur.execute("""
    INSERT INTO public.model_metrics (ts, precision, recall, f1)
    VALUES (%s, %s, %s, %s)
""", (ts, precision, recall, f1))

cur.close()
conn.close()
print("✅ Метрики сохранены в model_metrics")

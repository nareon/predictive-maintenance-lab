# predictor.py — онлайн-прогноз тревог по текущим метрикам
# ---------------------------------------------------------
# Загружает модель из ml_train.py,
# читает последние данные из sensor_data2,
# делает прогноз vibration_alarm,
# записывает результат в таблицу sensor_predictions.

import psycopg2
import pandas as pd
import joblib
from datetime import datetime

# --- Конфигурация PostgreSQL ---
db_config = {
    "dbname": "postgres",
    "user": "postgres",
    "password": "postgres",  # <<< замени при необходимости
    "host": "localhost",
    "port": 5432
}

# --- Загрузка модели ---
model = joblib.load("ml/model_rf_alarm.pkl")
print("✅ Модель загружена")

# --- Подключение к БД ---
conn = psycopg2.connect(**db_config)
conn.autocommit = True
cur = conn.cursor()

# --- Чтение последних данных ---
query = """
    SELECT ts, vibration, temperature, pressure
    FROM public.sensor_data2
    ORDER BY ts DESC
    LIMIT 1
"""
df = pd.read_sql(query, conn)
latest = df.iloc[0]
ts = latest["ts"]
features = latest[["vibration", "temperature", "pressure"]].values.reshape(1, -1)

# --- Прогноз ---
pred = model.predict(features)[0]
print(f"📡 Прогноз на {ts}: тревога = {bool(pred)}")

# --- Запись в таблицу sensor_predictions ---
cur.execute("""
    CREATE TABLE IF NOT EXISTS public.sensor_predictions (
        ts TIMESTAMPTZ NOT NULL,
        vibration DOUBLE PRECISION,
        temperature DOUBLE PRECISION,
        pressure DOUBLE PRECISION,
        predicted_alarm BOOLEAN
    )
""")

cur.execute("""
    INSERT INTO public.sensor_predictions (ts, vibration, temperature, pressure, predicted_alarm)
    VALUES (%s, %s, %s, %s, %s)
""", (ts, latest["vibration"], latest["temperature"], latest["pressure"], bool(pred)))

cur.close()
conn.close()
print("✅ Результат записан в sensor_predictions")

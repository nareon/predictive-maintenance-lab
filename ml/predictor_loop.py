# predictor_loop.py — потоковый прогноз тревог на основе текущих метрик
# ---------------------------------------------------------------------
# Загружает модель, периодически читает данные из sensor_data2,
# делает прогноз vibration_alarm и записывает результат в sensor_predictions.

import psycopg2
import pandas as pd
import joblib
from time import sleep

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

# --- Создание таблицы для предсказаний (если нет) ---
cur.execute("""
    CREATE TABLE IF NOT EXISTS public.sensor_predictions (
        ts TIMESTAMPTZ NOT NULL,
        vibration DOUBLE PRECISION,
        temperature DOUBLE PRECISION,
        pressure DOUBLE PRECISION,
        predicted_alarm BOOLEAN
    )
""")

# --- Основной цикл ---
print("🔁 Запуск потокового прогнозирования...")
last_ts = None

try:
    while True:
        # --- Чтение последней строки ---
        df = pd.read_sql("""
            SELECT ts, vibration, temperature, pressure
            FROM public.sensor_data2
            ORDER BY ts DESC
            LIMIT 1
        """, conn)

        if df.empty:
            print("⚠️ Нет данных в sensor_data2")
            sleep(5)
            continue

        row = df.iloc[0]
        ts = row["ts"]

        # --- Проверка: уже обработано ---
        if ts == last_ts:
            sleep(2)
            continue
        last_ts = ts

        # --- Прогноз ---
        features = row[["vibration", "temperature", "pressure"]].values.reshape(1, -1)
        pred = model.predict(features)[0]
        print(f"[{ts}] Прогноз тревоги: {bool(pred)}")

        # --- Запись результата ---
        cur.execute("""
            INSERT INTO public.sensor_predictions (ts, vibration, temperature, pressure, predicted_alarm)
            VALUES (%s, %s, %s, %s, %s)
        """, (ts, row["vibration"], row["temperature"], row["pressure"], bool(pred)))

        sleep(2)

except KeyboardInterrupt:
    print("⏹ Остановлено пользователем")

finally:
    cur.close()
    conn.close()
    print("🔌 Соединение закрыто")

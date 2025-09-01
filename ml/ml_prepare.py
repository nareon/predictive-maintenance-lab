# ml_prepare.py — подготовка данных для ML-модели
# ------------------------------------------------
# Извлекает данные из PostgreSQL (sensor_data2),
# формирует pandas.DataFrame с временным индексом
# и сохраняет в CSV и Parquet для обучения.

import psycopg2
import pandas as pd

# --- Конфигурация подключения к PostgreSQL ---
db_config = {
    "dbname": "postgres",
    "user": "postgres",
    "password": "postgres",
    "host": "localhost",
    "port": 5432
}

# --- SQL-запрос для извлечения данных ---
query = """
    SELECT ts, vibration, temperature, pressure, vibration_alarm
    FROM public.sensor_data2
    ORDER BY ts
"""

# --- Подключение и извлечение ---
print("📡 Подключение к PostgreSQL...")
conn = psycopg2.connect(**db_config)
df = pd.read_sql(query, conn)
conn.close()
print(f"✅ Загружено {len(df)} строк")

# --- Преобразование: временной индекс ---
df['ts'] = pd.to_datetime(df['ts'])
df.set_index('ts', inplace=True)

# --- Сохранение в CSV ---
csv_path = "data/sensor_data.csv"
df.to_csv(csv_path)
print(f"📁 Сохранено в CSV: {csv_path}")

# --- Сохранение в Parquet ---
parquet_path = "data/sensor_data.parquet"
df.to_parquet(parquet_path)
print(f"📦 Сохранено в Parquet: {parquet_path}")

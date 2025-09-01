# 
# reader4.py — OPC UA клиент для dynamic4.c
# -----------------------------------------
# Читает параметры оборудования:
#   - Bearing_Vibration_mm_s
#   - Temperature_C
#   - Pressure_bar
#   - Bearing_Alarm
# и записывает их в таблицу PostgreSQL: public.sensor_data2
#

from opcua import Client
from time import sleep
import psycopg2
from datetime import datetime

# --- Конфигурация OPC UA ---
opc_url = "opc.tcp://localhost:4840"

# --- Конфигурация PostgreSQL ---
db_config = {
    "dbname": "postgres",    # имя базы данных
    "user": "postgres",      # пользователь
    "password": "postgres",  # <<< замени на свой пароль
    "host": "localhost",
    "port": 5432
}

# --- Подключение к OPC UA серверу ---
client = Client(opc_url)
client.connect()
print("✅ Подключено к OPC UA серверу:", opc_url)

# --- Подключение к PostgreSQL ---
conn = psycopg2.connect(**db_config)
conn.autocommit = True
cur = conn.cursor()
print("✅ Подключено к PostgreSQL")

try:
    # --- Получаем узлы по NodeId ---
    vib_node   = client.get_node("ns=1;s=equipment.bearing.vibration")
    temp_node  = client.get_node("ns=1;s=equipment.temperature")
    press_node = client.get_node("ns=1;s=equipment.pressure")
    alarm_node = client.get_node("ns=1;s=equipment.bearing.alarm")

    # --- Цикл сбора данных ---
    print("📡 Начинаем сбор метрик от dynamic4.c...")
    for i in range(10000):  # 1000 итераций по 2 сек = ~33 минуты
        ts = datetime.now()
        vibration   = vib_node.get_value()
        temperature = temp_node.get_value()
        pressure    = press_node.get_value()
        alarm       = alarm_node.get_value()

        print(f"[{ts}] Vib: {vibration:.2f}  Temp: {temperature:.1f}  Press: {pressure:.3f}  Alarm: {alarm}")

        # --- Запись в таблицу ---
        cur.execute("""
            INSERT INTO public.sensor_data2 (ts, vibration, temperature, pressure, vibration_alarm)
            VALUES (%s, %s, %s, %s, %s)
        """, (ts, vibration, temperature, pressure, alarm))

        sleep(2)

finally:
    client.disconnect()
    cur.close()
    conn.close()
    print("🔌 Соединения закрыты")

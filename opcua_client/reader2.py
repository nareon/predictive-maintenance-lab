from opcua import Client
from time import sleep

# --- Адрес OPC UA сервера ---
url = "opc.tcp://localhost:4840"

# --- Создаём клиента и подключаемся ---
client = Client(url)
client.connect()
print("✅ Подключено к серверу:", url)

try:
    # --- Доступ к переменным по NodeId ---
    vib_node   = client.get_node("ns=1;s=equipment.bearing.vibration")  # вибрация подшипника
    temp_node  = client.get_node("ns=1;s=equipment.temperature")        # температура
    press_node = client.get_node("ns=1;s=equipment.pressure")           # давление
    alarm_node = client.get_node("ns=1;s=equipment.bearing.alarm")      # тревожный флаг

    # --- Читаем начальные значения ---
    print("Initial Bearing_Vibration_mm_s:", vib_node.get_value())
    print("Initial Temperature_C:", temp_node.get_value())
    print("Initial Pressure_bar:", press_node.get_value())
    print("Initial Bearing_Alarm:", alarm_node.get_value())

    # --- Пример: сброс вибрации до 1.0 мм/с (имитация ремонта) ---
    vib_node.set_value(1.0)
    print("🔄 Bearing_Vibration_mm_s сброшен на:", vib_node.get_value())

    # --- Цикл чтения значений ---
    for i in range(5):
        print(f"\n--- Измерение {i+1} ---")
        print("Bearing_Vibration_mm_s:", vib_node.get_value())
        print("Temperature_C:", temp_node.get_value())
        print("Pressure_bar:", press_node.get_value())
        print("Bearing_Alarm:", alarm_node.get_value())
        sleep(2)

finally:
    client.disconnect()
    print("🔌 Отключено от сервера")

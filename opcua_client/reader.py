from opcua import Client
from time import sleep

# Адрес твоего OPC UA сервера
url = "opc.tcp://localhost:4840"

# Создаём клиента и подключаемся
client = Client(url)
client.connect()
print("✅ Подключено к серверу:", url)

try:
    # Получаем объекты сервера (корневой каталог объектов)
    objects = client.get_objects_node()

    # Доступ к переменным по NodeId (ns=1 — наше пространство имён)
    eq_param   = client.get_node("ns=1;s=equipment.parameter")    # деградирующий параметр
    temp_param = client.get_node("ns=1;s=equipment.temperature")  # температура
    press_param= client.get_node("ns=1;s=equipment.pressure")     # давление

    # Читаем начальные значения
    print("Initial EquipmentParameter:", eq_param.get_value())
    print("Initial Temperature:", temp_param.get_value())
    print("Initial Pressure:", press_param.get_value())

    # Пример: меняем значение деградирующего параметра на 100 (имитация ремонта)
    eq_param.set_value(100.0)
    print("🔄 EquipmentParameter сброшен на:", eq_param.get_value())

    # Цикл чтения значений каждые 2 сек
    for i in range(5):
        print(f"\n--- Измерение {i+1} ---")
        print("EquipmentParameter:", eq_param.get_value())
        print("Temperature:", temp_param.get_value())
        print("Pressure:", press_param.get_value())
        sleep(2)

finally:
    client.disconnect()
    print("🔌 Отключено от сервера")

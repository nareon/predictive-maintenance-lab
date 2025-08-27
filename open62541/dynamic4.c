/*
 * dynamic4.c — OPC UA Сервер: генерация телеметрии с синусоидой, шумом и аномалиями
 * ---------------------------------------------------------------------------------
 * Моделирует параметры промышленного оборудования:
 *   1. Bearing_Vibration_mm_s — вибрация подшипника (мм/с), плавные колебания с шумом и редкими пиками.
 *   2. Temperature_C          — температура (°C), синусоида + шум + редкие перегревы.
 *   3. Pressure_bar            — давление (бар), плавные колебания с шумом.
 *
 * Дополнительно:
 *   — Логический флаг Bearing_Alarm = true при вибрации >= 7.0 мм/с, иначе false.
 *   — Обновление каждые 100 мс для имитации реального потока данных (10 Гц).
 *
 * Основан на предыдущей версии сервера (вибрация/температура/давление + тревога)
 * с доработкой генерации сигналов в стиле скрипта gen.c.
 *
 * Сборка:
 *   gcc dynamic4.c -lopen62541 -lm -o servers/dynamic4
 *
 * Запуск:
 *   servers/dynamic4
 *   OPC UA Endpoint: opc.tcp://localhost:4840
 *
 * Подключение клиента: UAExpert, Python (opcua.Client) или SCADA.
 */
#include <open62541/plugin/log_stdout.h>     // Лог в stdout
#include <open62541/server.h>                // Основное API сервера OPC UA
#include <open62541/server_config_default.h> // Быстрая конфигурация сервера
#include <signal.h>                          // Обработка Ctrl+C (SIGINT)
#include <math.h>                            // sin()
#include <stdlib.h>                          // rand()
#include <time.h>                            // time()

// === Глобальные переменные для управления работой ===
static UA_Boolean running = true;      // Флаг работы сервера
static UA_NodeId node_vib, node_temp, node_press, node_alarm;

static UA_Double vib_value   = 1.2;    // мм/с — вибрация подшипника
static UA_Double temp_value  = 60.0;   // °C — температура
static UA_Double press_value = 1.0;    // бар — давление
static UA_Boolean alarm_state = false; // тревога по вибрации

// Случайное число [0..1)
static double frand(void) { return (double)rand() / (double)RAND_MAX; }

// Обработчик SIGINT
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "⏹ Завершение работы сервера");
    running = false;
}

// Колбэк обновления значений (каждые 100 мс)
static void update_cb(UA_Server *server, void *data) {
    static double t = 0; // "время" для генерации синусов
    t += 0.1;            // шаг времени

    UA_Variant val;

    // --- 1. Вибрация ---
    vib_value = 2.0 + 0.5 * sin(t * 3.1) + 0.2 * (frand() - 0.5); // базовая модель
    if (frand() < 0.001) vib_value += 5.0;                        // редкий скачок
    if (vib_value < 0) vib_value = 0;
    if (vib_value > 15.0) vib_value = 15.0;
    UA_Variant_setScalar(&val, &vib_value, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, node_vib, val);

    // Порог тревоги по вибрации (>= 7 мм/с)
    UA_Boolean newAlarm = (vib_value >= 7.0);
    if (newAlarm != alarm_state) {
        alarm_state = newAlarm;
        UA_Variant alarmVal;
        UA_Variant_setScalar(&alarmVal, &alarm_state, &UA_TYPES[UA_TYPES_BOOLEAN]);
        UA_Server_writeValue(server, node_alarm, alarmVal);
    }

    // --- 2. Температура ---
    temp_value = 60.0 + 10.0 * sin(t * 0.1) + 0.5 * (frand() - 0.5);
    if (frand() < 0.001) temp_value += 20.0; // редкий перегрев
    UA_Variant_setScalar(&val, &temp_value, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, node_temp, val);

    // --- 3. Давление ---
    press_value = 1.0 + 0.1 * sin(t * 0.7) + 0.02 * (frand() - 0.5);
    UA_Variant_setScalar(&val, &press_value, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, node_press, val);

    // Лог в консоль
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Vib: %.2f мм/с  Temp: %.1f°C  Press: %.3f бар  Alarm: %s",
                vib_value, temp_value, press_value, alarm_state ? "ON" : "OFF");
}

int main(void) {
    signal(SIGINT, stopHandler);
    srand(time(NULL));

    // Создаём сервер и конфигурацию по умолчанию
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // Общие атрибуты для параметров типа Double
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    // 1. Вибрация
    UA_Variant_setScalar(&attr.value, &vib_value, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Bearing_Vibration_mm_s");
    attr.description = UA_LOCALIZEDTEXT("ru-RU", "Скорость вибрации подшипника, мм/с");
    node_vib = UA_NODEID_STRING(1, "equipment.bearing.vibration");
    UA_Server_addVariableNode(server, node_vib,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Bearing_Vibration_mm_s"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);

    // 2. Температура
    UA_Variant_setScalar(&attr.value, &temp_value, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Temperature_C");
    attr.description = UA_LOCALIZEDTEXT("ru-RU", "Температура оборудования, °C");
    node_temp = UA_NODEID_STRING(1, "equipment.temperature");
    UA_Server_addVariableNode(server, node_temp,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Temperature_C"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);

    // 3. Давление
    UA_Variant_setScalar(&attr.value, &press_value, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Pressure_bar");
    attr.description = UA_LOCALIZEDTEXT("ru-RU", "Давление в системе, бар");
    node_press = UA_NODEID_STRING(1, "equipment.pressure");
    UA_Server_addVariableNode(server, node_press,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Pressure_bar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);

    // 4. Флаг тревоги (Boolean)
    UA_VariableAttributes attrAlarm = UA_VariableAttributes_default;
    attrAlarm.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    attrAlarm.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_Variant_setScalar(&attrAlarm.value, &alarm_state, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attrAlarm.displayName = UA_LOCALIZEDTEXT("en-US", "Bearing_Alarm");
    attrAlarm.description = UA_LOCALIZEDTEXT("ru-RU", "Тревога по вибрации подшипника");
    node_alarm = UA_NODEID_STRING(1, "equipment.bearing.alarm");
    UA_Server_addVariableNode(server, node_alarm,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Bearing_Alarm"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attrAlarm, NULL, NULL);

    // Регистрируем обновление каждые 100 мс (как в gen.c)
    UA_Server_addRepeatedCallback(server, update_cb, NULL, 100, NULL);

    UA_Server_run(server, &running);
    UA_Server_delete(server);
    return 0;
}

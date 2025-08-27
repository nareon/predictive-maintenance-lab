/* 
 * Пример OPC UA сервера на базе open62541.
 * Моделируются параметры оборудования: вибрация подшипника, температура, давление.
 * Реализован флаг тревоги по превышению порога вибрации.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

static UA_Boolean running = true;
static UA_NodeId varNodeId_vib;    // Вибрация подшипника
static UA_NodeId varNodeId_temp;   // Температура
static UA_NodeId varNodeId_press;  // Давление
static UA_NodeId varNodeId_alarm;  // Тревожный флаг

static UA_Double bearingVibration = 1.2; // мм/с
static UA_Boolean alarmState = false;
static double t = 0.0;

// --- Обработчик Ctrl+C ---
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Сервер завершает работу");
    running = false;
}

static void updateParameters(UA_Server *server, void *data) {
    UA_Variant curVal;
    UA_Variant_init(&curVal);

    // 1. Вибрация: рост + шум
    UA_Server_readValue(server, varNodeId_vib, &curVal);
    if (UA_Variant_hasScalarType(&curVal, &UA_TYPES[UA_TYPES_DOUBLE])) {
        bearingVibration = *(UA_Double *)curVal.data;
        bearingVibration += 0.02 + ((rand() % 11) - 5) / 500.0; // ±0.01 случайно
        if (bearingVibration > 10.0) bearingVibration = 10.0;
        if (bearingVibration < 0) bearingVibration = 0;

        UA_Variant_setScalar(&curVal, &bearingVibration, &UA_TYPES[UA_TYPES_DOUBLE]);
        UA_Server_writeValue(server, varNodeId_vib, curVal);

        // === Проверка порога ===
        UA_Boolean newAlarmState = (bearingVibration >= 7.0);
        if (newAlarmState != alarmState) {
            alarmState = newAlarmState;
            UA_Variant alarmVal;
            UA_Variant_setScalar(&alarmVal, &alarmState, &UA_TYPES[UA_TYPES_BOOLEAN]);
            UA_Server_writeValue(server, varNodeId_alarm, alarmVal);
        }
    }

    // 2. Температура
    t += 0.1;
    UA_Double tempValue = 25.0 + 5.0 * sin(t);
    UA_Variant_setScalar(&curVal, &tempValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, varNodeId_temp, curVal);

    // 3. Давление
    double rnd = (rand() % 2001 - 1000) / 10000.0;
    UA_Double pressureValue = 1.0 + rnd;
    UA_Variant_setScalar(&curVal, &pressureValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, varNodeId_press, curVal);

    // Лог
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Vib: %.2f мм/с  Temp: %.2f°C  Press: %.3f бар  Alarm: %s",
                bearingVibration, tempValue, pressureValue,
                alarmState ? "ON" : "OFF");
}

int main(void) {
    signal(SIGINT, stopHandler);
    srand(time(NULL));

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_VariableAttributes attr = UA_VariableAttributes_default;

    // --- Вибрация ---
    UA_Variant_setScalar(&attr.value, &bearingVibration, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Bearing_Vibration_mm_s");
    attr.description = UA_LOCALIZEDTEXT("ru-RU", "Скорость вибрации подшипника, мм/с");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    varNodeId_vib = UA_NODEID_STRING(1, "equipment.bearing.vibration");
    UA_Server_addVariableNode(server, varNodeId_vib,
                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                               UA_QUALIFIEDNAME(1, "Bearing_Vibration_mm_s"),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                               attr, NULL, NULL);

    // --- Температура ---
    UA_Double tempInit = 25.0;
    UA_Variant_setScalar(&attr.value, &tempInit, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Temperature_C");
    attr.description = UA_LOCALIZEDTEXT("ru-RU", "Температура оборудования, °C");
    varNodeId_temp = UA_NODEID_STRING(1, "equipment.temperature");
    UA_Server_addVariableNode(server, varNodeId_temp,
                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                               UA_QUALIFIEDNAME(1, "Temperature_C"),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                               attr, NULL, NULL);

    // --- Давление ---
    UA_Double pressInit = 1.0;
    UA_Variant_setScalar(&attr.value, &pressInit, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Pressure_bar");
    attr.description = UA_LOCALIZEDTEXT("ru-RU", "Давление в системе, бар");
    varNodeId_press = UA_NODEID_STRING(1, "equipment.pressure");
    UA_Server_addVariableNode(server, varNodeId_press,
                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                               UA_QUALIFIEDNAME(1, "Pressure_bar"),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                               attr, NULL, NULL);

    // --- Тревога ---
    UA_Variant_setScalar(&attr.value, &alarmState, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Bearing_Alarm");
    attr.description = UA_LOCALIZEDTEXT("ru-RU", "Тревога по вибрации подшипника");
    attr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    varNodeId_alarm = UA_NODEID_STRING(1, "equipment.bearing.alarm");
    UA_Server_addVariableNode(server, varNodeId_alarm,
                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                               UA_QUALIFIEDNAME(1, "Bearing_Alarm"),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                               attr, NULL, NULL);

    // --- Колбэк ---
    UA_Server_addRepeatedCallback(server, updateParameters, NULL, 1000, NULL);

    // --- Запуск ---
    UA_Server_run(server, &running);
    UA_Server_delete(server);
    return 0;
}

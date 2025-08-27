#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

// --- Глобальные переменные ---
static UA_Boolean running = true;
static UA_NodeId varNodeId_vib;   // Вибрация подшипника
static UA_NodeId varNodeId_temp;  // Температура
static UA_NodeId varNodeId_press; // Давление

static UA_Double bearingVibration = 1.2; // мм/с — начальное значение вибрации
static double t = 0.0;                    // “время” для синусоиды температуры

// Обработчик Ctrl+C
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Сервер завершает работу");
    running = false;
}

// Колбэк обновления параметров (каждую секунду)
static void updateParameters(UA_Server *server, void *data) {
    UA_Variant curVal;
    UA_Variant_init(&curVal);

    // === 1. Вибрация подшипника ===
    UA_Server_readValue(server, varNodeId_vib, &curVal);
    if (UA_Variant_hasScalarType(&curVal, &UA_TYPES[UA_TYPES_DOUBLE])) {
        bearingVibration = *(UA_Double *)curVal.data;
        // Эмулируем рост вибрации из-за износа: +0.02 мм/с
        bearingVibration += 0.02;
        // Ограничим сверху (аварийный порог)
        if (bearingVibration > 10.0) bearingVibration = 10.0;
        UA_Variant_setScalar(&curVal, &bearingVibration, &UA_TYPES[UA_TYPES_DOUBLE]);
        UA_Server_writeValue(server, varNodeId_vib, curVal);
    }

    // === 2. Температура (синус 20..30 °C) ===
    t += 0.1;
    UA_Double tempValue = 25.0 + 5.0 * sin(t);
    UA_Variant_setScalar(&curVal, &tempValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, varNodeId_temp, curVal);

    // === 3. Давление (0.9..1.1 бар) ===
    double rnd = (rand() % 2001 - 1000) / 10000.0;
    UA_Double pressureValue = 1.0 + rnd;
    UA_Variant_setScalar(&curVal, &pressureValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, varNodeId_press, curVal);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Vibration: %.2f мм/с  Temp: %.2f °C  Press: %.3f бар",
                bearingVibration, tempValue, pressureValue);
}

int main(void) {
    signal(SIGINT, stopHandler);
    srand(time(NULL));

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // ==== Вибрация подшипника ====
    UA_VariableAttributes attr = UA_VariableAttributes_default;
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

    // ==== Температура ====
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

    // ==== Давление ====
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

    // Регистрируем колбэк
    UA_Server_addRepeatedCallback(server, updateParameters, NULL, 1000, NULL);

    // Запуск сервера
    UA_Server_run(server, &running);

    // Завершение
    UA_Server_delete(server);
    return 0;
}

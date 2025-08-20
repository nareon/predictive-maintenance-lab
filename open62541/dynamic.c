#include <open62541/plugin/log_stdout.h>     // Плагин логгера — позволяет выводить сообщения в консоль
#include <open62541/server.h>                // Основные определения и структуры для OPC UA сервера
#include <open62541/server_config_default.h> // Быстрая настройка сервера с конфигурацией по умолчанию
#include <signal.h>                          // Для перехвата сигнала Ctrl+C (SIGINT) и завершения работы
#include <math.h>                            // Для функции sin() — будет использоваться в симуляции температуры
#include <stdlib.h>                          // Для rand() — генерация случайных колебаний давления
#include <time.h>                            // Для инициализации генератора случайных чисел и измерения времени

// --- Глобальные переменные ---
static UA_Boolean running = true;       // Флаг работы сервера — false приведёт к завершению цикла
static UA_NodeId varNodeId_degrade;     // NodeId переменной, имитирующей деградацию
static UA_NodeId varNodeId_temp;        // NodeId переменной температуры
static UA_NodeId varNodeId_press;       // NodeId переменной давления
static UA_Double parameterValue = 100.0;// Начальное значение деградирующего параметра (в процентах)
static double t = 0.0;                  // "Время" для вычисления синусоиды температуры

// --- Обработчик сигнала Ctrl+C ---
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Сервер завершает работу"); // Выводим в лог
    running = false;                         // Останавливаем главный цикл
}

// --- Колбэк, вызываемый каждые 1000 мс (1 секунда) ---
static void updateParameters(UA_Server *server, void *data) {
    // ===== 1. Обновление деградирующего параметра =====
    UA_Variant curVal;
    UA_Variant_init(&curVal);

    // Читаем текущее значение из узла (учитываем, что клиент мог его изменить)
    UA_Server_readValue(server, varNodeId_degrade, &curVal);

    // Проверяем, что тип — Double (число с плавающей точкой)
    if (UA_Variant_hasScalarType(&curVal, &UA_TYPES[UA_TYPES_DOUBLE])) {
        parameterValue = *(UA_Double *)curVal.data; // Копируем в нашу переменную
        parameterValue -= 0.5;                      // Уменьшаем на 0.5% за секунду
        if (parameterValue < 0) parameterValue = 0; // Не даём уйти в минус

        // Записываем новое значение обратно в узел
        UA_Variant_setScalar(&curVal, &parameterValue, &UA_TYPES[UA_TYPES_DOUBLE]);
        UA_Server_writeValue(server, varNodeId_degrade, curVal);
    }

    // ===== 2. Температура: синусоида от 20 до 30 °C =====
    t += 0.1; // шаг "времени" для плавных изменений
    UA_Double tempValue = 25.0 + 5.0 * sin(t);
    UA_Variant_setScalar(&curVal, &tempValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, varNodeId_temp, curVal);

    // ===== 3. Давление: случайное колебание 0.9..1.1 бар =====
    double rnd = (rand() % 2001 - 1000) / 10000.0; // диапазон -0.1..+0.1
    UA_Double pressureValue = 1.0 + rnd;
    UA_Variant_setScalar(&curVal, &pressureValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, varNodeId_press, curVal);

    // Выводим все значения в консоль для наблюдения
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Deg: %.1f %%  Temp: %.2f °C  Press: %.3f бар",
                parameterValue, tempValue, pressureValue);
}

int main(void) {
    // Привязываем обработчик сигнала
    signal(SIGINT, stopHandler);
    // Инициализация генератора случайных чисел
    srand(time(NULL));

    // Создаём сервер и загружаем конфигурацию по умолчанию (порт 4840, стандартный endpoint)
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // === Создание переменной 1: деградирующий параметр ===
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &parameterValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "EquipmentParameter");
    attr.description = UA_LOCALIZEDTEXT("en-US", "Degrading parameter (%)");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE; // Чтение+запись
    varNodeId_degrade = UA_NODEID_STRING(1, "equipment.parameter");
    UA_Server_addVariableNode(server, varNodeId_degrade,
                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                               UA_QUALIFIEDNAME(1, "EquipmentParameter"),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                               attr, NULL, NULL);

    // === Переменная 2: температура ===
    UA_Double tempInit = 25.0;
    UA_Variant_setScalar(&attr.value, &tempInit, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Temperature");
    attr.description = UA_LOCALIZEDTEXT("en-US", "Simulated temperature (°C)");
    varNodeId_temp = UA_NODEID_STRING(1, "equipment.temperature");
    UA_Server_addVariableNode(server, varNodeId_temp,
                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                               UA_QUALIFIEDNAME(1, "Temperature"),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                               attr, NULL, NULL);

    // === Переменная 3: давление ===
    UA_Double pressInit = 1.0;
    UA_Variant_setScalar(&attr.value, &pressInit, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Pressure");
    attr.description = UA_LOCALIZEDTEXT("en-US", "Simulated pressure (bar)");
    varNodeId_press = UA_NODEID_STRING(1, "equipment.pressure");
    UA_Server_addVariableNode(server, varNodeId_press,
                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                               UA_QUALIFIEDNAME(1, "Pressure"),
                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                               attr, NULL, NULL);

    // Регистрируем колбэк — будет вызываться каждые 1000 мс
    UA_Server_addRepeatedCallback(server, updateParameters, NULL, 1000, NULL);

    // Запускаем основной цикл сервера
    UA_Server_run(server, &running);

    // Завершаем работу
    UA_Server_delete(server);
    return 0;
}

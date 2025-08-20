#include <open62541/plugin/log_stdout.h>     // Для UA_Log_Stdout и макросов логирования
#include <open62541/server.h>                // Основные определения сервера OPC UA
#include <open62541/server_config_default.h> // Быстрая настройка сервера по умолчанию
#include <signal.h>                          // Для обработки сигнала Ctrl+C (остановка)

static UA_Boolean running = true; // Глобальный флаг — пока true, сервер крутится

// Обработчик Ctrl+C — вызывается при нажатии в терминале
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Получен сигнал завершения (Ctrl+C) — останавливаем сервер");
    running = false; // Меняем флаг, чтобы выйти из UA_Server_run
}

int main(void) {
    // Привязываем нашу функцию к сигналу SIGINT (Ctrl+C)
    signal(SIGINT, stopHandler);

    // === 1. Создаём сервер и применяем конфигурацию по умолчанию ===
    UA_Server *server = UA_Server_new();
    // Конфигурация по умолчанию: порт 4840, endpoint opc.tcp://localhost:4840
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // === 2. Настраиваем атрибуты переменной ===
    UA_VariableAttributes attr = UA_VariableAttributes_default; // Значения по умолчанию

    UA_Double myValue = 42.0; // Начальное значение переменной (например, температура)
    // Привязываем это значение к атрибутам (UA_Variant может хранить любой тип OPC UA)
    UA_Variant_setScalar(&attr.value, &myValue, &UA_TYPES[UA_TYPES_DOUBLE]);

    attr.displayName = UA_LOCALIZEDTEXT("ru-RU", "МояПеременная"); // Имя, отображаемое в клиенте
    attr.description = UA_LOCALIZEDTEXT("ru-RU", "Тестовая переменная типа Double"); // Описание
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId; // Тип данных — Double (вещественное число)
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE; // Чтение+запись

    // === 3. Добавляем переменную в адресное пространство сервера ===
    /*
       Адресное пространство (упрощённо):
       Root
        └─ Objects (UA_NS0ID_OBJECTSFOLDER)
             └─ МояПеременная (наш новый узел)
    */
    UA_NodeId newNodeId = UA_NODEID_STRING(1, "my.variable"); // Уникальный ID в пространстве имён 1
    UA_QualifiedName newName = UA_QUALIFIEDNAME(1, "МояПеременная");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); // Родитель — Objects
    UA_NodeId parentRefNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);  // Тип связи ORGANIZES

    UA_Server_addVariableNode(server,
                              newNodeId,
                              parentNodeId,
                              parentRefNodeId,
                              newName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), // Базовый тип переменной
                              attr,
                              NULL, NULL); // Нет пользовательских callback'ов на чтение/запись

    // === 4. Запуск основного цикла сервера ===
    // Этот вызов блокирует выполнение, пока running == true
    UA_Server_run(server, &running);

    // === 5. Завершение работы ===
    UA_Server_delete(server); // Освобождаем ресурсы
    return 0;
}

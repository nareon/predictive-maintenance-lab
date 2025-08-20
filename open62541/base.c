#include <open62541/plugin/log_stdout.h>     // Для UA_Log_Stdout и макросов логирования
#include <open62541/server.h>               // Основной API сервера OPC UA
#include <open62541/server_config_default.h>// Функция конфигурации сервера "по умолчанию"
#include <signal.h>                         // Для обработки сигнала прерывания (Ctrl+C)

// Глобальная переменная-флаг: пока = UA_TRUE — сервер работает
static UA_Boolean running = UA_TRUE;

/*
 * Обработчик сигнала SIGINT (срабатывает при Ctrl+C в терминале).
 * Логирует сообщение и изменяет флаг running на UA_FALSE.
 * Это даёт знать основному циклу, что пора завершаться.
 */
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received Ctrl-C");
    running = UA_FALSE;
}

int main(void) {
    // Привязываем обработчик stopHandler к сигналу Ctrl+C
    signal(SIGINT, stopHandler);

    // === 1. Создание экземпляра сервера ===
    UA_Server *server = UA_Server_new(); // Выделяем память и инициализируем структуру сервера

    // === 2. Применение конфигурации по умолчанию ===
    // Настраивает сервер на стандартный endpoint (opc.tcp://localhost:4840)
    // и базовые параметры (логирование, транспорт, безопасность по умолчанию)
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // === 3. Запуск цикла работы сервера ===
    // UA_Server_run блокирует выполнение и обрабатывает входящие запросы
    // до тех пор, пока *running = UA_TRUE.
    // Параметр &running — это указатель на флаг, который мы можем изменить извне (в stopHandler).
    UA_StatusCode retval = UA_Server_run(server, &running);

    // === 4. Освобождение ресурсов и завершение ===
    UA_Server_delete(server); // Чистим память, освобождаем ресурсы

    // Возвращаем код завершения (0 — успех, иначе код ошибки)
    return (int)retval;
}


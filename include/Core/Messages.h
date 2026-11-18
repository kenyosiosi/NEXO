#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>

namespace nexo {

// Idiomas soportados por el motor. ES es el default histórico: los mensajes
// nacen en español y la suite de tests los verifica así.
enum class Lang { ES, EN };

Lang lang();
void setLang(Lang l);

// Catálogo de mensajes del motor. Cada entrada guarda su variante ES y EN;
// los marcadores {0}..{3} se sustituyen por los argumentos pasados a msg().
enum class Msg {
    // --- QueryExecutor -----------------------------------------------------
    EmptyQuery,
    CollectionExists,      // {0} = nombre
    InvalidName,
    CollectionCreated,     // {0} = nombre
    CollectionNotExists,   // {0} = nombre
    SchemaNotFound,        // {0} = nombre
    DefineReport,          // {0} = nombre, {1} = campos, {2} = siguiente id, {3} = índices
    IndexExists,           // {0} = campo
    IndexCreated,          // {0} = campo
    CollectionDropped,     // {0} = nombre
    ScanEmpty,             // {0} = nombre
    ScanHeader,            // {0} = cantidad
    RecordInserted,        // {0} = id
    GetNeedsIndex,         // {0} = nombre
    FoundViaIndex,         // {0} = campo
    GetNotFound,           // {0} = campo, {1} = valor
    DeleteNeedsId,
    DeleteOk,              // {0} = id
    DeleteNotFound,
    UpdateNeedsId,
    UpdateNotFound,
    AlreadyDeleted,
    UpdateOk,
    UnsupportedOp,         // {0} = operación
    // --- StorageManager ----------------------------------------------------
    RecordTooBig,          // {0} = bytes del registro, {1} = máximo por página
    // --- Métricas (STATS) --------------------------------------------------
    MetricsTitle,          // {0} = nombre
    MetricsLive,           // {0} = vivos
    MetricsDead,           // {0} = sucios
    MetricsFrag,           // {0} = %
    MetricsSize,           // {0} = bytes
    MetricsAlert,
};

std::string msg(Msg id);
std::string msg(Msg id, const std::string& a0);
std::string msg(Msg id, const std::string& a0, const std::string& a1);
std::string msg(Msg id, const std::string& a0, const std::string& a1, const std::string& a2);
std::string msg(Msg id, const std::string& a0, const std::string& a1, const std::string& a2, const std::string& a3);

} // namespace nexo

#endif // MESSAGES_H

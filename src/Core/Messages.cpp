#include "Core/Messages.h"
#include <array>

namespace nexo {
namespace {

Lang g_lang = Lang::ES;

struct Entry { const char* es; const char* en; };

// Tabla indexada por Msg. ES es el idioma de origen del proyecto.
const std::array<Entry, 31> kCatalog = {{
    /* EmptyQuery          */ {"[Error] Consulta vacía o mal formada.",
                              "[Error] Empty or malformed query."},
    /* CollectionExists    */ {"[Error] La colección '{0}' ya existe.",
                              "[Error] Collection '{0}' already exists."},
    /* InvalidName         */ {"[Error] Nombre de colección inválido: usa solo letras, números, '_' o '-'.",
                              "[Error] Invalid collection name: use only letters, numbers, '_' or '-'."},
    /* CollectionCreated   */ {"Colección '{0}' creada con éxito. Índice 'id' autogenerado.",
                              "Collection '{0}' created successfully. 'id' index auto-generated."},
    /* CollectionNotExists */ {"[Error] La colección '{0}' no existe.",
                              "[Error] Collection '{0}' does not exist."},
    /* SchemaNotFound      */ {"[Error] No se encontró el esquema de '{0}'.",
                              "[Error] No schema found for '{0}'."},
    /* DefineReport        */ {"Estructura de '{0}':\nCampos: {1}\nSiguiente ID: {2}\nÍndices Activos: [{3}]",
                              "Structure of '{0}':\nFields: {1}\nNext ID: {2}\nActive Indexes: [{3}]"},
    /* IndexExists         */ {"[Error] El índice para '{0}' ya existe.",
                              "[Error] Index for '{0}' already exists."},
    /* IndexCreated        */ {"Índice secundario sobre '{0}' creado exitosamente.",
                              "Secondary index on '{0}' created successfully."},
    /* CollectionDropped   */ {"Colección '{0}' eliminada por completo.",
                              "Collection '{0}' fully dropped."},
    /* ScanEmpty           */ {"La colección '{0}' está vacía.",
                              "Collection '{0}' is empty."},
    /* ScanHeader          */ {"{0} documentos encontrados:\n",
                              "{0} documents found:\n"},
    /* RecordInserted      */ {"Ok. Registro insertado exitosamente con ID: {0}",
                              "Ok. Record successfully inserted with ID: {0}"},
    /* GetNeedsIndex       */ {"[Error] No se encontró un índice para los campos proporcionados. "
                              "Crea uno con: INDEX \"campo\" ON \"{0}\"",
                              "[Error] No index found for the provided fields. "
                              "Create one with: INDEX \"field\" ON \"{0}\""},
    /* FoundViaIndex       */ {"Documento encontrado (vía índice '{0}'):\n",
                              "Document found (via '{0}' index):\n"},
    /* GetNotFound         */ {"Documento no encontrado con {0}: '{1}'.",
                              "Document not found with {0}: '{1}'."},
    /* DeleteNeedsId       */ {"[Error] DELETE requiere especificar el 'id'.",
                              "[Error] DELETE requires the 'id' field."},
    /* DeleteOk            */ {"Documento (ID: {0}) eliminado lógicamente.",
                              "Document (ID: {0}) logically deleted."},
    /* DeleteNotFound      */ {"No se encontró el documento para eliminar.",
                              "Document to delete was not found."},
    /* UpdateNeedsId       */ {"[Error] UPDATE requiere un 'id' válido.",
                              "[Error] UPDATE requires a valid 'id'."},
    /* UpdateNotFound      */ {"No se encontró el documento a modificar.",
                              "Document to update was not found."},
    /* AlreadyDeleted      */ {"El documento ya fue eliminado previamente.",
                              "The document was already deleted."},
    /* UpdateOk            */ {"Documento actualizado correctamente.",
                              "Document updated successfully."},
    /* UnsupportedOp       */ {"[Error] Operación no soportada: {0}",
                              "[Error] Unsupported operation: {0}"},
    /* RecordTooBig        */ {"El registro ({0} bytes) excede el máximo por página ({1} bytes).",
                              "The record ({0} bytes) exceeds the per-page maximum ({1} bytes)."},
    /* MetricsTitle        */ {"\n--- MÉTRICAS DE {0} ---\n",
                              "\n--- METRICS FOR {0} ---\n"},
    /* MetricsLive         */ {" Registros Vivos:    {0}\n",
                              " Live Records:    {0}\n"},
    /* MetricsDead         */ {" Registros Sucios:   {0}\n",
                              " Dead Records:   {0}\n"},
    /* MetricsFrag         */ {" Fragmentación:      {0} %\n",
                              " Fragmentation:      {0} %\n"},
    /* MetricsSize         */ {" Tamaño en disco:    {0} bytes",
                              " On-disk size:    {0} bytes"},
    /* MetricsAlert        */ {"\n[ALERTA] Índice de fragmentación crítico. Sugerencia: COMPACT.",
                              "\n[ALERTA] Critical fragmentation level. Suggestion: COMPACT."},
}};

const Entry& entry(Msg id) {
    const size_t i = static_cast<size_t>(id);
    return (i < kCatalog.size()) ? kCatalog[i] : kCatalog[0];
}

std::string pick(const Entry& e) { return (g_lang == Lang::ES) ? e.es : e.en; }

// Sustituye {0}..{9} por los argumentos dados, en orden.
std::string interpolate(const std::string& tpl, const std::string* args, size_t n) {
    std::string out;
    for (size_t i = 0; i < tpl.size();) {
        if (tpl[i] == '{' && i + 2 < tpl.size() &&
            tpl[i + 1] >= '0' && tpl[i + 1] <= '9' && tpl[i + 2] == '}') {
            const int idx = tpl[i + 1] - '0';
            if (idx < static_cast<int>(n)) { out += args[idx]; i += 3; continue; }
        }
        out += tpl[i++];
    }
    return out;
}

} // namespace

Lang lang() { return g_lang; }
void setLang(Lang l) { g_lang = l; }

std::string msg(Msg id) { return pick(entry(id)); }

std::string msg(Msg id, const std::string& a0) {
    const std::string args[] = {a0};
    return interpolate(msg(id), args, 1);
}

std::string msg(Msg id, const std::string& a0, const std::string& a1) {
    const std::string args[] = {a0, a1};
    return interpolate(msg(id), args, 2);
}

std::string msg(Msg id, const std::string& a0, const std::string& a1, const std::string& a2) {
    const std::string args[] = {a0, a1, a2};
    return interpolate(msg(id), args, 3);
}

std::string msg(Msg id, const std::string& a0, const std::string& a1, const std::string& a2, const std::string& a3) {
    const std::string args[] = {a0, a1, a2, a3};
    return interpolate(msg(id), args, 4);
}

} // namespace nexo

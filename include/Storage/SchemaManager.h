#ifndef SCHEMA_MANAGER_H
#define SCHEMA_MANAGER_H

#include <string>
#include <vector>

// Metadatos de una colección, persistidos en data/<coleccion>.schema:
// línea 1 = siguiente id de la secuencia, línea 2 = lista de campos.
struct SchemaInfo {
    int nextId = 1;
    std::string fields;                   // lista cruda, ej: "id,nombre,edad"
    std::vector<std::string> fieldsList;  // campos del usuario, sin "id"
};

// Un esquema no puede contener separadores de ruta ni delimitadores propios.
bool validateSchemaFields(const std::string& fields);

// El nombre de una colección se convierte en nombre de archivo: solo se
// aceptan [A-Za-z0-9_-] para evitar rutas travesadas tipo "../algo".
bool validateCollectionName(const std::string& name);

// Lee data/<coleccion>.schema; si no existe devuelve un esquema por defecto.
SchemaInfo loadSchema(const std::string& tableName);

// Escribe data/<coleccion>.schema, creando el directorio data/ si falta.
void saveSchema(const std::string& tableName, int nextId, const std::string& fields);

#endif // SCHEMA_MANAGER_H

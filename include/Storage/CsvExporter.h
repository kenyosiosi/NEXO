#ifndef CSV_EXPORTER_H
#define CSV_EXPORTER_H

#include <map>
#include <string>
#include <vector>

/**
 * Exportación de colecciones a CSV.
 *
 * Vive en el motor (no en la UI) para poder probarla sin ventana: recibe los
 * registros ya materializados y devuelve el texto completo.
 */
namespace csv {

// Escapa un valor según RFC 4180: si contiene coma, comilla o salto de línea
// se rodea de comillas dobles y cada comilla interna se duplica.
std::string escape(const std::string& value);

// Cabecera en el orden de `fields` más una fila por registro. Un campo que el
// registro no tenga se exporta vacío; el resto se ignora.
std::string build(const std::vector<std::map<std::string, std::string>>& records,
                  const std::vector<std::string>& fields);

} // namespace csv

#endif

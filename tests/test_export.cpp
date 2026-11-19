#include <doctest/doctest.h>

#include "Storage/CsvExporter.h"

#include <string>
#include <vector>

namespace {

std::vector<std::map<std::string, std::string>> rows() {
    return {
        {{"id", "1"}, {"nombre", "Ana"}, {"ciudad", "Lima"}},
        {{"id", "2"}, {"nombre", "Luis"}, {"ciudad", "Bogotá"}},
    };
}

} // namespace

// ---------------------------------------------------------------------------
// CSV: escape y armado del documento
// ---------------------------------------------------------------------------
TEST_CASE("CSV: valores simples salen sin comillas") {
    CHECK(csv::escape("Ana") == "Ana");
    CHECK(csv::escape("") == "");
    CHECK(csv::escape("Bogotá") == "Bogotá");
}

TEST_CASE("CSV: comas, comillas y saltos de línea se entrecomillan") {
    CHECK(csv::escape("Pérez, Ana") == "\"Pérez, Ana\"");
    CHECK(csv::escape("dice \"hola\"") == "\"dice \"\"hola\"\"\"");
    CHECK(csv::escape("linea1\nlinea2") == "\"linea1\nlinea2\"");
    CHECK(csv::escape("retorno\r") == "\"retorno\r\"");
    // Un campo solo con comillas también necesita comillas externas.
    CHECK(csv::escape("\"") == "\"\"\"\"");
}

TEST_CASE("CSV: cabecera y filas respetan el orden de los campos") {
    const std::vector<std::string> fields = {"id", "nombre", "ciudad"};
    CHECK(csv::build(rows(), fields) ==
          "id,nombre,ciudad\n"
          "1,Ana,Lima\n"
          "2,Luis,Bogotá\n");
}

TEST_CASE("CSV: los campos ausentes se exportan vacíos") {
    std::vector<std::map<std::string, std::string>> data = {
        {{"id", "1"}, {"nombre", "Ana"}},
    };
    CHECK(csv::build(data, {"id", "nombre", "ciudad"}) ==
          "id,nombre,ciudad\n"
          "1,Ana,\n");
}

TEST_CASE("CSV: una colección vacía solo lleva la cabecera") {
    CHECK(csv::build({}, {"id", "nombre"}) == "id,nombre\n");
}

TEST_CASE("CSV: los valores peligrosos no rompen las columnas") {
    std::vector<std::map<std::string, std::string>> data = {
        {{"id", "1"}, {"nombre", "Ana, la jefa"}},
    };
    const std::string out = csv::build(data, {"id", "nombre"});
    CHECK(out == "id,nombre\n"
                 "1,\"Ana, la jefa\"\n");
}

#include <doctest/doctest.h>

#include "Core/Messages.h"

#include <string>

using namespace nexo;

// ---------------------------------------------------------------------------
// Messages: catálogo bilingüe del motor
// ---------------------------------------------------------------------------
TEST_CASE("Messages: ES es el idioma por defecto") {
    setLang(Lang::ES);

    CHECK(lang() == Lang::ES);
    CHECK(msg(Msg::CollectionCreated, "usuarios").find("creada") != std::string::npos);
    CHECK(msg(Msg::CollectionNotExists, "fantasma").find("no existe") != std::string::npos);
    CHECK(msg(Msg::RecordInserted, "3").find("exitosamente") != std::string::npos);
    CHECK(msg(Msg::DeleteOk, "1").find("eliminado") != std::string::npos);
    CHECK(msg(Msg::InvalidName).find("[Error]") != std::string::npos);
}

TEST_CASE("Messages: interpolación de marcadores {n}") {
    setLang(Lang::ES);
    CHECK(msg(Msg::GetNotFound, "id", "9") == "Documento no encontrado con id: '9'.");
    CHECK(msg(Msg::ScanHeader, "7").find("7") != std::string::npos);
    // Un marcador sin argumento se deja literal en vez de romper la salida.
    CHECK(msg(Msg::RecordInserted).find("{0}") != std::string::npos);
}

TEST_CASE("Messages: el catálogo EN cambia el texto y se restaura ES") {
    setLang(Lang::ES);
    const std::string es = msg(Msg::CollectionCreated, "usuarios");

    setLang(Lang::EN);
    CHECK(lang() == Lang::EN);
    const std::string en = msg(Msg::CollectionCreated, "usuarios");
    CHECK(en.find("created") != std::string::npos);
    CHECK(en != es);

    CHECK(msg(Msg::ScanEmpty, "usuarios").find("empty") != std::string::npos);
    CHECK(msg(Msg::UnsupportedOp, "FOO").find("Unsupported") != std::string::npos);

    // Restaurar el default para el resto de la suite (los tests del motor
    // verifican los mensajes en español).
    setLang(Lang::ES);
    CHECK(lang() == Lang::ES);
    CHECK(msg(Msg::CollectionCreated, "usuarios") == es);
}

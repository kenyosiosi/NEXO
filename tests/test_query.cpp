#include <doctest/doctest.h>

#include "query_engine/QueryExecutor.h"
#include "query_engine/parser.h"
#include "query_engine/tokenizer.h"
#include "Storage/StorageManager.h"
#include "B-tree/IndexManager.h"

#include <filesystem>
#include <map>
#include <string>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Parser / Tokenizer
// ---------------------------------------------------------------------------
TEST_CASE("Tokenizer: commands become typed tokens") {
    Tokenizer tz("INSERT INTO \"usuarios\" {\"id\": \"1\"}");
    const std::vector<Token> tokens = tz.tokenize();

    REQUIRE(tokens.size() >= 6);
    CHECK(tokens[0].type == TokenType::INSERT);
    CHECK(tokens[1].type == TokenType::INTO);
    CHECK(tokens[2].type == TokenType::STRING);
    CHECK(tokens[2].lexeme == "usuarios");
    CHECK(tokens[3].type == TokenType::LBRACE);
    CHECK(tokens.back().type == TokenType::END_OF_FILE);
}

TEST_CASE("Parser: INSERT shape lands in the AST") {
    const std::map<std::string, std::string> ast =
        Parser(Tokenizer("INSERT INTO \"usuarios\" {\"nombre\": \"Keny\", \"edad\": \"21\"}").tokenize()).parse();

    CHECK(ast.at("operation") == "INSERT");
    CHECK(ast.at("_table_target") == "usuarios");
    CHECK(ast.at("nombre") == "Keny");
    CHECK(ast.at("edad") == "21");
}

TEST_CASE("Parser: UPDATE splits SET and WHERE payloads") {
    const std::map<std::string, std::string> ast =
        Parser(Tokenizer("UPDATE \"usuarios\" SET {\"edad\": \"23\"} WHERE {\"id\": \"1\"}").tokenize()).parse();

    CHECK(ast.at("operation") == "UPDATE");
    CHECK(ast.at("_table_target") == "usuarios");
    CHECK(ast.at("_set_edad") == "23");
    CHECK(ast.at("id") == "1");
}

TEST_CASE("Parser: single-word commands carry the target") {
    CHECK(Parser(Tokenizer("SCAN \"usuarios\"").tokenize()).parse().at("operation") == "SCAN");
    CHECK(Parser(Tokenizer("DROP \"usuarios\"").tokenize()).parse().at("operation") == "DROP");
    CHECK(Parser(Tokenizer("STATS \"usuarios\"").tokenize()).parse().at("operation") == "STATS");
    CHECK(Parser(Tokenizer("DEFINE \"usuarios\"").tokenize()).parse().at("operation") == "DEFINE");
}

TEST_CASE("Parser: INDEX declares field and target") {
    const std::map<std::string, std::string> ast =
        Parser(Tokenizer("INDEX \"nombre\" ON \"usuarios\"").tokenize()).parse();

    CHECK(ast.at("operation") == "INDEX");
    CHECK(ast.at("field") == "nombre");
    CHECK(ast.at("_table_target") == "usuarios");
}

TEST_CASE("Parser: malformed commands are rejected with an error") {
    CHECK_THROWS(Parser(Tokenizer("FROBNICATE \"x\"").tokenize()).parse());
    CHECK_THROWS(Parser(Tokenizer("INSERT INTO").tokenize()).parse());
    CHECK_THROWS(Parser(Tokenizer("SCAN").tokenize()).parse());
}

// ---------------------------------------------------------------------------
// QueryExecutor end-to-end
// ---------------------------------------------------------------------------
class EngineFixture {
public:
    EngineFixture() {
        std::error_code ec;
        fs::remove_all("data", ec);
    }
    ~EngineFixture() {
        for (auto& [name, mgr] : storage) delete mgr;
        for (auto& [table, fields] : indexes)
            for (auto& [field, idx] : fields) delete idx;
        std::error_code ec;
        fs::remove_all("data", ec);
    }

    std::string run(const std::string& command) {
        Tokenizer tz(command);
        Parser parser(tz.tokenize());
        return executor.execute(parser.parse());
    }

    std::map<std::string, StorageManager*> storage;
    std::map<std::string, std::map<std::string, IndexManager*>> indexes;
    std::map<std::string, size_t> dirty;
    QueryExecutor executor{ storage, indexes, dirty };
};

TEST_CASE("QueryExecutor: CREATE prepares collection, schema and id index") {
    EngineFixture f;
    const std::string out = f.run("CREATE \"usuarios\" {fields: \"nombre,edad\"}");
    CHECK(out.find("creada") != std::string::npos);
    CHECK(f.storage.count("usuarios") == 1);
    CHECK(f.indexes.at("usuarios").count("id") == 1);
    CHECK(fs::exists("data/usuarios.schema"));

    CHECK(f.run("CREATE \"usuarios\" {fields: \"x\"}").find("ya existe") != std::string::npos);
}

TEST_CASE("QueryExecutor: INSERT, SCAN and GET flow through the index") {
    EngineFixture f;
    f.run("CREATE \"usuarios\" {fields: \"nombre,edad\"}");
    CHECK(f.run("INSERT INTO \"usuarios\" {\"nombre\": \"Keny\", \"edad\": \"21\"}").find("ID: 1") != std::string::npos);
    CHECK(f.run("INSERT INTO \"usuarios\" {\"nombre\": \"Rafa\", \"edad\": \"22\"}").find("ID: 2") != std::string::npos);

    CHECK(f.run("SCAN \"usuarios\"").find("2 documentos") != std::string::npos);

    const std::string got = f.run("GET FROM \"usuarios\" {\"id\": \"1\"}");
    CHECK(got.find("Keny") != std::string::npos);
    CHECK(got.find("21") != std::string::npos);

    CHECK(f.run("GET FROM \"usuarios\" {\"id\": \"999\"}").find("no encontrado") != std::string::npos);
}

TEST_CASE("QueryExecutor: GET requires an index over the queried field") {
    EngineFixture f;
    f.run("CREATE \"usuarios\" {fields: \"nombre\"}");
    f.run("INSERT INTO \"usuarios\" {\"nombre\": \"Keny\"}");

    const std::string out = f.run("GET FROM \"usuarios\" {\"nombre\": \"Keny\"}");
    CHECK(out.find("[Error]") != std::string::npos);
    CHECK(out.find("INDEX") != std::string::npos);
}

TEST_CASE("QueryExecutor: secondary indexes are built from existing data") {
    EngineFixture f;
    f.run("CREATE \"usuarios\" {fields: \"nombre\"}");
    f.run("INSERT INTO \"usuarios\" {\"nombre\": \"Keny\"}");
    f.run("INSERT INTO \"usuarios\" {\"nombre\": \"Rafa\"}");

    const std::string out = f.run("INDEX \"nombre\" ON \"usuarios\"");
    CHECK(out.find("exitosamente") != std::string::npos);
    CHECK(f.run("GET FROM \"usuarios\" {\"nombre\": \"Rafa\"}").find("Rafa") != std::string::npos);
}

TEST_CASE("QueryExecutor: DELETE tombstones and SCAN hides the record") {
    EngineFixture f;
    f.run("CREATE \"usuarios\" {fields: \"nombre\"}");
    f.run("INSERT INTO \"usuarios\" {\"nombre\": \"Keny\"}");
    f.run("INSERT INTO \"usuarios\" {\"nombre\": \"Rafa\"}");

    CHECK(f.run("DELETE FROM \"usuarios\" WHERE {\"id\": \"1\"}").find("eliminado") != std::string::npos);
    CHECK(f.run("SCAN \"usuarios\"").find("1 documentos") != std::string::npos);
}

TEST_CASE("QueryExecutor: UPDATE rewrites the document") {
    EngineFixture f;
    f.run("CREATE \"usuarios\" {fields: \"nombre,edad\"}");
    f.run("INSERT INTO \"usuarios\" {\"nombre\": \"Keny\", \"edad\": \"21\"}");

    CHECK(f.run("UPDATE \"usuarios\" SET {\"edad\": \"22\"} WHERE {\"id\": \"1\"}").find("correctamente") != std::string::npos);
    CHECK(f.run("SCAN \"usuarios\"").find("\"22\"") != std::string::npos);
}

TEST_CASE("QueryExecutor: reads never return tombstoned records after UPDATE") {
    // UPDATE aplica borrado lógico + reinserta, dejando entradas de índice
    // viejas. GET/UPDATE/DELETE deben resolver siempre el registro vivo.
    EngineFixture f;
    f.run("CREATE \"usuarios\" {fields: \"nombre,edad\"}");
    f.run("INSERT INTO \"usuarios\" {\"nombre\": \"Keny\", \"edad\": \"21\"}");
    f.run("UPDATE \"usuarios\" SET {\"edad\": \"22\"} WHERE {\"id\": \"1\"}");

    const std::string got = f.run("GET FROM \"usuarios\" {\"id\": \"1\"}");
    CHECK(got.find("\"22\"") != std::string::npos);
    CHECK(got.find("\"21\"") == std::string::npos);

    // Segunda actualización sobre el registro ya movido.
    CHECK(f.run("UPDATE \"usuarios\" SET {\"edad\": \"23\"} WHERE {\"id\": \"1\"}").find("correctamente") != std::string::npos);
    CHECK(f.run("GET FROM \"usuarios\" {\"id\": \"1\"}").find("\"23\"") != std::string::npos);

    // El borrado usa también el índice: debe eliminar el registro vivo.
    CHECK(f.run("DELETE FROM \"usuarios\" WHERE {\"id\": \"1\"}").find("eliminado") != std::string::npos);
    CHECK(f.run("SCAN \"usuarios\"").find("vacía") != std::string::npos);
}

TEST_CASE("QueryExecutor: secondary indexes resolve the live record too") {
    EngineFixture f;
    f.run("CREATE \"usuarios\" {fields: \"nombre\"}");
    f.run("INSERT INTO \"usuarios\" {\"nombre\": \"Keny\"}");
    f.run("INDEX \"nombre\" ON \"usuarios\"");
    f.run("UPDATE \"usuarios\" SET {\"nombre\": \"Keny Jr.\"} WHERE {\"id\": \"1\"}");

    // La clave vieja sigue en el índice (se purga con COMPACT), pero la
    // búsqueda no debe devolver el registro muerto.
    CHECK(f.run("GET FROM \"usuarios\" {\"nombre\": \"Keny\"}").find("no encontrado") != std::string::npos);
    CHECK(f.run("GET FROM \"usuarios\" {\"nombre\": \"Keny Jr.\"}").find("Keny Jr.") != std::string::npos);
}

TEST_CASE("QueryExecutor: STATS reports fragmentation and DROP removes everything") {
    EngineFixture f;
    f.run("CREATE \"usuarios\" {fields: \"nombre\"}");
    f.run("INSERT INTO \"usuarios\" {\"nombre\": \"Keny\"}");
    f.run("DELETE FROM \"usuarios\" WHERE {\"id\": \"1\"}");

    const std::string stats = f.run("STATS \"usuarios\"");
    CHECK(stats.find("Registros Vivos") != std::string::npos);
    CHECK(stats.find("Fragmentaci") != std::string::npos);

    CHECK(f.run("DROP \"usuarios\"").find("eliminada") != std::string::npos);
    CHECK(f.storage.count("usuarios") == 0);
    CHECK_FALSE(fs::exists("data/usuarios.bin"));
    CHECK_FALSE(fs::exists("data/usuarios.schema"));
}

TEST_CASE("QueryExecutor: operations on a missing collection fail cleanly") {
    EngineFixture f;
    CHECK(f.run("SCAN \"fantasma\"").find("no existe") != std::string::npos);
    CHECK(f.run("DELETE FROM \"fantasma\" WHERE {\"id\": \"1\"}").find("no existe") != std::string::npos);
    CHECK(f.run("DROP \"fantasma\"").find("no existe") != std::string::npos);
}

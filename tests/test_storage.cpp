#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "Storage/StorageManager.h"
#include "Storage/Serializer.h"
#include "Core/Types.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

// Every test lives under data/nexotest_* so the GUI's auto-discovery never
// picks these files up as user collections.
static const std::string kDataDir = "data";

static void clean(const std::string& base) {
    for (const char* ext : { ".bin", ".schema" }) {
        std::error_code ec;
        fs::remove(kDataDir + "/" + base + ext, ec);
    }
    std::error_code ec;
    fs::remove_all(kDataDir, ec);
}

static std::map<std::string, std::string> makeRecord(const std::string& id,
                                                     const std::string& nombre,
                                                     const std::string& edad) {
    return { { "id", id }, { "nombre", nombre }, { "edad", edad } };
}

TEST_CASE("Serializer: map round-trips through the binary format") {
    const std::map<std::string, std::string> original = {
        { "id", "42" },
        { "nombre", "Keny" },
        { "campo_con_espacios", "valor con    espacios y tabulado\t" },
        { "unicode", "Colección ñandú" },
    };

    const std::vector<char> bytes = Serializer::serialize(original);
    const std::map<std::string, std::string> back = Serializer::deserialize(bytes);

    CHECK(back == original);
}

TEST_CASE("Serializer: empty map round-trips") {
    const std::map<std::string, std::string> empty;
    const std::vector<char> bytes = Serializer::serialize(empty);
    CHECK(Serializer::deserialize(bytes).empty());
}

TEST_CASE("Serializer: large values round-trip") {
    const std::string big(2000, 'x');
    const std::map<std::string, std::string> original = { { "blob", big } };
    const std::map<std::string, std::string> back =
        Serializer::deserialize(Serializer::serialize(original));
    CHECK(back == original);
}

TEST_CASE("Serializer: truncated and empty buffers are handled safely") {
    // A header announcing more keys than the buffer contains must not read
    // past the end.
    const std::vector<char> truncated = { 0x05, 0x00 };           // 5 keys, no data
    CHECK_NOTHROW(Serializer::deserialize(truncated));

    const std::vector<char> empty;
    CHECK(Serializer::deserialize(empty).empty());
}

TEST_CASE("Serializer: lengths pointing outside the buffer never read past it") {
    // One key announced, but with a key length that swallows the rest of the
    // buffer (and beyond). Must not crash and must not read out of bounds.
    std::vector<char> lying = { 0x01, 0x00,                                // 1 key
                                static_cast<char>(0xFF), static_cast<char>(0xFF),
                                0x00, 0x00, 0x00, 0x00 };                  // keyLen = 65535
    CHECK_NOTHROW(Serializer::deserialize(lying));
    CHECK(Serializer::deserialize(lying).empty());

    // Same idea for the value length.
    std::vector<char> lying_value = { 0x01, 0x00,
                                      0x01, 0x00, 0x00, 0x00, 'a',         // key "a"
                                      static_cast<char>(0xFF), static_cast<char>(0xFF),
                                      static_cast<char>(0xFF), 0x00 };     // valLen huge
    CHECK_NOTHROW(Serializer::deserialize(lying_value));
    CHECK(Serializer::deserialize(lying_value).empty());
}

TEST_CASE("StorageManager: insert and read back records") {
    clean("nexotest_basic");
    StorageManager sm(kDataDir + "/nexotest_basic.bin");

    const RecordPointer p1 = sm.insertRecord(makeRecord("1", "Rafael", "21"));
    const RecordPointer p2 = sm.insertRecord(makeRecord("2", "Keny", "22"));

    CHECK(sm.getRecord(p1) == makeRecord("1", "Rafael", "21"));
    CHECK(sm.getRecord(p2) == makeRecord("2", "Keny", "22"));
    CHECK(sm.scanAll().size() == 2);
}

TEST_CASE("StorageManager: pages overflow and all records stay reachable") {
    clean("nexotest_overflow");
    StorageManager sm(kDataDir + "/nexotest_overflow.bin");

    const int count = 250; // far more than a single 4096-byte page holds
    for (int i = 1; i <= count; ++i) {
        const std::map<std::string, std::string> rec = {
            { "id", std::to_string(i) },
            { "payload", std::string(120, static_cast<char>('a' + (i % 26))) },
        };
        const RecordPointer p = sm.insertRecord(rec);
        CHECK(sm.getRecord(p)["id"] == std::to_string(i));
    }

    CHECK(static_cast<int>(sm.scanAll().size()) == count);
    CHECK(sm.getLastPageId() > 1); // actually spilled to several pages
}

TEST_CASE("StorageManager: logical delete hides a record from scan and get") {
    clean("nexotest_delete");
    StorageManager sm(kDataDir + "/nexotest_delete.bin");

    const RecordPointer p1 = sm.insertRecord(makeRecord("1", "a", "1"));
    sm.insertRecord(makeRecord("2", "b", "2"));

    sm.logicalDelete(p1);

    CHECK(sm.getRecord(p1).empty());
    CHECK(sm.scanAll().size() == 1);
    CHECK(sm.scanAll().front().second.at("id") == "2");
}

TEST_CASE("StorageManager: records survive a close/reopen cycle") {
    clean("nexotest_persist");
    const std::string path = kDataDir + "/nexotest_persist.bin";

    {
        StorageManager sm(path);
        sm.insertRecord(makeRecord("1", "a", "1"));
        sm.insertRecord(makeRecord("2", "b", "2"));
        sm.insertRecord(makeRecord("3", "c", "3"));
    }
    {
        StorageManager sm(path);
        const auto all = sm.scanAll();
        REQUIRE(all.size() == 3);
        CHECK(all[0].second.at("id") == "1");
        CHECK(all[1].second.at("id") == "2");
        CHECK(all[2].second.at("id") == "3");
    }
}

TEST_CASE("StorageManager: corrupt or out-of-range pointers are rejected safely") {
    clean("nexotest_badptr");
    StorageManager sm(kDataDir + "/nexotest_badptr.bin");
    sm.insertRecord(makeRecord("1", "a", "1"));

    CHECK(sm.getRecord({ -1, 0 }).empty());
    CHECK(sm.getRecord({ 1, -5 }).empty());
    CHECK(sm.getRecord({ 1, 999999 }).empty());  // slot fuera de la página
    CHECK(sm.getRecord({ 987654, 0 }).empty());  // página inexistente
}

TEST_CASE("StorageManager: a record that cannot fit any page is rejected") {
    clean("nexotest_huge");
    StorageManager sm(kDataDir + "/nexotest_huge.bin");

    const std::string huge_payload(PAGE_SIZE, 'x'); // > 4072 bytes útiles
    const std::map<std::string, std::string> huge = { { "blob", huge_payload } };

    CHECK_THROWS_AS(sm.insertRecord(huge), std::runtime_error);
    // La colección sigue operativa después del rechazo.
    const RecordPointer ok = sm.insertRecord(makeRecord("1", "a", "1"));
    CHECK(sm.getRecord(ok).at("id") == "1");
}

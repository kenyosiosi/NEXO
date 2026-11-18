#include <doctest/doctest.h>

#include "B-tree/IndexManager.h"
#include "Core/Types.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static const std::string kIdxPath = "data/nexotest_btree.idx";

static void clean() {
    std::error_code ec;
    fs::remove(kIdxPath, ec);
    fs::remove_all("data", ec);
}

TEST_CASE("B-Tree: findOne retrieves every inserted key") {
    clean();
    IndexManager idx(kIdxPath, 3);

    for (int i = 1; i <= 200; ++i) {
        idx.insert(std::to_string(i), { i, i });
    }

    for (int i = 1; i <= 200; ++i) {
        const RecordPointer p = idx.findOne(std::to_string(i));
        CHECK(p.page_id == i);
        CHECK(p.slot_offset == i);
    }
}

TEST_CASE("B-Tree: unordered insertion keeps the tree consistent") {
    clean();
    IndexManager idx(kIdxPath, 3);

    const std::vector<int> order = { 77, 3, 190, 45, 12, 200, 1, 99, 88, 34 };
    for (const int v : order) idx.insert(std::to_string(v), { v, v });

    for (const int v : order) {
        const RecordPointer p = idx.findOne(std::to_string(v));
        CHECK(p.page_id == v);
    }
}

TEST_CASE("B-Tree: missing keys are reported as not found") {
    clean();
    IndexManager idx(kIdxPath, 3);
    idx.insert("10", { 1, 1 });
    idx.insert("20", { 2, 2 });

    const RecordPointer missing = idx.findOne("15");
    CHECK(missing.page_id == -1);
    CHECK(missing.slot_offset == -1);

    // Searching in a brand-new (empty) index must also be safe.
    IndexManager empty("data/nexotest_btree_empty.idx", 3);
    CHECK(empty.findOne("anything").page_id == -1);
    std::error_code ec;
    fs::remove("data/nexotest_btree_empty.idx", ec);
}

TEST_CASE("B-Tree: duplicate keys are all retrievable via search") {
    clean();
    IndexManager idx(kIdxPath, 3);

    idx.insert("Keny", { 1, 0 });
    idx.insert("Keny", { 2, 0 });
    idx.insert("Keny", { 3, 0 });
    idx.insert("Rafa", { 4, 0 });

    const std::vector<RecordPointer> hits = idx.search("Keny");
    REQUIRE(hits.size() == 3);
    std::vector<int> pages;
    for (const RecordPointer& h : hits) pages.push_back(h.page_id);
    CHECK(std::find(pages.begin(), pages.end(), 1) != pages.end());
    CHECK(std::find(pages.begin(), pages.end(), 2) != pages.end());
    CHECK(std::find(pages.begin(), pages.end(), 3) != pages.end());
}

TEST_CASE("B-Tree: the tree survives a close/reopen cycle") {
    clean();
    {
        IndexManager idx(kIdxPath, 3);
        for (int i = 1; i <= 40; ++i) idx.insert(std::to_string(i), { i, i });
    }
    {
        IndexManager idx(kIdxPath, 3);
        for (int i = 1; i <= 40; ++i) {
            const RecordPointer p = idx.findOne(std::to_string(i));
            CHECK(p.page_id == i);
        }
        CHECK(idx.findOne("999").page_id == -1);
    }
}

TEST_CASE("B-Tree: the degree is honored and larger trees stay consistent") {
    clean();
    const std::string path = "data/nexotest_btree_t8.idx";
    {
        IndexManager idx(path, 8);
        CHECK(idx.getDegree() == 8);
        for (int i = 1; i <= 500; ++i) idx.insert(std::to_string(i), { i, i });
    }
    {
        IndexManager idx(path, 3); // el grado del archivo manda sobre el parámetro
        CHECK(idx.getDegree() == 8);
        for (int i = 1; i <= 500; ++i) {
            const RecordPointer p = idx.findOne(std::to_string(i));
            CHECK(p.page_id == i);
        }
        CHECK(idx.findOne("501").page_id == -1);
    }
    std::error_code ec;
    fs::remove(path, ec);
}

TEST_CASE("B-Tree: reopening after page splits keeps the tree usable") {
    // El contador de páginas nuevas debe persistirse en cada split: si no, al
    // reabrir el archivo el índice reutiliza páginas ocupadas y se corrompe.
    clean();
    {
        IndexManager idx(kIdxPath, 3);
        for (int i = 1; i <= 100; ++i) idx.insert(std::to_string(i), { i, i });
    }
    {
        IndexManager idx(kIdxPath, 3);
        for (int i = 101; i <= 200; ++i) idx.insert(std::to_string(i), { i, i });
        for (int i = 1; i <= 200; ++i) {
            CHECK(idx.findOne(std::to_string(i)).page_id == i);
        }
    }
}

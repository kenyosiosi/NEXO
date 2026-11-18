#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "query_engine/QueryExecutor.h"
#include "query_engine/tokenizer.h"
#include "query_engine/parser.h"
#include "Storage/StorageManager.h"
#include "Storage/SchemaManager.h"
#include "B-tree/IndexManager.h"

#ifdef _WIN32
    #include <windows.h>
    #ifdef DELETE
        #undef DELETE 
    #endif
#endif

namespace fs = std::filesystem;

struct LogEntry {
    std::string text;
    sf::Color color;
};

void asegurarCarpetaData() {
    if (!fs::exists("data")) fs::create_directory("data");
}

void autoCompact(std::string tableName,
                 std::map<std::string, StorageManager*>& storage_units, 
                 std::map<std::string, std::map<std::string, IndexManager*>>& index_map,
                 std::map<std::string, size_t>& registros_sucios) {
    
    if (storage_units.find(tableName) == storage_units.end()) return;

    auto live_records = storage_units[tableName]->scanAll(); 
    std::vector<std::string> fields_with_index;
    for (auto const& [field, mgr] : index_map[tableName]) fields_with_index.push_back(field);

    delete storage_units[tableName];
    for (auto const& field : fields_with_index) delete index_map[tableName][field];

    fs::remove("data/" + tableName + ".bin");
    for (const auto& field : fields_with_index) fs::remove("data/" + tableName + "_" + field + ".idx");

    storage_units[tableName] = new StorageManager("data/" + tableName + ".bin");
    for (const auto& field : fields_with_index) {
        index_map[tableName][field] = new IndexManager("data/" + tableName + "_" + field + ".idx", 3);
    }
    
    for (auto& record_pair : live_records) {
        auto& data = record_pair.second;
        RecordPointer new_ptr = storage_units[tableName]->insertRecord(data);
        for (const auto& field : fields_with_index) {
            if (data.count(field)) index_map[tableName][field]->insert(data[field], new_ptr);
        }
    }
    registros_sucios[tableName] = 0;
}

int main() {
    asegurarCarpetaData();
    sf::RenderWindow window(sf::VideoMode({1280, 720}), "NEXO Engine - Workspace");
    window.setFramerateLimit(60);

    // Icono y fuente viven en assets/, que puede estar al lado del
    // ejecutable (build/) o en la raíz del repo según desde dónde se corra.
    const auto localizarAsset = [](const char* relativa) -> std::string {
        for (const char* prefijo : { "", "../" }) {
            std::string candidato = std::string(prefijo) + relativa;
            std::error_code ec;
            if (fs::exists(candidato, ec)) return candidato;
        }
        return "";
    };

    if (const std::string logo = localizarAsset("assets/nexo_logo.png"); !logo.empty()) {
        sf::Image icon;
        if (icon.loadFromFile(logo)) window.setIcon(icon.getSize(), icon.getPixelsPtr());
    }

    if (!ImGui::SFML::Init(window)) return -1;

    // Fuente con cobertura Latin (acentos/ñ). ImGui usa por defecto una fuente
    // ASCII-only que corrompe el texto en español.
    if (const std::string font_path = localizarAsset("assets/fonts/JetBrainsMono-Regular.ttf"); !font_path.empty()) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.Fonts->AddFontFromFileTTF(font_path.c_str(), 17.0f)) {
            ImGui::SFML::UpdateFontTexture();
        }
    }

    std::map<std::string, StorageManager*> storage_units;
    std::map<std::string, std::map<std::string, IndexManager*>> index_map;
    std::map<std::string, size_t> registros_sucios;
    
    QueryExecutor executor(storage_units, index_map, registros_sucios);

    for (const auto& entry : fs::directory_iterator("data")) {
        std::string ext = entry.path().extension().string();
        std::string filename = entry.path().stem().string();

        if (ext == ".schema") {
            storage_units[filename] = new StorageManager("data/" + filename + ".bin");
            registros_sucios[filename] = 0;
        }
    }
    
    for (const auto& entry : fs::directory_iterator("data")) {
        if (entry.path().extension() == ".idx") {
            std::string filename = entry.path().stem().string();
            size_t pos = filename.find_last_of('_');
            if (pos != std::string::npos) {
                std::string tName = filename.substr(0, pos);
                std::string fName = filename.substr(pos + 1);
                if (storage_units.count(tName)) {
                    index_map[tName][fName] = new IndexManager(entry.path().string(), 3);
                }
            }
        }
    }

    std::vector<LogEntry> consoleLog;

    // Primera ejecución: sembrar una colección de demostración para que la
    // ventana no abra vacía.
    if (storage_units.empty()) {
        saveSchema("demo_usuarios", 6, "id,nombre,rol,ciudad");
        auto* demo_storage = new StorageManager("data/demo_usuarios.bin");
        storage_units["demo_usuarios"] = demo_storage;
        registros_sucios["demo_usuarios"] = 0;
        index_map["demo_usuarios"]["id"] = new IndexManager("data/demo_usuarios_id.idx", 3);

        const auto insertarDemo = [&](std::map<std::string, std::string> doc) {
            const RecordPointer ptr = demo_storage->insertRecord(doc);
            for (auto const& [field, idx] : index_map["demo_usuarios"]) {
                if (doc.count(field)) idx->insert(doc[field], ptr);
            }
        };

        insertarDemo({{"id", "1"}, {"nombre", "Alejandra Pérez"}, {"rol", "Admin"},     {"ciudad", "Mérida"}});
        insertarDemo({{"id", "2"}, {"nombre", "Rafael Ramírez"},   {"rol", "Ingeniero"},{"ciudad", "El Vigía"}});
        insertarDemo({{"id", "3"}, {"nombre", "Keny Rondón"},      {"rol", "Backend"},  {"ciudad", "Tovar"}});
        insertarDemo({{"id", "4"}, {"nombre", "Mónica Gutiérrez"}, {"rol", "Analista"}, {"ciudad", "Mérida"}});
        insertarDemo({{"id", "5"}, {"nombre", "José Ríos"},        {"rol", "DevOps"},   {"ciudad", "Valera"}});

        consoleLog.push_back({"Colección de demostración 'demo_usuarios' creada (5 documentos).", sf::Color(150, 150, 170)});
    }

    std::string selectedTable = "";
    SchemaInfo currentSchema = {1, "", {}};
    std::map<std::string, std::string> formInputs;
    
    std::vector<std::pair<RecordPointer, std::map<std::string, std::string>>> currentRecords;
    auto refreshUI = [&]() {
        if (selectedTable != "" && storage_units.count(selectedTable)) {
            currentRecords = storage_units[selectedTable]->scanAll();
            currentSchema = loadSchema(selectedTable);
        } else {
            currentRecords.clear();
        }
    };

    // Abrir con la primera colección ya seleccionada (la demo en el primer run).
    if (!storage_units.empty()) {
        selectedTable = storage_units.begin()->first;
        refreshUI();
    }

    char terminalBuffer[1024] = "";
    char nName[64] = "";
    char nFields[256] = "";
    bool showCreateModal = false;

    sf::Clock deltaClock;
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) window.close();
        }

        ImGui::SFML::Update(window, deltaClock.restart());
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)window.getSize().x, (float)window.getSize().y));
        ImGui::Begin("NEXO Workspace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        ImGui::Columns(3, "MainLayout", true);
        
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "EXPLORADOR DE COLECCIONES");
        if (ImGui::Button("Nueva Colección +", ImVec2(-1, 0))) showCreateModal = true;
        ImGui::Separator();
        
        ImGui::BeginChild("TableList");
        for (auto const& [name, ptr] : storage_units) {
            if (ImGui::Selectable(name.c_str(), selectedTable == name)) {
                selectedTable = name;
                formInputs.clear();
                refreshUI(); 
            }
        }
        ImGui::EndChild();
        ImGui::NextColumn();

        if (selectedTable == "") {
            ImGui::SetCursorPosY(ImGui::GetWindowHeight() / 2);
            ImGui::TextDisabled(" <- selecciona una coleccion para comenzar");
        } else {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "COLECCIÓN ACTIVA: %s", selectedTable.c_str());
            ImGui::Separator();

            ImGui::Text("Nuevo Documento:");
            for (auto& f : currentSchema.fieldsList) {
                if (f == "id") continue; 
                char buf[256] = "";
                if (formInputs.count(f)) strcpy(buf, formInputs[f].c_str());
                if (ImGui::InputText(f.c_str(), buf, 256)) formInputs[f] = buf;
            }
            
            if (ImGui::Button("INSERTAR", ImVec2(100, 30))) {
                try {
                    formInputs["id"] = std::to_string(currentSchema.nextId);

                    RecordPointer ptr = storage_units[selectedTable]->insertRecord(formInputs);
                    for (auto const& [field, idx] : index_map[selectedTable]) {
                        if (formInputs.count(field)) idx->insert(formInputs[field], ptr);
                    }

                    saveSchema(selectedTable, currentSchema.nextId + 1, currentSchema.fields);

                    consoleLog.push_back({"Registro insertado con ID: " + formInputs["id"], sf::Color::Green});
                    formInputs.clear();
                    refreshUI();
                } catch (const std::exception& e) {
                    consoleLog.push_back({"[Error] " + std::string(e.what()), sf::Color::Red});
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("COMPACTAR", ImVec2(100, 30))) {
                autoCompact(selectedTable, storage_units, index_map, registros_sucios);
                consoleLog.push_back({"Colección compactada físicamente y re-indexada.", sf::Color::Cyan});
                refreshUI(); 
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "DOCUMENTOS GUARDADOS:");
            ImGui::BeginChild("DocExplorer", ImVec2(0, 0), true);
            
            int doc_idx = 0;
            for (auto& row : currentRecords) {
                std::string header = "Doc [" + std::to_string(row.first.page_id) + ":" + std::to_string(row.first.slot_offset) + "]";
                if (row.second.count("id")) header += " | ID: " + row.second["id"];

                if (ImGui::TreeNode((void*)(intptr_t)doc_idx, "%s", header.c_str())) {
                    for (auto const& [k, v] : row.second) {
                        ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 1.0f), "\"%s\":", k.c_str());
                        ImGui::SameLine();
                        ImGui::Text("\"%s\"", v.c_str());
                    }
                    ImGui::TreePop();
                }
                ImGui::Separator();
                doc_idx++;
            }
            ImGui::EndChild();
        }
        ImGui::NextColumn();

        ImGui::Text("TERMINAL NEXO");
        ImGui::BeginChild("Logs", ImVec2(0, -40), true);
        for (auto& log : consoleLog) ImGui::TextColored(ImVec4(log.color.r/255.f, log.color.g/255.f, log.color.b/255.f, 1.0f), "%s", log.text.c_str());
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();

        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##t", terminalBuffer, 1024, ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::string inputCmd(terminalBuffer);
            if (!inputCmd.empty()) {
                consoleLog.push_back({"> " + inputCmd, sf::Color(180, 180, 180)});

                try {
                    Tokenizer tz(inputCmd);
                    Parser parser(tz.tokenize());
                    std::map<std::string, std::string> ast = parser.parse();

                    std::string result = executor.execute(ast);
                    consoleLog.push_back({result, sf::Color::Cyan});
                    
                    if (ast["operation"] == "DROP" && ast["_table_target"] == selectedTable) {
                        selectedTable = "";
                    }
                    
                    refreshUI(); 
                } catch (const std::exception& e) {
                    consoleLog.push_back({"[Error] " + std::string(e.what()), sf::Color::Red});
                }
            }
            memset(terminalBuffer, 0, 1024);
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::PopItemWidth();

        ImGui::Columns(1);

        if (showCreateModal) {
            ImGui::OpenPopup("Configurar Nueva Colección");
            showCreateModal = false;
        }

        if (ImGui::BeginPopupModal("Configurar Nueva Colección", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Nombre:"); 
            ImGui::InputText("##name", nName, 64);
            
            ImGui::Text("Campos:");
            ImGui::InputTextWithHint("##fields", "Ej. nombre, edad, cedula", nFields, 256);

            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                if (strlen(nName) > 0 && validateCollectionName(nName) && validateSchemaFields(nFields)) {
                    std::string finalFields = "id";
                    if (strlen(nFields) > 0) finalFields += "," + std::string(nFields);

                    saveSchema(nName, 1, finalFields);
                    storage_units[nName] = new StorageManager("data/" + std::string(nName) + ".bin");
                    registros_sucios[nName] = 0;
                    
                    index_map[nName]["id"] = new IndexManager("data/" + std::string(nName) + "_id.idx", 3);
                    
                    consoleLog.push_back({"Colección '" + std::string(nName) + "' lista.", sf::Color::Green});
                    memset(nName, 0, 64); memset(nFields, 0, 256);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::End();
        window.clear(sf::Color(25, 25, 28));
        ImGui::SFML::Render(window);
        window.display();
    }

    for (auto& [n, p] : storage_units) delete p;
    for (auto& [tName, fMap] : index_map) {
        for (auto& [fName, p] : fMap) delete p;
    }
    
    ImGui::SFML::Shutdown();
    return 0;
}
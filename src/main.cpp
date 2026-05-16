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

#include "Query_engine/QueryExecutor.h"
#include "Query_engine/tokenizer.h"
#include "Query_engine/parser.h"
#include "Storage/StorageManager.h"
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

struct SchemaInfo { 
    int nextId; 
    std::string fields; 
    std::vector<std::string> fieldsList;
};

bool validateSchema(const std::string& fields) {
    if (fields.empty()) return false;
    return (fields.find_first_of(".;/\\") == std::string::npos);
}

SchemaInfo loadSchema(const std::string& tableName) {
    std::ifstream f("data/" + tableName + ".schema");
    SchemaInfo info = {1, "", {}};
    if (f.is_open()) {
        std::string idStr;
        std::getline(f, idStr);
        if (!idStr.empty()) info.nextId = std::stoi(idStr);
        std::getline(f, info.fields);
        
        std::stringstream ss(info.fields);
        std::string field;
        while (std::getline(ss, field, ',')) {
            field.erase(0, field.find_first_not_of(" "));
            if(!field.empty()) info.fieldsList.push_back(field);
        }
        f.close();
    }
    return info;
}

void saveSchema(const std::string& tableName, int nextId, const std::string& fields) {
    std::ofstream f("data/" + tableName + ".schema");
    if (f.is_open()) {
        f << nextId << "\n" << fields;
        f.close();
    }
}

// PASO 1: Auto-Compactado (Mejorado con reinicio de métricas)
void autoCompact(std::string tableName, 
                 std::map<std::string, StorageManager*>& storage_units, 
                 std::map<std::string, std::map<std::string, IndexManager*>>& index_map,
                 std::map<std::string, size_t>& registros_sucios) {
    
    if (storage_units.find(tableName) == storage_units.end()) return;

    // 1. Extraer solo los vivos
    auto live_records = storage_units[tableName]->scanAll(); 
    std::vector<std::string> fields_with_index;
    for (auto const& [field, mgr] : index_map[tableName]) {
        fields_with_index.push_back(field);
    }

    // 2. Cerrar archivos (Libera lock de memoria)
    delete storage_units[tableName];
    for (auto const& field : fields_with_index) {
        delete index_map[tableName][field];
    }

    // 3. Borrar basura física
    fs::remove("data/" + tableName + ".bin");
    for (const auto& field : fields_with_index) {
        fs::remove("data/" + tableName + "_" + field + ".idx");
    }

    // 4. Recrear objetos y archivos limpios
    storage_units[tableName] = new StorageManager("data/" + tableName + ".bin");
    for (const auto& field : fields_with_index) {
        index_map[tableName][field] = new IndexManager("data/" + tableName + "_" + field + ".idx", 3);
    }
    
    // 5. Reinserción y re-indexado físico
    for (auto& record_pair : live_records) {
        auto& data = record_pair.second;
        RecordPointer new_ptr = storage_units[tableName]->insertRecord(data);
        for (const auto& field : fields_with_index) {
            if (data.count(field)) index_map[tableName][field]->insert(data[field], new_ptr);
        }
    }

    // 6. Limpiar contador de fragmentación
    registros_sucios[tableName] = 0;
}

int main() {
    asegurarCarpetaData();
    sf::RenderWindow window(sf::VideoMode(1280, 720), "NEXO Engine - Workspace");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window)) return -1;

    std::map<std::string, StorageManager*> storage_units;
    std::map<std::string, std::map<std::string, IndexManager*>> index_map;
    std::map<std::string, size_t> registros_sucios;
    
    QueryExecutor executor(storage_units, index_map, registros_sucios);

    // PASO 3: Persistencia y Auto-Carga (Buscando Schemas e Índices)
    // Primero instanciamos los StorageManagers
    for (const auto& entry : fs::directory_iterator("data")) {
        std::string ext = entry.path().extension().string();
        std::string filename = entry.path().stem().string();

        if (ext == ".schema") {
            storage_units[filename] = new StorageManager("data/" + filename + ".bin");
            registros_sucios[filename] = 0; // Asumimos 0 al iniciar, se actualizará al borrar
        }
    }
    
    // Segundo: Enlazamos todos los índices existentes sin importar la tabla
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
    std::string selectedTable = "";
    SchemaInfo currentSchema = {1, "", {}};
    std::map<std::string, std::string> formInputs;
    
    // PASO 2: Sincronización UI-Motor (Caché local de registros)
    std::vector<std::pair<RecordPointer, std::map<std::string, std::string>>> currentRecords;
    auto refreshUI = [&]() {
        if (selectedTable != "" && storage_units.count(selectedTable)) {
            currentRecords = storage_units[selectedTable]->scanAll();
        } else {
            currentRecords.clear();
        }
    };

    char terminalBuffer[1024] = "";
    char nName[64] = "";
    char nFields[256] = "";
    bool showCreateModal = false;

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);
            if (event.type == sf::Event::Closed) window.close();
        }

        ImGui::SFML::Update(window, deltaClock.restart());
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)window.getSize().x, (float)window.getSize().y));
        ImGui::Begin("NEXO Workspace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        ImGui::Columns(3, "MainLayout", true);
        
        // --- COLUMNA 1: EXPLORADOR DE DATOS ---
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "EXPLORADOR DE COLECCIONES");
        if (ImGui::Button("Nueva Colección +", ImVec2(-1, 0))) showCreateModal = true;
        ImGui::Separator();
        
        ImGui::BeginChild("TableList");
        for (auto const& [name, ptr] : storage_units) {
            if (ImGui::Selectable(name.c_str(), selectedTable == name)) {
                selectedTable = name;
                currentSchema = loadSchema(selectedTable);
                formInputs.clear();
                refreshUI(); // <--- Disparador UI
            }
        }
        ImGui::EndChild();
        ImGui::NextColumn();

        // --- COLUMNA 2: FORMULARIO Y EXPLORADOR NoSQL ---
        if (selectedTable == "") {
            ImGui::SetCursorPosY(ImGui::GetWindowHeight() / 2);
            ImGui::TextDisabled(" <- selecciona una coleccion para comenzar");
        } else {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "COLECCIÓN ACTIVA: %s", selectedTable.c_str());
            ImGui::Separator();

            ImGui::Text("Nuevo Documento:");
            for (auto& f : currentSchema.fieldsList) {
                char buf[256] = "";
                if (formInputs.count(f)) strcpy(buf, formInputs[f].c_str());
                if (ImGui::InputText(f.c_str(), buf, 256)) formInputs[f] = buf;
            }
            
            if (ImGui::Button("INSERTAR", ImVec2(100, 30))) {
                RecordPointer ptr = storage_units[selectedTable]->insertRecord(formInputs);
                for (auto const& [field, idx] : index_map[selectedTable]) {
                    if (formInputs.count(field)) idx->insert(formInputs[field], ptr);
                }
                consoleLog.push_back({"Registro insertado en " + selectedTable, sf::Color::Green});
                refreshUI(); // <--- Disparador UI
            }
            ImGui::SameLine();
            if (ImGui::Button("COMPACTAR", ImVec2(100, 30))) {
                autoCompact(selectedTable, storage_units, index_map, registros_sucios);
                consoleLog.push_back({"Colección compactada físicamente y re-indexada.", sf::Color::Cyan});
                refreshUI(); // <--- Disparador UI
            }

            ImGui::Separator();
            
            // PASO 4: Explorador de Documentos NoSQL (Árboles JSON Expandibles)
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "DOCUMENTOS GUARDADOS:");
            ImGui::BeginChild("DocExplorer", ImVec2(0, 0), true);
            
            int doc_idx = 0;
            for (auto& row : currentRecords) {
                // Título del Nodo (Ej: Doc [1:0] | ID: 15)
                std::string header = "Doc [" + std::to_string(row.first.page_id) + ":" + std::to_string(row.first.slot_offset) + "]";
                if (row.second.count("id")) header += " | ID: " + row.second["id"];

                // ImGui::TreeNode permite desplegar objetos
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

        // --- COLUMNA 3: LOGS Y TERMINAL ---
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
                    
                    refreshUI(); // <--- Disparador UI después de ejecutar comando de consola
                } catch (const std::exception& e) {
                    consoleLog.push_back({"[Error] " + std::string(e.what()), sf::Color::Red});
                }
            }
            memset(terminalBuffer, 0, 1024);
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::PopItemWidth();

        ImGui::Columns(1);

        // --- POPUP MODAL ---
        if (showCreateModal) {
            ImGui::OpenPopup("Configurar Nueva Colección");
            showCreateModal = false;
        }

        if (ImGui::BeginPopupModal("Configurar Nueva Colección", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Nombre:"); 
            ImGui::InputText("##name", nName, 64);
            
            ImGui::Text("Campos (Solo visual, NEXO acepta llaves variables):");
            ImGui::InputTextWithHint("##fields", "Ej. id, nombre, correo", nFields, 256);

            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                if (strlen(nName) > 0 && validateSchema(nFields)) {
                    saveSchema(nName, 1, nFields);
                    storage_units[nName] = new StorageManager("data/" + std::string(nName) + ".bin");
                    registros_sucios[nName] = 0;
                    
                    std::string flds = nFields;
                    std::string first = flds.substr(0, flds.find(','));
                    index_map[nName][first] = new IndexManager("data/" + std::string(nName) + "_" + first + ".idx", 3);
                    
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

    // Limpieza de memoria al cerrar (provoca cierre seguro de fstream en StorageManager)
    for (auto& [n, p] : storage_units) delete p;
    for (auto& [tName, fMap] : index_map) {
        for (auto& [fName, p] : fMap) delete p;
    }
    
    ImGui::SFML::Shutdown();
    return 0;
}
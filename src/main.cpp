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
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>

#include "query_engine/QueryExecutor.h"
#include "query_engine/tokenizer.h"
#include "query_engine/parser.h"
#include "Storage/StorageManager.h"
#include "Storage/SchemaManager.h"
#include "Storage/CsvExporter.h"
#include "B-tree/IndexManager.h"
#include "ui/GuiStrings.h"

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

// El log crecía sin límite (fuga en sesiones largas): cap duro de entradas.
constexpr size_t kMaxLogEntries = 500;

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

// Layout del área de trabajo (en píxeles de la ventana).
constexpr float kSidebarWidth = 230.0f;
constexpr float kTerminalHeight = 208.0f;
constexpr float kStatusBarHeight = 26.0f;

// Comandos de la guía: sintaxis universal + ejemplo ejecutable ("Probar").
struct GuideCmd {
    const char* syntax;
    ui::S desc;
    const char* example;
};

const GuideCmd kGuideCmds[] = {
    {"CREATE \"col\" {\"fields\": \"nombre,edad\"}",                          ui::S::CmdCreateDesc, "CREATE \"usuarios\" {\"fields\": \"nombre,edad\"}"},
    {"INSERT INTO \"col\" {\"campo\": \"valor\"}",                            ui::S::CmdInsertDesc, "INSERT INTO \"demo_usuarios\" {\"nombre\": \"Ana\", \"rol\": \"QA\"}"},
    {"GET FROM \"col\" {\"id\": \"1\"}",                                      ui::S::CmdGetDesc,    "GET FROM \"demo_usuarios\" {\"id\": \"1\"}"},
    {"SCAN \"col\"",                                                          ui::S::CmdScanDesc,   "SCAN \"demo_usuarios\""},
    {"UPDATE \"col\" SET {\"campo\": \"valor\"} WHERE {\"id\": \"1\"}",       ui::S::CmdUpdateDesc, "UPDATE \"demo_usuarios\" SET {\"rol\": \"Senior\"} WHERE {\"id\": \"1\"}"},
    {"DELETE FROM \"col\" WHERE {\"id\": \"1\"}",                             ui::S::CmdDeleteDesc, "DELETE FROM \"demo_usuarios\" WHERE {\"id\": \"1\"}"},
    {"INDEX \"campo\" ON \"col\"",                                            ui::S::CmdIndexDesc,  "INDEX \"ciudad\" ON \"demo_usuarios\""},
    {"STATS \"col\"",                                                         ui::S::CmdStatsDesc,  "STATS \"demo_usuarios\""},
    {"DEFINE \"col\"",                                                        ui::S::CmdDefineDesc, "DEFINE \"demo_usuarios\""},
    {"DROP \"col\"",                                                          ui::S::CmdDropDesc,   "DROP \"col\""},
};

// ---------------------------------------------------------------------------
// Driver de demostración: con la variable de entorno NEXO_DEMO_SCRIPT la app
// se conduce sola (teclear, ejecutar, abrir modales, cambiar idioma) y captura
// frames a PNG. Lo usa la automatización de GIF/capturas sin depender del
// foco de la ventana de Windows. Pasos separados por ';':
//   TYPE:<texto> · ENTER · CLICK:x,y · PAUSE:<ms> · SHOT:<nombre> ·
//   GUIDE · CLOSE_GUIDE · LANG:ES|EN
// ---------------------------------------------------------------------------
struct DemoStep {
    enum Kind { TYPE, ENTER, CLICK, PAUSE, SHOT, GUIDE, CLOSE_GUIDE, LANG } kind;
    std::string arg1;
    std::string arg2;
    int ms = 0;
};

std::vector<DemoStep> parseDemoScript(const std::string& script) {
    std::vector<DemoStep> steps;
    size_t pos = 0;
    while (pos <= script.size()) {
        size_t sep = script.find(';', pos);
        if (sep == std::string::npos) sep = script.size();
        std::string tok = script.substr(pos, sep - pos);
        pos = sep + 1;
        if (tok.empty()) continue;

        auto starts = [&](const char* p) { return tok.rfind(p, 0) == 0; };
        if (starts("TYPE:")) steps.push_back({DemoStep::TYPE, tok.substr(5), "", 0});
        else if (starts("CLICK:")) {
            DemoStep s{DemoStep::CLICK, "", "", 0};
            size_t x = tok.substr(6).find('x');
            if (x != std::string::npos) {
                s.arg1 = tok.substr(6, x);
                s.arg2 = tok.substr(6 + x + 1);
            }
            steps.push_back(s);
        }
        else if (starts("PAUSE:")) steps.push_back({DemoStep::PAUSE, "", "", atoi(tok.substr(6).c_str())});
        else if (starts("SHOT:")) steps.push_back({DemoStep::SHOT, tok.substr(5), "", 0});
        else if (starts("LANG:")) steps.push_back({DemoStep::LANG, tok.substr(5), "", 0});
        else if (tok == "ENTER") steps.push_back({DemoStep::ENTER, "", "", 0});
        else if (tok == "GUIDE") steps.push_back({DemoStep::GUIDE, "", "", 0});
        else if (tok == "CLOSE_GUIDE") steps.push_back({DemoStep::CLOSE_GUIDE, "", "", 0});
    }
    return steps;
}

int main() {
    asegurarCarpetaData();
    ui::loadPrefs(); // idioma + banner persistidos en data/ui.cfg

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

    // Init sin la fuente por defecto: así el atlas se construye una sola vez y
    // ya con JetBrains Mono. Si Init() lo construye con la fuente embebida, la
    // TTF se añade después y el texto acaba dibujándose con la de reserva (otro
    // tamaño, sin guiones tipográficos ni flechas).
    if (!ImGui::SFML::Init(window, false)) return -1;

    {
        ImGuiIO& io = ImGui::GetIO();
        // Rangos explícitos: Latin-1 (acentos, ñ, ¿, ¡) más guiones
        // tipográficos, viñetas y flechas que usa la ayuda en pantalla.
        static const ImWchar kGlyphRanges[] = {
            0x0020, 0x00FF, // Latin básico + suplemento
            0x2010, 0x2027, // guion, comillas tipográficas, puntos suspensivos, viñeta
            0x2190, 0x21FF, // flechas (↑ ↓ → ←)
            0x25A0, 0x25FF, // formas geométricas
            0,
        };
        const std::string font_path = localizarAsset("assets/fonts/JetBrainsMono-Regular.ttf");
        const bool loaded = !font_path.empty() &&
            io.Fonts->AddFontFromFileTTF(font_path.c_str(), 17.0f, nullptr, kGlyphRanges) != nullptr;
        if (!loaded) io.Fonts->AddFontDefault(); // nunca dejar el atlas vacío
        if (!ImGui::SFML::UpdateFontTexture())
            std::cerr << "[Nexo] no se pudo subir el atlas de fuentes\n";
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
    auto pushLog = [&](std::string text, sf::Color color) {
        consoleLog.push_back({std::move(text), color});
        if (consoleLog.size() > kMaxLogEntries)
            consoleLog.erase(consoleLog.begin(),
                             consoleLog.begin() + static_cast<long>(consoleLog.size() - kMaxLogEntries));
    };

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

        pushLog(ui::str(ui::S::LogDemoSeed), sf::Color(150, 150, 170));
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
    bool showGuide = false;
    bool createInvalid = false;
    bool focusTerminal = true; // la terminal arranca enfocada: se puede escribir de inmediato

    // Edición y borrado desde el visor de documentos.
    bool editing = false;
    RecordPointer editPtr = { -1, -1 };
    std::string editId;
    bool showDeleteModal = false;
    RecordPointer deletePtr = { -1, -1 };
    std::string deleteId;

    // Historial de la terminal (↑ / ↓).
    std::vector<std::string> cmdHistory;
    int cmdHistoryPos = -1;
    constexpr size_t kMaxHistory = 200;

    // --- Modo demo: conducción scripted para GIF/capturas automatizadas ---
    std::vector<DemoStep> demoSteps;
    if (const char* d = std::getenv("NEXO_DEMO_SCRIPT")) demoSteps = parseDemoScript(d);
    size_t demoIdx = 0;
    int demoHold = 45;         // frames de gracia antes del primer paso
    int demoTypeLeft = -1;     // caracteres que faltan por teclear (-1 = paso sin iniciar)
    // Un click real necesita varios frames: ImGui calcula el item bajo el cursor
    // antes de ver la pulsación, así que posicionamos, esperamos y pulsamos.
    int demoClickPhase = 0;    // 0 = inactivo, 1 = posicionando, 2 = pulsado, 3 = soltar
    int demoClickX = 0;
    int demoClickY = 0;
    int demoClickWait = 0;
    bool demoEnterPending = false;  // soltar Enter en el frame siguiente
    std::string pendingShot;   // captura solicitada para el final del frame
    if (!fs::exists("data/shots")) fs::create_directory("data/shots");
    bool demoCloseGuide = false;    // cierra el modal GUÍA desde el script

    // Ejecución de un comando de terminal: la usan el usuario (Enter), el
    // botón "Probar" de la guía y el driver del modo demo.
    auto ejecutarComando = [&](const std::string& inputCmd) {
        if (inputCmd.empty()) return;
        if (cmdHistory.empty() || cmdHistory.back() != inputCmd) cmdHistory.push_back(inputCmd);
        if (cmdHistory.size() > kMaxHistory) cmdHistory.erase(cmdHistory.begin());
        cmdHistoryPos = -1;

        pushLog("> " + inputCmd, sf::Color(180, 180, 180));

        std::string upperCmd;
        for (char ch : inputCmd) upperCmd.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));

        if (upperCmd == "HELP" || upperCmd == "AYUDA" || upperCmd == "?") {
            for (const auto& c : kGuideCmds)
                pushLog(std::string(c.syntax) + "  —  " + ui::str(c.desc), sf::Color(200, 200, 215));
        } else if (upperCmd == "CLEAR" || upperCmd == "LIMPIAR") {
            consoleLog.clear();
        } else {
            try {
                Tokenizer tz(inputCmd);
                Parser parser(tz.tokenize());
                std::map<std::string, std::string> ast = parser.parse();

                std::string result = executor.execute(ast);
                pushLog(result, sf::Color::Cyan);

                if (ast["operation"] == "DROP" && ast["_table_target"] == selectedTable) {
                    selectedTable = "";
                }

                refreshUI();
            } catch (const std::exception& e) {
                pushLog(std::string("[Error] ") + e.what(), sf::Color::Red);
            }
        }
    };

    sf::Clock deltaClock;
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) window.close();
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        // --- Driver del modo demo (antes de construir la UI) ---------------
        if (!demoSteps.empty()) {
            ImGuiIO& io = ImGui::GetIO();
            if (demoEnterPending) {
                io.AddKeyEvent(ImGuiKey_Enter, false); // soltar Enter al frame siguiente
                demoEnterPending = false;
                demoHold = 15;
            } else if (demoClickPhase == 1) {
                // Cursor quieto sobre el objetivo para que ImGui lo marque hovered.
                io.AddMousePosEvent(static_cast<float>(demoClickX), static_cast<float>(demoClickY));
                if (--demoClickWait <= 0) demoClickPhase = 2;
            } else if (demoClickPhase == 2) {
                io.AddMousePosEvent(static_cast<float>(demoClickX), static_cast<float>(demoClickY));
                io.AddMouseButtonEvent(0, true);
                demoClickPhase = 3;
            } else if (demoClickPhase == 3) {
                io.AddMousePosEvent(static_cast<float>(demoClickX), static_cast<float>(demoClickY));
                io.AddMouseButtonEvent(0, false); // soltar el click
                demoClickPhase = 0;
                demoIdx++;
                demoHold = 15;
            } else if (demoHold > 0) {
                demoHold--;
            } else if (demoIdx < demoSteps.size()) {
                const DemoStep& s = demoSteps[demoIdx];
                switch (s.kind) {
                    case DemoStep::TYPE:
                        if (demoTypeLeft < 0) demoTypeLeft = static_cast<int>(s.arg1.size());
                        if (demoTypeLeft > 0) {
                            const size_t idx = s.arg1.size() - static_cast<size_t>(demoTypeLeft);
                            // Como una pulsación real: ImGui escribe en su estado
                            // interno del InputText (no tocamos el buffer a mano).
                            io.AddInputCharacter(static_cast<unsigned int>(
                                static_cast<unsigned char>(s.arg1[idx])));
                            demoTypeLeft--;
                            demoHold = 3; // ritmo de tecleo visible
                        } else {
                            demoIdx++;
                            demoHold = 12;
                            demoTypeLeft = -1;
                        }
                        break;
                    case DemoStep::ENTER:
                        // Enter real: el InputText con EnterReturnsTrue ejecuta.
                        io.AddKeyEvent(ImGuiKey_Enter, true);
                        demoEnterPending = true;
                        demoIdx++;
                        demoHold = 6;
                        break;
                    case DemoStep::CLICK: {
                        demoClickX = atoi(s.arg1.c_str());
                        demoClickY = atoi(s.arg2.c_str());
                        demoClickPhase = 1;
                        demoClickWait = 3; // frames de hover antes de pulsar
                        break;
                    }
                    case DemoStep::PAUSE: demoHold = std::max(1, s.ms / 16); demoIdx++; break;
                    case DemoStep::SHOT:  pendingShot = s.arg1; demoIdx++; demoHold = 10; break;
                    case DemoStep::GUIDE: showGuide = true; demoIdx++; demoHold = 25; break;
                    case DemoStep::CLOSE_GUIDE: demoCloseGuide = true; demoIdx++; demoHold = 15; focusTerminal = true; break;
                    case DemoStep::LANG:
                        ui::toggleLang(s.arg1 == "EN" ? nexo::Lang::EN : nexo::Lang::ES);
                        demoIdx++;
                        demoHold = 25;
                        break;
                }
            } else {
                // Guion terminado: cerrar la app sola.
                window.close();
            }
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)window.getSize().x, (float)window.getSize().y));
        ImGui::Begin("NEXO Workspace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        // --- Barra superior: título, idioma y ayuda -------------------------
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "NEXO Workspace");
        ImGui::SameLine();
        ImGui::TextDisabled("v1.1");

        const float topRight = ImGui::GetWindowWidth() - 8.0f;
        const bool esActive = (nexo::lang() == nexo::Lang::ES);

        ImGui::SameLine(topRight - 130.0f);
        if (esActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.45f, 0.75f, 1.0f));
        if (ImGui::Button("ES", ImVec2(40, 0))) {
            if (!esActive) {
                ui::toggleLang(nexo::Lang::ES);
                pushLog(ui::str(ui::S::LogLangEs), sf::Color(150, 150, 170));
            }
        }
        if (esActive) ImGui::PopStyleColor();

        ImGui::SameLine();
        if (!esActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.45f, 0.75f, 1.0f));
        if (ImGui::Button("EN", ImVec2(40, 0))) {
            if (esActive) {
                ui::toggleLang(nexo::Lang::EN);
                pushLog(ui::str(ui::S::LogLangEn), sf::Color(150, 150, 170));
            }
        }
        if (!esActive) ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button("?", ImVec2(40, 0))) showGuide = true;

        // Banner de bienvenida (primer arranque, descartable y persistido).
        if (!ui::bannerSeen()) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.16f, 0.24f, 0.34f, 1.0f));
            ImGui::BeginChild("##welcome", ImVec2(0, 72), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::TextWrapped("%s", ui::str(ui::S::BannerText).c_str());
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 110.0f);
            if (ImGui::SmallButton(ui::str(ui::S::BannerDismiss).c_str())) ui::setBannerSeen(true);
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        // --- Cuerpo: explorador de ancho fijo (230 px) + área de trabajo ---
        // Tabla en lugar de Columns: la barra lateral no se reparte el espacio
        // con el contenido y el separador se puede arrastrar.
        ImGui::BeginChild("##body", ImVec2(0, -kStatusBarHeight), false);
        if (ImGui::BeginTable("##layout", 2,
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV |
                              ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("##sidebar", ImGuiTableColumnFlags_WidthFixed, kSidebarWidth);
            ImGui::TableSetupColumn("##work", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "%s", ui::str(ui::S::CollectionsExplorer).c_str());
            if (ImGui::Button(ui::str(ui::S::NewCollectionBtn).c_str(), ImVec2(-1, 0))) showCreateModal = true;
            ImGui::Separator();

            ImGui::BeginChild("TableList");
            for (auto const& [name, ptr] : storage_units) {
                if (ImGui::Selectable(name.c_str(), selectedTable == name)) {
                    selectedTable = name;
                    formInputs.clear();
                    editing = false;
                    refreshUI(); 
                }
            }
            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);
            // Documentos arriba; la terminal ocupa una franja fija abajo.
            ImGui::BeginChild("##docsPane", ImVec2(0, -kTerminalHeight), false);

        if (selectedTable == "") {
            ImGui::SetCursorPosY(ImGui::GetWindowHeight() / 2);
            ImGui::TextDisabled("%s", ui::str(ui::S::SelectPrompt).c_str());
        } else {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%s", ui::strf(ui::S::ActiveCollection, selectedTable).c_str());
            ImGui::Separator();

            if (ImGui::BeginTabBar("##collectionTabs")) {
                // ---------------- Documentos ----------------
                if (ImGui::BeginTabItem(ui::str(ui::S::TabDocs).c_str())) {
                    if (editing) {
                        ImGui::PushTextWrapPos(0.0f);
                        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.30f, 1.0f), "%s",
                                           ui::strf(ui::S::EditModeHint, editId).c_str());
                        ImGui::PopTextWrapPos();
                    } else {
                        ImGui::Text("%s", ui::str(ui::S::NewDocument).c_str());
                    }
                    // Formulario: etiqueta a la izquierda y campo de ancho fijo;
                    // así los nombres no se pegan al borde de la ventana.
                    if (ImGui::BeginTable("##form", 2, ImGuiTableFlags_SizingFixedFit)) {
                        ImGui::TableSetupColumn("##lbl", ImGuiTableColumnFlags_WidthFixed, 84.0f);
                        ImGui::TableSetupColumn("##in", ImGuiTableColumnFlags_WidthFixed, 340.0f);
                        for (auto& f : currentSchema.fieldsList) {
                            if (f == "id") continue;
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::AlignTextToFramePadding();
                            ImGui::TextUnformatted(f.c_str());
                            ImGui::TableSetColumnIndex(1);
                            char buf[256] = "";
                            if (formInputs.count(f)) strcpy(buf, formInputs[f].c_str());
                            ImGui::SetNextItemWidth(-FLT_MIN);
                            if (ImGui::InputText(("##f_" + f).c_str(), buf, 256)) formInputs[f] = buf;
                        }
                        ImGui::EndTable();
                    }
                    
                    const std::string primaryLabel = editing ? ui::str(ui::S::UpdateBtn)
                                                             : ui::str(ui::S::InsertBtn);
                    if (ImGui::Button(primaryLabel.c_str(), ImVec2(100, 30))) {
                        try {
                            auto* sm = storage_units[selectedTable];

                            if (editing) {
                                // UPDATE = tombstone del documento viejo + alta
                                // del nuevo. Las entradas de índice del viejo
                                // quedan apuntando a un slot muerto: findLiveRecord
                                // las ignora y COMPACTAR las purga.
                                formInputs["id"] = editId;
                                if (sm->getRecord(editPtr).empty()) {
                                    pushLog(ui::str(ui::S::LogDocStale), sf::Color(240, 200, 80));
                                } else {
                                    sm->logicalDelete(editPtr);
                                    registros_sucios[selectedTable]++;

                                    RecordPointer fresh = sm->insertRecord(formInputs);
                                    for (auto const& [field, idx] : index_map[selectedTable]) {
                                        if (formInputs.count(field)) idx->insert(formInputs[field], fresh);
                                    }
                                    pushLog(ui::strf(ui::S::LogUpdateOk, editId), sf::Color::Green);
                                }
                                editing = false;
                            } else {
                                formInputs["id"] = std::to_string(currentSchema.nextId);

                                RecordPointer ptr = sm->insertRecord(formInputs);
                                for (auto const& [field, idx] : index_map[selectedTable]) {
                                    if (formInputs.count(field)) idx->insert(formInputs[field], ptr);
                                }

                                saveSchema(selectedTable, currentSchema.nextId + 1, currentSchema.fields);

                                pushLog(ui::strf(ui::S::LogInsertOk, formInputs["id"]), sf::Color::Green);
                            }
                            formInputs.clear();
                            refreshUI();
                        } catch (const std::exception& e) {
                            pushLog(std::string("[Error] ") + e.what(), sf::Color::Red);
                        }
                    }
                    if (editing) {
                        ImGui::SameLine();
                        if (ImGui::Button(ui::str(ui::S::CancelBtn).c_str(), ImVec2(100, 30))) {
                            editing = false;
                            formInputs.clear();
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(ui::str(ui::S::CompactBtn).c_str(), ImVec2(100, 30))) {
                        autoCompact(selectedTable, storage_units, index_map, registros_sucios);
                        pushLog(ui::str(ui::S::LogCompacted), sf::Color::Cyan);
                        refreshUI(); 
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(ui::str(ui::S::ExportCsvBtn).c_str(), ImVec2(120, 30))) {
                        // El CSV se escribe junto al .bin, en data/, con los
                        // campos del esquema en su orden y los vivos en memoria.
                        try {
                            std::vector<std::map<std::string, std::string>> rows;
                            rows.reserve(currentRecords.size());
                            for (const auto& row : currentRecords) rows.push_back(row.second);

                            // El 'id' no está en fieldsList (el formulario lo
                            // omite a propósito), pero en el CSV sí debe salir.
                            std::vector<std::string> columns = {"id"};
                            for (const auto& f : currentSchema.fieldsList) columns.push_back(f);

                            const std::string outPath = "data/" + selectedTable + ".csv";
                            std::ofstream out(outPath, std::ios::binary);
                            if (!out) throw std::runtime_error("no se pudo abrir " + outPath);
                            // BOM UTF-8: Excel en Windows lo necesita para no
                            // mostrar los acentos como caracteres extraños.
                            out << "\xEF\xBB\xBF" << csv::build(rows, columns);
                            if (!out) throw std::runtime_error("error al escribir " + outPath);

                            pushLog(ui::strf(ui::S::LogExported, selectedTable, outPath),
                                    sf::Color::Green);
                        } catch (const std::exception& e) {
                            pushLog(std::string("[Error] ") + e.what(), sf::Color::Red);
                        }
                    }

                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", ui::str(ui::S::SavedDocs).c_str());
                    ImGui::BeginChild("DocExplorer", ImVec2(0, 0), true);
                    
                    int doc_idx = 0;
                    for (auto& row : currentRecords) {
                        ImGui::PushID(doc_idx);
                        std::string header = "Doc [" + std::to_string(row.first.page_id) + ":" + std::to_string(row.first.slot_offset) + "]";
                        const std::string docId = row.second.count("id") ? row.second["id"] : "";
                        if (!docId.empty()) header += " | ID: " + docId;

                        if (ImGui::TreeNode((void*)(intptr_t)doc_idx, "%s", header.c_str())) {
                            for (auto const& [k, v] : row.second) {
                                ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.5f, 1.0f), "\"%s\":", k.c_str());
                                ImGui::SameLine();
                                ImGui::Text("\"%s\"", v.c_str());
                            }
                            ImGui::TreePop();
                        }

                        // Editar / Eliminar alineados a la derecha de la fila.
                        const ImGuiStyle& st = ImGui::GetStyle();
                        const float buttonsW =
                            ImGui::CalcTextSize(ui::str(ui::S::EditBtn).c_str()).x +
                            ImGui::CalcTextSize(ui::str(ui::S::DeleteBtn).c_str()).x +
                            st.FramePadding.x * 4.0f + st.ItemSpacing.x + 8.0f;
                        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - buttonsW);
                        if (ImGui::SmallButton(ui::str(ui::S::EditBtn).c_str())) {
                            formInputs = row.second;
                            formInputs.erase("id");
                            editPtr = row.first;
                            editId = docId;
                            editing = true;
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton(ui::str(ui::S::DeleteBtn).c_str())) {
                            deletePtr = row.first;
                            deleteId = docId;
                            showDeleteModal = true;
                        }

                        ImGui::Separator();
                        ImGui::PopID();
                        doc_idx++;
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                // ---------------- Estadísticas ----------------
                if (ImGui::BeginTabItem(ui::str(ui::S::TabStats).c_str())) {
                    const StorageStats st = storage_units[selectedTable]->getStats();
                    const float frag = (st.totalSlots > 0)
                        ? static_cast<float>(st.dead) / static_cast<float>(st.totalSlots)
                        : 0.0f;
                    const std::string bin_path = "data/" + selectedTable + ".bin";
                    std::error_code ec;
                    const auto bytes = fs::exists(bin_path, ec) ? fs::file_size(bin_path, ec) : 0;

                    ImGui::Text("%s %d", ui::str(ui::S::StatPages).c_str(), st.pages);
                    ImGui::Text("%s %zu", ui::str(ui::S::StatSlots).c_str(), st.totalSlots);
                    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "%s %zu",
                                       ui::str(ui::S::StatLive).c_str(), st.live);
                    ImGui::TextColored(ImVec4(0.95f, 0.65f, 0.25f, 1.0f), "%s %zu",
                                       ui::str(ui::S::StatDead).c_str(), st.dead);

                    char frag_buf[32];
                    snprintf(frag_buf, sizeof(frag_buf), "%.1f %%", frag * 100.0f);
                    ImGui::Text("%s", ui::str(ui::S::StatFrag).c_str());
                    ImGui::ProgressBar(frag, ImVec2(-1, 18), frag_buf);

                    ImGui::Text("%s %.1f KB", ui::str(ui::S::StatSize).c_str(),
                                static_cast<double>(bytes) / 1024.0);
                    ImGui::EndTabItem();
                }

                // ---------------- Índices ----------------
                if (ImGui::BeginTabItem(ui::str(ui::S::TabIndexes).c_str())) {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s",
                                       ui::str(ui::S::IndexListTitle).c_str());

                    std::string dropField;
                    for (auto& [field, mgr] : index_map[selectedTable]) {
                        ImGui::PushID(field.c_str());
                        ImGui::BulletText("%s — %s", field.c_str(),
                                          ui::strf(ui::S::IndexDegree, std::to_string(mgr->getDegree())).c_str());
                        // El índice 'id' es estructural (DELETE/UPDATE lo usan).
                        if (field != "id") {
                            ImGui::SameLine();
                            if (ImGui::SmallButton(ui::str(ui::S::IndexDropBtn).c_str())) dropField = field;
                        }
                        ImGui::PopID();
                    }
                    if (!dropField.empty()) {
                        delete index_map[selectedTable][dropField];
                        index_map[selectedTable].erase(dropField);
                        fs::remove("data/" + selectedTable + "_" + dropField + ".idx");
                        pushLog(ui::strf(ui::S::LogIndexDropped, dropField), sf::Color(240, 200, 80));
                    }

                    ImGui::Separator();
                    std::vector<std::string> candidates;
                    for (auto& f : currentSchema.fieldsList) {
                        if (!index_map[selectedTable].count(f)) candidates.push_back(f);
                    }

                    if (candidates.empty()) {
                        ImGui::TextDisabled("%s", ui::str(ui::S::IndexNoFields).c_str());
                    } else {
                        static int candidateIdx = 0;
                        if (candidateIdx >= static_cast<int>(candidates.size())) candidateIdx = 0;

                        ImGui::SetNextItemWidth(-120);
                        if (ImGui::BeginCombo("##idxfield", candidates[candidateIdx].c_str())) {
                            for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
                                if (ImGui::Selectable(candidates[i].c_str(), i == candidateIdx)) candidateIdx = i;
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(ui::str(ui::S::IndexCreateBtn).c_str())) {
                            std::map<std::string, std::string> ast = {
                                {"operation", "INDEX"},
                                {"field", candidates[candidateIdx]},
                                {"_table_target", selectedTable},
                            };
                            pushLog(executor.execute(ast), sf::Color::Cyan);
                            candidateIdx = 0;
                        }
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild(); // ##docsPane

        // --- Terminal (franja inferior del área de trabajo) ---------------
        ImGui::BeginChild("##terminalPane", ImVec2(0, 0), true);
        ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.0f, 1.0f), "%s", ui::str(ui::S::TerminalTitle).c_str());
        ImGui::BeginChild("Logs", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 4), false);
        for (auto& log : consoleLog)
            ImGui::TextColored(ImVec4(log.color.r/255.f, log.color.g/255.f, log.color.b/255.f, 1.0f),
                               "%s", log.text.c_str());
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();

        ImGui::PushItemWidth(-1);

        // Callback del historial: ↑ recorre hacia atrás, ↓ hacia adelante.
        struct HistCtx { std::vector<std::string>* hist; int* pos; };
        HistCtx histCtx{&cmdHistory, &cmdHistoryPos};
        const ImGuiInputTextCallback histCallback = [](ImGuiInputTextCallbackData* data) -> int {
            HistCtx* c = static_cast<HistCtx*>(data->UserData);
            if (data->EventFlag != ImGuiInputTextFlags_CallbackHistory || c->hist->empty()) return 0;

            const int prev = *c->pos;
            if (data->EventKey == ImGuiKey_UpArrow) {
                *c->pos = (*c->pos == -1) ? static_cast<int>(c->hist->size()) - 1
                                          : (*c->pos > 0 ? *c->pos - 1 : 0);
            } else if (data->EventKey == ImGuiKey_DownArrow) {
                if (*c->pos == -1) return 0;
                *c->pos += 1;
                if (*c->pos >= static_cast<int>(c->hist->size())) *c->pos = -1;
            }
            if (prev != *c->pos) {
                const std::string s = (*c->pos == -1) ? std::string() : (*c->hist)[*c->pos];
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, s.c_str());
            }
            return 0;
        };

        if (focusTerminal) { ImGui::SetKeyboardFocusHere(0); focusTerminal = false; }

        if (ImGui::InputText("##t", terminalBuffer, 1024,
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
                             histCallback, &histCtx)) {
            ejecutarComando(std::string(terminalBuffer));
            memset(terminalBuffer, 0, 1024);
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::PopItemWidth();
        ImGui::EndChild(); // ##terminalPane

        ImGui::EndTable();
        }
        ImGui::EndChild(); // ##body

        // --- Barra de estado ----------------------------------------------
        {
            const std::string binName = selectedTable.empty() ? "" : selectedTable + ".bin";
            ImGui::TextDisabled("%s", ui::strf(ui::S::StatusData, binName).c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("·");
            ImGui::SameLine();
            ImGui::TextDisabled("%s", ui::strf(ui::S::StatusCollections,
                                               std::to_string(storage_units.size())).c_str());
            const char* langTag = (nexo::lang() == nexo::Lang::ES) ? "ES" : "EN";
            const float tagW = ImGui::CalcTextSize(langTag).x;
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - tagW);
            ImGui::TextDisabled("%s", langTag);
        }

        if (showCreateModal) {
            ImGui::OpenPopup(ui::str(ui::S::CreateModalTitle).c_str());
            showCreateModal = false;
            createInvalid = false;
        }

        if (ImGui::BeginPopupModal(ui::str(ui::S::CreateModalTitle).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", ui::str(ui::S::NameLabel).c_str()); 
            ImGui::InputText("##name", nName, 64);
            
            ImGui::Text("%s", ui::str(ui::S::FieldsLabel).c_str());
            ImGui::InputTextWithHint("##fields", ui::str(ui::S::FieldsHint).c_str(), nFields, 256);

            if (createInvalid) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", ui::str(ui::S::CreateInvalid).c_str());
            }

            ImGui::Separator();
            if (ImGui::Button(ui::str(ui::S::OkBtn).c_str(), ImVec2(120, 0))) {
                if (strlen(nName) > 0 && validateCollectionName(nName) && validateSchemaFields(nFields)) {
                    std::string finalFields = "id";
                    if (strlen(nFields) > 0) finalFields += "," + std::string(nFields);

                    saveSchema(nName, 1, finalFields);
                    storage_units[nName] = new StorageManager("data/" + std::string(nName) + ".bin");
                    registros_sucios[nName] = 0;
                    
                    index_map[nName]["id"] = new IndexManager("data/" + std::string(nName) + "_id.idx", 3);
                    
                    pushLog(ui::strf(ui::S::LogCollectionReady, nName), sf::Color::Green);
                    memset(nName, 0, 64); memset(nFields, 0, 256);
                    ImGui::CloseCurrentPopup();
                } else {
                    createInvalid = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(ui::str(ui::S::CancelBtn).c_str(), ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // --- Modal GUÍA -----------------------------------------------------
        if (showGuide) {
            ImGui::OpenPopup(ui::str(ui::S::GuideTitle).c_str());
            showGuide = false;
        }

        if (ImGui::BeginPopupModal(ui::str(ui::S::GuideTitle).c_str(), NULL,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            if (demoCloseGuide) { ImGui::CloseCurrentPopup(); demoCloseGuide = false; }
            ImGui::BeginChild("##guideBody", ImVec2(700, 420), true);
            if (ImGui::BeginTabBar("##guideTabs")) {
                if (ImGui::BeginTabItem(ui::str(ui::S::GuideTabStart).c_str())) {
                    ImGui::TextWrapped("%s", ui::str(ui::S::GuideStartText).c_str());
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(ui::str(ui::S::GuideTabCmds).c_str())) {
                    if (ImGui::BeginTable("##guideCmds", 3,
                                          ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
                                          ImGuiTableFlags_ScrollY)) {
                        ImGui::TableSetupScrollFreeze(0, 1);
                        ImGui::TableSetupColumn(ui::str(ui::S::GuideColCmd).c_str(),
                                                ImGuiTableColumnFlags_WidthFixed, 280.0f);
                        ImGui::TableSetupColumn(ui::str(ui::S::GuideColDesc).c_str(),
                                                ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("##go",
                                                ImGuiTableColumnFlags_NoHide |
                                                ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableHeadersRow();

                        for (const auto& c : kGuideCmds) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextWrapped("%s", c.syntax);
                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextWrapped("%s", ui::str(c.desc).c_str());
                            ImGui::TableSetColumnIndex(2);
                            if (ImGui::SmallButton(ui::str(ui::S::GuideTryBtn).c_str())) {
                                snprintf(terminalBuffer, sizeof(terminalBuffer), "%s", c.example);
                                focusTerminal = true;
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(ui::str(ui::S::GuideTabKeys).c_str())) {
                    ImGui::TextWrapped("%s", ui::str(ui::S::GuideKeysText).c_str());
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndChild();

            ImGui::Separator();
            if (ImGui::Button(ui::str(ui::S::CancelBtn).c_str(), ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // --- Modal de confirmación de borrado ------------------------------
        if (showDeleteModal) {
            ImGui::OpenPopup(ui::str(ui::S::ConfirmDeleteTitle).c_str());
            showDeleteModal = false;
        }

        if (ImGui::BeginPopupModal(ui::str(ui::S::ConfirmDeleteTitle).c_str(), NULL,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(ui::strf(ui::S::ConfirmDeleteText, deleteId).c_str());
            ImGui::Spacing();

            if (ImGui::Button(ui::str(ui::S::ConfirmYes).c_str(), ImVec2(120, 0))) {
                auto* sm = storage_units.count(selectedTable) ? storage_units[selectedTable] : nullptr;
                if (sm && !sm->getRecord(deletePtr).empty()) {
                    // Borrado lógico: la compactación recupera el espacio.
                    sm->logicalDelete(deletePtr);
                    registros_sucios[selectedTable]++;
                    pushLog(ui::strf(ui::S::LogDeleteOk, deleteId), sf::Color(255, 170, 90));
                }
                // Si estábamos editando ese mismo documento, salimos del modo.
                if (editing && editPtr.page_id == deletePtr.page_id &&
                    editPtr.slot_offset == deletePtr.slot_offset) {
                    editing = false;
                    formInputs.clear();
                }
                deletePtr = { -1, -1 };
                refreshUI();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(ui::str(ui::S::CancelBtn).c_str(), ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::End();
        window.clear(sf::Color(25, 25, 28));
        ImGui::SFML::Render(window);
        // Captura del frame renderizado (modo demo): la textura debe tener
        // el tamaño de la ventana antes de copiar el framebuffer.
        if (!pendingShot.empty()) {
            sf::Texture tex(window.getSize());
            tex.update(window);
            sf::Image img = tex.copyToImage();
            if (!img.saveToFile("data/shots/" + pendingShot + ".png"))
                std::cerr << "[Nexo] no se pudo guardar la captura " << pendingShot << "\n";
            pendingShot.clear();
        }
        window.display();
    }

    for (auto& [n, p] : storage_units) delete p;
    for (auto& [tName, fMap] : index_map) {
        for (auto& [fName, p] : fMap) delete p;
    }
    
    ImGui::SFML::Shutdown();
    return 0;
}

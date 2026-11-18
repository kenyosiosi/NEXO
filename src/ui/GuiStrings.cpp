#include "ui/GuiStrings.h"
#include <array>
#include <fstream>
#include <filesystem>

namespace ui {
namespace {

struct Entry { const char* es; const char* en; };

// Tabla indexada por S. Los textos con {n} se formatean en strf().
const std::array<Entry, 71> kCatalog = {{
    /* GuideTitle        */ {"Guía de NEXO", "NEXO Guide"},
    /* GuideTabStart     */ {"Primeros pasos", "Getting started"},
    /* GuideTabCmds      */ {"Comandos", "Commands"},
    /* GuideTabKeys      */ {"Atajos", "Shortcuts"},
    /* GuideColCmd       */ {"Comando", "Command"},
    /* GuideColDesc      */ {"Descripción", "Description"},
    /* GuideTryBtn       */ {"Probar", "Try"},
    /* GuideStartText    */ {"1. Crea una colección con 'Nueva Colección +' o CREATE.\n"
                           "2. Inserta documentos desde el formulario o con INSERT.\n"
                           "3. Consulta con GET (usa un índice) o SCAN (todos los documentos).\n"
                           "4. INDEX crea índices B-Tree sobre cualquier campo y acelera GET.\n"
                           "5. DELETE y UPDATE funcionan en la terminal y en el visor de documentos.\n"
                           "6. STATS muestra la fragmentación; COMPACT la elimina reescribiendo el archivo.",
                           "1. Create a collection with 'New Collection +' or CREATE.\n"
                           "2. Insert documents from the form or with INSERT.\n"
                           "3. Query with GET (uses an index) or SCAN (all documents).\n"
                           "4. INDEX builds B-Tree indexes on any field and speeds up GET.\n"
                           "5. DELETE and UPDATE work in the terminal and in the document viewer.\n"
                           "6. STATS shows fragmentation; COMPACT removes it by rewriting the file."},
    /* GuideKeysText     */ {"Enter — ejecuta el comando de la terminal\n"
                           "↑ / ↓ — recorre el historial de comandos\n"
                           "? — abre esta guía\n"
                           "Esc — cierra el modal activo",
                           "Enter — run the terminal command\n"
                           "↑ / ↓ — browse command history\n"
                           "? — open this guide\n"
                           "Esc — close the active modal"},
    /* BannerText        */ {"¡Bienvenido a NEXO! Escribe HELP en la terminal o pulsa ? para ver la guía completa. "
                           "Puedes empezar jugando con la colección de demostración 'demo_usuarios'.",
                           "Welcome to NEXO! Type HELP in the terminal or press ? for the full guide. "
                           "You can start by playing with the 'demo_usuarios' demo collection."},
    /* BannerDismiss     */ {"Entendido", "Got it"},
    /* CollectionsExplorer*/ {"EXPLORADOR DE COLECCIONES", "COLLECTION EXPLORER"},
    /* NewCollectionBtn  */ {"Nueva Colección +", "New Collection +"},
    /* SelectPrompt      */ {"<- selecciona una colección para comenzar", "<- select a collection to begin"},
    /* ActiveCollection  */ {"COLECCIÓN ACTIVA: {0}", "ACTIVE COLLECTION: {0}"},
    /* NewDocument       */ {"Nuevo Documento:", "New Document:"},
    /* InsertBtn         */ {"INSERTAR", "INSERT"},
    /* UpdateBtn         */ {"ACTUALIZAR", "UPDATE"},
    /* CompactBtn        */ {"COMPACTAR", "COMPACT"},
    /* ExportCsvBtn      */ {"Exportar CSV", "Export CSV"},
    /* SavedDocs         */ {"DOCUMENTOS GUARDADOS:", "SAVED DOCUMENTS:"},
    /* TerminalTitle     */ {"TERMINAL NEXO", "NEXO TERMINAL"},
    /* CreateModalTitle  */ {"Configurar Nueva Colección", "New Collection Setup"},
    /* NameLabel         */ {"Nombre:", "Name:"},
    /* FieldsLabel       */ {"Campos:", "Fields:"},
    /* FieldsHint        */ {"Ej. nombre, edad, cedula", "e.g. name, age, ssn"},
    /* OkBtn             */ {"OK", "OK"},
    /* CancelBtn         */ {"Cancelar", "Cancel"},
    /* CreateInvalid     */ {"Nombre o campos inválidos (usa letras, números, '_' o '-').",
                           "Invalid name or fields (use letters, numbers, '_' or '-')."},
    /* LogInsertOk       */ {"Registro insertado con ID: {0}", "Record inserted with ID: {0}"},
    /* LogCompacted      */ {"Colección compactada físicamente y re-indexada.",
                           "Collection physically compacted and re-indexed."},
    /* LogCollectionReady*/ {"Colección '{0}' lista.", "Collection '{0}' ready."},
    /* LogDemoSeed       */ {"Colección de demostración 'demo_usuarios' creada (5 documentos).",
                           "Demo collection 'demo_usuarios' created (5 documents)."},
    /* LogLangEs         */ {"Idioma cambiado a Español.", "Language switched to Spanish."},
    /* LogLangEn         */ {"Idioma cambiado a Inglés.", "Language switched to English."},
    /* LogExported       */ {"Colección '{0}' exportada a {1}", "Collection '{0}' exported to {1}"},
    /* TabDocs           */ {"Documentos", "Documents"},
    /* TabStats          */ {"Estadísticas", "Statistics"},
    /* TabIndexes        */ {"Índices", "Indexes"},
    /* StatPages         */ {"Páginas:", "Pages:"},
    /* StatSlots         */ {"Slots totales:", "Total slots:"},
    /* StatLive          */ {"Registros vivos:", "Live records:"},
    /* StatDead          */ {"Registros muertos (tombstones):", "Dead records (tombstones):"},
    /* StatFrag          */ {"Fragmentación:", "Fragmentation:"},
    /* StatSize          */ {"Tamaño en disco:", "On-disk size:"},
    /* IndexListTitle    */ {"Índices activos:", "Active indexes:"},
    /* IndexCreateBtn    */ {"Crear índice", "Create index"},
    /* IndexDropBtn      */ {"Eliminar", "Drop"},
    /* IndexNoFields     */ {"Todos los campos ya tienen índice.", "All fields already have an index."},
    /* IndexDegree       */ {"Grado: {0}", "Degree: {0}"},
    /* EditBtn           */ {"Editar", "Edit"},
    /* DeleteBtn         */ {"Eliminar", "Delete"},
    /* ConfirmDeleteTitle*/ {"Eliminar documento", "Delete document"},
    /* ConfirmDeleteText */ {"¿Eliminar el documento con ID {0}?\nEl borrado es lógico (tombstone) y desaparece al compactar.",
                           "Delete the document with ID {0}?\nDeletion is logical (tombstone) and disappears on compaction."},
    /* ConfirmYes        */ {"Sí, eliminar", "Yes, delete"},
    /* EditModeHint      */ {"Editando doc {0} — modifica los campos y pulsa ACTUALIZAR.",
                           "Editing doc {0} — change fields and press UPDATE."},
    /* LogDeleteOk       */ {"Documento {0} eliminado lógicamente.", "Document {0} logically deleted."},
    /* LogUpdateOk       */ {"Documento {0} actualizado.", "Document {0} updated."},
    /* StatusData        */ {"data/: {0}", "data/: {0}"},
    /* StatusCollections */ {"{0} colecciones", "{0} collections"},
    /* CmdCreateDesc     */ {"Crea una colección y define sus campos. Índice 'id' autogenerado.",
                           "Creates a collection and its fields. 'id' index auto-generated."},
    /* CmdInsertDesc     */ {"Inserta un documento. Si omites el id, se asigna el siguiente de la secuencia.",
                           "Inserts a document. Without an id, the next sequence value is used."},
    /* CmdGetDesc        */ {"Busca un documento por un campo indexado (B-Tree).",
                           "Finds a document by an indexed field (B-Tree)."},
    /* CmdScanDesc       */ {"Lista todos los documentos vivos de la colección.",
                           "Lists every live document in the collection."},
    /* CmdUpdateDesc     */ {"Actualiza campos de un documento (borrado lógico + reinserción).",
                           "Updates document fields (logical delete + reinsertion)."},
    /* CmdDeleteDesc     */ {"Borrado lógico de un documento por id.",
                           "Logical delete of a document by id."},
    /* CmdIndexDesc      */ {"Crea un índice B-Tree sobre un campo (acelera GET).",
                           "Creates a B-Tree index on a field (speeds up GET)."},
    /* CmdStatsDesc      */ {"Métricas de almacenamiento: páginas, registros y fragmentación.",
                           "Storage metrics: pages, records and fragmentation."},
    /* CmdDefineDesc     */ {"Muestra campos, siguiente id e índices de la colección.",
                           "Shows fields, next id and indexes of the collection."},
    /* CmdDropDesc       */ {"Elimina la colección y todos sus archivos.",
                           "Drops the collection and all its files."},
    /* CmdHelpDesc       */ {"Muestra esta lista de comandos.",
                           "Shows this command list."},
}};

bool g_bannerSeen = false;

const Entry& entry(S id) {
    const size_t i = static_cast<size_t>(id);
    return (i < kCatalog.size()) ? kCatalog[i] : kCatalog[0];
}

std::string interpolate(const std::string& tpl, const std::string& a0, const std::string& a1) {
    std::string out;
    const std::string args[] = {a0, a1};
    for (size_t i = 0; i < tpl.size();) {
        if (tpl[i] == '{' && i + 2 < tpl.size() &&
            tpl[i + 1] >= '0' && tpl[i + 1] <= '9' && tpl[i + 2] == '}') {
            const int idx = tpl[i + 1] - '0';
            if (idx < 2) { out += args[idx]; i += 3; continue; }
        }
        out += tpl[i++];
    }
    return out;
}

} // namespace

std::string str(S id) {
    const Entry& e = entry(id);
    return (nexo::lang() == nexo::Lang::ES) ? e.es : e.en;
}

std::string strf(S id, const std::string& a0, const std::string& a1) {
    return interpolate(str(id), a0, a1);
}

void loadPrefs() {
    std::ifstream in("data/ui.cfg");
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("lang=", 0) == 0) {
            nexo::setLang(line == "lang=en" ? nexo::Lang::EN : nexo::Lang::ES);
        } else if (line.rfind("banner=", 0) == 0) {
            g_bannerSeen = (line == "banner=1");
        }
    }
}

void savePrefs() {
    std::filesystem::create_directory("data");
    std::ofstream out("data/ui.cfg");
    out << "lang=" << (nexo::lang() == nexo::Lang::EN ? "en" : "es") << "\n";
    out << "banner=" << (g_bannerSeen ? 1 : 0) << "\n";
}

bool bannerSeen() { return g_bannerSeen; }
void setBannerSeen(bool seen) { g_bannerSeen = seen; savePrefs(); }

void toggleLang(nexo::Lang l) { nexo::setLang(l); savePrefs(); }

} // namespace ui

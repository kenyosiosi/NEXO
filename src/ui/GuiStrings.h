#ifndef GUI_STRINGS_H
#define GUI_STRINGS_H

#include <string>
#include "Core/Messages.h"

namespace ui {

// Ids de los textos de la interfaz (solo el cliente gráfico). El motor habla
// por medio de nexo::Msg; esta tabla cubre etiquetas, botones y modales.
enum class S {
    // Barra superior / guía
    GuideTitle,
    GuideTabStart,
    GuideTabCmds,
    GuideTabKeys,
    GuideColCmd,
    GuideColDesc,
    GuideTryBtn,
    GuideStartText,
    GuideKeysText,
    BannerText,
    BannerDismiss,
    // Explorador
    CollectionsExplorer,
    NewCollectionBtn,
    SelectPrompt,
    ActiveCollection,      // {0} = nombre
    NewDocument,
    InsertBtn,
    UpdateBtn,
    CompactBtn,
    ExportCsvBtn,
    SavedDocs,
    // Terminal
    TerminalTitle,
    // Modales
    CreateModalTitle,
    NameLabel,
    FieldsLabel,
    FieldsHint,
    OkBtn,
    CancelBtn,
    CreateInvalid,
    // Log
    LogInsertOk,           // {0} = id
    LogCompacted,
    LogCollectionReady,    // {0} = nombre
    LogDemoSeed,
    LogLangEs,
    LogLangEn,
    LogExported,           // {0} = colección, {1} = ruta
    // Pestañas centrales (B)
    TabDocs,
    TabStats,
    TabIndexes,
    StatPages,
    StatSlots,
    StatLive,
    StatDead,
    StatFrag,
    StatSize,
    IndexListTitle,
    IndexCreateBtn,
    IndexDropBtn,
    IndexNoFields,
    IndexDegree,           // {0} = grado
    // CRUD (C)
    EditBtn,
    DeleteBtn,
    ConfirmDeleteTitle,
    ConfirmDeleteText,     // {0} = id
    ConfirmYes,
    EditModeHint,          // {0} = id
    LogDeleteOk,           // {0} = id
    LogUpdateOk,           // {0} = id
    // Layout (D)
    StatusData,            // {0} = ruta
    StatusCollections,     // {0} = cantidad
    // Comandos de la guía (descripciones)
    CmdCreateDesc,
    CmdInsertDesc,
    CmdGetDesc,
    CmdScanDesc,
    CmdUpdateDesc,
    CmdDeleteDesc,
    CmdIndexDesc,
    CmdStatsDesc,
    CmdDefineDesc,
    CmdDropDesc,
    CmdHelpDesc,
    LogIndexDropped,       // {0} = campo
    LogDocStale,
};

// Texto del id en el idioma activo de la UI.
std::string str(S id);
// Igual que str() pero sustituyendo marcadores {0}..{3}.
std::string strf(S id, const std::string& a0, const std::string& a1 = "");

// Preferencias de la UI, persistidas en data/ui.cfg: idioma y si el banner
// de bienvenida ya fue descartado.
void loadPrefs();
void savePrefs();
bool bannerSeen();
void setBannerSeen(bool seen);

// Cambia el idioma de la UI y del motor a la vez (un solo toggle).
void toggleLang(nexo::Lang l);

} // namespace ui

#endif // GUI_STRINGS_H

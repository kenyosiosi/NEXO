#include <iostream>
#include <cstdlib>
#include "StorageManager.h"

int main() {
    system("pause");
    // 1. Inicializar el motor de almacenamiento
    // Esto debería crear el archivo "database.bin" en la carpeta data/
    StorageManager engine("data/database.bin");

    std::cout << "--- Keny-DB Storage Test ---" << std::endl;

    // 2. Simular un JSON plano (lo que vendría del Tokenizer)
    std::map<std::string, std::string> record1;
    record1["id"] = "101";
    record1["nombre"] = "Keny";
    record1["puesto"] = "Storage Engineer";

    std::map<std::string, std::string> record2;
    record2["id"] = "102";
    record2["status"] = "active";
    record2["log"] = "Iniciando sistema de archivos binarios";

    // 3. Insertar registros
    std::cout << "Insertando Registro 1..." << std::endl;
    RecordPointer ptr1 = engine.insertRecord(record1);
    std::cout << "Guardado en -> Pagina: " << ptr1.page_id << ", Slot: " << ptr1.slot_offset << std::endl;

    std::cout << "\nInsertando Registro 2..." << std::endl;
    RecordPointer ptr2 = engine.insertRecord(record2);
    std::cout << "Guardado en -> Pagina: " << ptr2.page_id << ", Slot: " << ptr2.slot_offset << std::endl;

    std::cout << "\n--- Prueba finalizada con exito ---" << std::endl;

    system("pause");
    return 0;
}
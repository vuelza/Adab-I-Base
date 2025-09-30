// ===================================================================================
//  Ana Uygulama Başlatıcı
// ===================================================================================
//
//  YAPILAN GÜNCELLEMELER:
//  ---------------------
//  1. UYARI GİDERİLDİ: 'e' değişkeni artık hata mesajında kullanılıyor.
//
// ===================================================================================

#include <iostream>
#include <stdexcept>
#include "CreateSimpleScenario.hpp"
#include "prepareMuaseretInputs.hpp"
#include "3Dplotter.hpp"

int main() {
    try {
        SearchableDataset gercekYorunge = createAndSaveSimpleScenario("SimpleScenario.csv");
        MuaseretScenarioOutput muaseretGirdileri = prepareMuaseretInputs("SimpleScenario.csv");
        plot3D(gercekYorunge, muaseretGirdileri);
    }
    catch (const std::exception& e) {
        std::cerr << "Uygulama bir hatayla karsilasti: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}


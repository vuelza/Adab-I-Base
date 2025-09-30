// ===================================================================================
//  Senaryo Oluşturucu Arayüzü (Interface)
// ===================================================================================
//
//  YAPILAN GÜNCELLEMELER:
//  ---------------------
//  1. DÖNÜŞ TÜRÜ GÜNCELLENDİ: Fonksiyon artık eski "Coordinates" yapısı
//     yerine, yeni dinamik "SearchableDataset" yapısını döndürüyor.
//
// ===================================================================================

#ifndef CREATE_SIMPLE_SCENARIO_HPP
#define CREATE_SIMPLE_SCENARIO_HPP

#include "InteractivePlotter.hpp" // Yeni veri yapıları için
#include <string>

SearchableDataset createAndSaveSimpleScenario(const std::string& output_csv_filename);

#endif // CREATE_SIMPLE_SCENARIO_HPP


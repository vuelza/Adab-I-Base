#ifndef PLOTTER_3D_HPP
#define PLOTTER_3D_HPP

#include "DataStructures.hpp"
#include "prepareMuaseretInputs.hpp" // Bu dosyanın projenizde var olduğu varsayılmıştır.
#include "InteractivePlotter.hpp"

// Veriyi işleyen ve 3D çizdiriciyi başlatan ana fonksiyon.
void plot3D(const SearchableDataset& gercek_yorunge_data, const MuaseretScenarioOutput& muaseret_input);

#endif // PLOTTER_3D_HPP

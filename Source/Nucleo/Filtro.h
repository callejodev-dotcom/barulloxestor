#pragma once
#include "Produto.h"

namespace xestor
{
// O que se ve na xanela: a pestana (todo, instrumentos, efectos, paquetes) e o buscador.
enum class Pestana { todo, instrumentos, efectos, paquetes };

// Está o produto nesta pestana? (os de tipo descoñecido, só en «todo»)
bool casaPestana (const Produto&, Pestana);
// Casan todas as palabras da busca co nome, a descrición ou os plugins (sen tiles nin maiúsculas)?
// Unha busca baleira casa con todo.
bool casaBusca (const Produto&, const juce::String& busca);

// Cantos hai en cada pestana (cos que casan coa busca)
struct Reconto
{
    int todo = 0, instrumentos = 0, efectos = 0, paquetes = 0;
    int de (Pestana) const;
};
Reconto contar (const std::vector<Produto>&, const juce::String& busca = {});
} // namespace xestor

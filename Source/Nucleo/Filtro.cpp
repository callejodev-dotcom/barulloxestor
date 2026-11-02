#include "Filtro.h"

namespace xestor
{
bool casaPestana (const Produto& p, Pestana t)
{
    switch (t)
    {
        case Pestana::todo:         return true;
        case Pestana::instrumentos: return p.tipo == TipoProduto::instrumento;
        case Pestana::efectos:      return p.tipo == TipoProduto::efecto;
        case Pestana::paquetes:     return p.tipo == TipoProduto::paquete;
    }
    return true;
}

bool casaBusca (const Produto& p, const juce::String& busca)
{
    auto palabras = juce::StringArray::fromTokens (busca, " \t", "");
    palabras.removeEmptyStrings();
    if (palabras.isEmpty()) return true;
    // todo xunto e normalizado: «Barullo Opto Comp» → «barullooptocomp»
    const auto onde = normalizar (p.nome + " " + p.curto + " " + p.descricion + " " + p.plugins.joinIntoString (" "));
    for (auto& w : palabras)
    {
        const auto q = normalizar (w);
        if (q.isNotEmpty() && ! onde.contains (q)) return false;
    }
    return true;
}

int Reconto::de (Pestana t) const
{
    switch (t)
    {
        case Pestana::todo:         return todo;
        case Pestana::instrumentos: return instrumentos;
        case Pestana::efectos:      return efectos;
        case Pestana::paquetes:     return paquetes;
    }
    return 0;
}

Reconto contar (const std::vector<Produto>& ps, const juce::String& busca)
{
    Reconto r;
    for (auto& p : ps)
    {
        if (! casaBusca (p, busca)) continue;
        ++r.todo;
        r.instrumentos += casaPestana (p, Pestana::instrumentos) ? 1 : 0;
        r.efectos      += casaPestana (p, Pestana::efectos) ? 1 : 0;
        r.paquetes     += casaPestana (p, Pestana::paquetes) ? 1 : 0;
    }
    return r;
}
} // namespace xestor

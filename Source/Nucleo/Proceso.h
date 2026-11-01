#pragma once
#include <juce_core/juce_core.h>

namespace xestor
{
// Executa unha orde (ditto, codesign, tar, osascript...) e recolle a saída (stdout e stderr xuntos).
struct ResultadoProceso
{
    int codigo = -1;
    juce::String saida;
    bool arrancou = false;
    bool ok() const { return arrancou && codigo == 0; }
};

ResultadoProceso executar (const juce::StringArray& orde, int tempoMaximoMs = 10 * 60 * 1000);
// O primeiro programa que exista desta lista (ou buscado no PATH)
juce::String atoparPrograma (const juce::String& nome, const juce::StringArray& candidatos = {});
} // namespace xestor

#pragma once
#include "Produto.h"
#include <functional>

namespace xestor
{
struct Resposta
{
    int estado = 0;                      // 0: non houbo resposta (sen rede)
    juce::MemoryBlock corpo;
    juce::StringPairArray cabeceiras;    // sen distinguir maiúsculas
    juce::String erro;

    juce::String texto() const { return corpo.toString(); }
};

// Por onde se fala coa rede. O de verdade é TransporteHttp; as probas usan un GitHub de mentira.
class Transporte
{
public:
    virtual ~Transporte() = default;
    virtual Resposta pedir (const juce::String& url, const juce::StringPairArray& cabeceiras) = 0;
    // descarga a disco; progreso (0…1) devolve false para cancelar
    virtual juce::Result descargar (const juce::String& url, const juce::File& destino,
                                    std::function<bool (double)> progreso) = 0;
};

class TransporteHttp : public Transporte
{
public:
    Resposta pedir (const juce::String& url, const juce::StringPairArray& cabeceiras) override;
    juce::Result descargar (const juce::String& url, const juce::File& destino, std::function<bool (double)> progreso) override;
};

// Copia un file:// (ou unha ruta) a destino: para as probas e para o --zip local
juce::Result copiarLocal (const juce::String& url, const juce::File& destino);

// A suma SHA-256 dun ficheiro, en hexadecimal
juce::String sha256 (const juce::File&);
// Comproba tamaño e suma contra o que di GitHub
juce::Result verificar (const juce::File&, const Activo&);
// «12,3 MB»
juce::String tamanoLexible (juce::int64 bytes);
} // namespace xestor

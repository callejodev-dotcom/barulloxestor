#pragma once
#include "Produto.h"

namespace xestor
{
// As copias para volver atrás: ao actualizar, a versión de antes gárdase (copiada) antes de que vaia
// ao Lixo, e así pódese volver a ela sen rede (o menú ••• «Volver á 0.5.0»).
//   <cartafol>/<Curto>/<versión>/<formato>/<bundle>   (p. ex. copias/BarulloFM/0.5.0/vst3/Barullo FM.vst3)
// Gárdanse as «maximo» versións máis novas de cada produto; as outras quítanse (son copias do xestor).
class Copias
{
public:
    explicit Copias (juce::File c) : cartafol (std::move (c)) {}
    static juce::File estandar();   // <datos>/copias

    int maximo = 2;

    juce::Result gardar (const Produto&, Formato, const juce::File& bundle, const Version&);
    std::vector<Version> versions (const Produto&) const;   // a máis nova primeiro
    juce::File cartafolDe (const Produto&, const Version&) const;
    const juce::File& getCartafol() const { return cartafol; }

    // Copia un bundle (ou ficheiro) enteiro; en Mac con ditto (as firmas e as ligazóns)
    static juce::Result copiar (const juce::File& de, const juce::File& a);

private:
    void podar (const Produto&) const;
    juce::File cartafol;
};

// Os presets dos plugins: <Documentos>/<Nome do plugin>/Presets (o do núcleo de Escoita).
namespace presets
{
    juce::File documentos();
    // os cartafoles de presets que existen dos plugins dun produto
    juce::Array<juce::File> cartafoles (const Produto&, const juce::File& documentos = presets::documentos());
    // onde van as copias: <Documentos>/Barullo Xestor/Copias dos presets
    juce::File cartafolCopias (const juce::File& documentos = presets::documentos());
    // Garda os presets do produto nun .zip («Barullo FM presets 2026-11-02 10.30.zip»);
    // en «zip», o ficheiro feito. Falla se non hai presets.
    juce::Result copiaDeSeguridade (const Produto&, const juce::File& destino, juce::File& zip,
                                    const juce::File& documentos = presets::documentos(),
                                    juce::Time cando = juce::Time::getCurrentTime());
}
} // namespace xestor

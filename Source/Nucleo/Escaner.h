#pragma once
#include "Rutas.h"

namespace xestor
{
// Un bundle (ou ficheiro) instalado no disco.
struct Instalado
{
    juce::String plugin;
    Formato formato = Formato::vst3;
    Ubicacion ubicacion;
    juce::File ficheiro;
    std::optional<Version> version;
    juce::String textoVersion;   // tal como o di o bundle (baleiro: non se sabe)
    bool deEscoita = false;

    bool precisaAdmin() const { return ubicacion.eSistema() && ! ubicacion.escribible (formato); }
};

// O que instalou o xestor (ruta → versión), para os ficheiros que non din a súa versión
// (CLAP, LV2 e as apps en Windows e Linux). Só vale mentres o ficheiro non cambie.
class Rexistro
{
public:
    explicit Rexistro (juce::File f = {}) : ficheiro (std::move (f)) {}
    static juce::File estandar();
    juce::String version (const juce::File& f) const;
    void anotar (const juce::File& f, const juce::String& version);
private:
    juce::File ficheiro;
    mutable juce::CriticalSection bloqueo;
};

// Le a versión dun bundle: Contents/Info.plist (Mac), Contents/Resources/moduleinfo.json (VST3 en
// todos os sistemas) ou o rexistro do xestor.
namespace infoBundle
{
    juce::String versionPlist (const juce::File& bundle);
    juce::String identificadorPlist (const juce::String& plist);
    juce::String valorPlist (const juce::String& plist, const juce::String& chave);
    juce::String versionModuleInfo (const juce::File& bundle);
    bool vendedorEscoita (const juce::File& bundle);
}

class Escaner
{
public:
    Escaner (Rutas r, const Rexistro* rex = nullptr) : rutas (std::move (r)), rexistro (rex) {}

    std::optional<Instalado> ler (const juce::String& plugin, Formato, const Ubicacion&, const juce::String& binario = {}) const;
    // Todas as copias dos plugins dun produto (en todos os formatos e ubicacións).
    std::vector<Instalado> instalados (const Produto&) const;
    // Os de Escoita que non son de ningún produto coñecido (p. ex. un plugin compilado na casa).
    std::vector<Instalado> orfos (const std::vector<Produto>& conecidos) const;
    // Un produto «solto» para un bundle de Escoita sen repo coñecido (para poder desinstalalo).
    std::optional<Produto> produtoSolto (const juce::String& nome) const;

    const Rutas& getRutas() const { return rutas; }
private:
    Rutas rutas;
    const Rexistro* rexistro;
};
} // namespace xestor

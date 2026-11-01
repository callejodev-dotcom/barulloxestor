#pragma once
#include "Produto.h"
#include <map>

namespace xestor
{
// Un sitio onde poden estar os plugins: o do usuario (sen admin) ou un do sistema (precisa admin).
struct Ubicacion
{
    enum class Clase { usuario, sistema };
    Clase clase = Clase::usuario;
    SO so = soActual();
    std::map<Formato, juce::File> cartafoles;   // onde vai cada formato
    bool appEnSubcartafol = false;              // Windows (Inno Setup): Program Files\<Nome>\<Nome>.exe

    juce::File cartafol (Formato f) const;
    // o bundle (ou o ficheiro) dun plugin: «Barullo Tune.vst3», «Barullo Tune.exe», «barullo-tune»...
    juce::File ruta (const juce::String& plugin, Formato, const juce::String& binario = {}) const;
    // Pódese escribir sen admin?
    bool escribible (Formato) const;
    bool eSistema() const { return clase == Clase::sistema; }
    juce::String etiqueta() const { return eSistema() ? "sistema" : "usuario"; }
};

// O nome do ficheiro dun formato neste sistema («Nome.vst3», «Nome.app», «Nome.exe», «binario»)
juce::String nomeFicheiro (SO, const juce::String& plugin, Formato, const juce::String& binario = {});
// O formato dun ficheiro atopado nun .zip ou nun cartafol (nullopt: non é un plugin)
std::optional<Formato> formatoDoFicheiro (SO, const juce::File&);

// Onde mirar e onde instalar. A instalación vai sempre á ubicación do usuario.
struct Rutas
{
    Ubicacion usuario;
    std::vector<Ubicacion> sistema;

    std::vector<const Ubicacion*> todas() const;

    // As de verdade deste sistema:
    //   Mac:     ~/Library/Audio/Plug-Ins/{VST3,Components,CLAP} e ~/Applications; /Library/... e /Applications
    //   Windows: %LOCALAPPDATA%\Programs\Common\{VST3,CLAP}; C:\Program Files\Common Files\{VST3,CLAP}
    //   Linux:   ~/.vst3, ~/.clap, ~/.lv2, ~/.local/bin; /usr/local/lib/... e /usr/lib/...
    static Rutas reais (SO = soActual());
    // Un cartafol calquera (probas ou --dir): <dir>/VST3, /Components, /CLAP, /LV2 e /Applications.
    static Ubicacion cartafol (const juce::File& dir, Ubicacion::Clase = Ubicacion::Clase::usuario, SO = soActual());
};
} // namespace xestor

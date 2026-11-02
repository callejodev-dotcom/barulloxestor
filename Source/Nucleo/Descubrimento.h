#pragma once
#include "GitHub.h"

namespace xestor
{
// O cartafol dos datos do xestor (axustes e rexistro):
//   Mac ~/Library/Application Support/Barullo Xestor · Windows %APPDATA%\Barullo Xestor · Linux ~/.config/Barullo Xestor
juce::File cartafolDatos();

// Os axustes (axustes.json no cartafol dos datos; todos opcionais).
struct Axustes
{
    juce::String propietario = "callejodev-dotcom";   // a conta de GitHub onde están os plugins
    juce::StringArray reposExtra;                       // «propietario/nome» doutras contas
    juce::StringArray reposOcultos;                     // os que non se queren ver
    juce::StringArray cartafolesLocais { "~/Downloads" };   // onde buscar repos con packaging/produto.env
    std::vector<Formato> formatos { Formato::vst3 };    // os que se instalan e se miran
    bool usarGh = true;                                 // usar `gh auth token` se o gh ten sesión
    juce::String idioma;                                // «gl», «es», «en»; baleiro: o do sistema
    bool bandeja = false;                               // a icona na barra de menús / bandeja (comprobar cada día)
    bool copiaPresets = false;                          // copia dos presets antes de actualizar
    juce::int64 ultimaComprobacion = 0;                 // ms (a última vez que se preguntou a GitHub)
    juce::StringArray avisadas;                         // «Barullo FM 0.5.1»: as versións novas xa notificadas

    static juce::File ficheiroEstandar() { return cartafolDatos().getChildFile ("axustes.json"); }
    static Axustes cargar (const juce::File& = ficheiroEstandar());
    bool gardar (const juce::File& = ficheiroEstandar()) const;
    juce::Array<juce::File> urlsLocais() const;
};

// O que se sabe dos plugins: os produtos (de GitHub e locais) e os avisos que houbo polo camiño.
struct Catalogo
{
    std::vector<Produto> produtos;
    juce::StringArray avisos;
    juce::Time data = juce::Time::getCurrentTime();
    bool senRede = false;   // non se puido falar con GitHub nin había nada gardado

    const Produto* buscar (const juce::String& consulta) const;
};

// Descobre os plugins sen ter unha lista escrita no código: un repo é un plugin de Escoita se ten
// packaging/produto.env (o ficheiro do núcleo). Así un plugin novo aparece só ao publicalo, e un
// que se está a facer na casa (con ese ficheiro) aparece coma «só local».
class Descubridor
{
public:
    Descubridor (GitHub* gh, Axustes a) : github (gh), axustes (std::move (a)) {}
    Catalogo descubrir (bool forzar = false);

    // os subcartafoles con packaging/produto.env
    static std::vector<Produto> locais (const juce::Array<juce::File>& cartafoles);
    // a versión do project(... VERSION x.y.z) do CMakeLists
    static std::optional<Version> versionCMake (const juce::File&);
    // as capturas dun repo local (as mesmas rutas ca en GitHub, no disco)
    static juce::StringArray capturasLocais (const juce::File& repo, const juce::StringArray& declaradas);
private:
    GitHub* github;
    Axustes axustes;
};
} // namespace xestor

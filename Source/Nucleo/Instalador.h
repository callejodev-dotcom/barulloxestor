#pragma once
#include "Escaner.h"
#include "Rede.h"
#include "Papeleira.h"
#include "Admin.h"
#include <stdexcept>

namespace xestor
{
// O que pasou con cada bundle nunha instalación ou desinstalación.
struct Accion
{
    enum class Tipo { instalado, actualizado, omitido, aoLixo, aviso, erro };
    Tipo tipo;
    juce::String plugin;
    std::optional<Formato> formato;
    juce::String mensaxe;
    juce::String texto() const;
};
juce::String clave (Accion::Tipo);

struct Informe
{
    std::vector<Accion> accions;
    bool ok() const;
    int cambios() const;
    int conAdmin = 0;   // cantas cousas precisaron o permiso de administrador
    bool cancelouAdmin = false;
    void engadir (Accion::Tipo t, const juce::String& plugin, std::optional<Formato> f, const juce::String& m)
    {
        accions.push_back ({ t, plugin, f, m });
    }
    void xuntar (const Informe& o);
};

struct ErroInstalacion : std::runtime_error
{
    using std::runtime_error::runtime_error;
    juce::String texto() const { return juce::String::fromUTF8 (what()); }
};

// Un bundle atopado nun arquivo descargado
struct Atopado
{
    juce::String plugin;
    Formato formato;
    juce::File ficheiro;
};

// Instala, actualiza e desinstala. Instala sempre no cartafol do usuario (sen admin); o que se
// substitúe ou se quita vai ao Lixo, nunca se borra. O que está nun cartafol do sistema sácase
// co permiso de administrador (o Elevador) e despois vai ao Lixo coma o resto.
class Instalador
{
public:
    Instalador (Rutas rutas, const Papeleira& papeleira, Elevador* elevador, Transporte& transporte,
                juce::File temporal = cartafolTemporal(), Rexistro* rexistro = nullptr, SO so = soActual());

    static juce::File cartafolTemporal();   // un cartafol temporal neutro, fóra de ~/Downloads

    // trata os cartafoles do sistema coma se non se puidese escribir neles sen permiso (probas)
    bool simularAdmin = false;
    // ao instalar unha versión nova, levar ao Lixo as copias máis vellas dos cartafoles do sistema
    bool quitarVellasDoSistema = true;

    // Descarga o arquivo deste sistema da última versión, compróbao e instala os formatos pedidos.
    Informe instalar (const Produto&, const std::vector<Formato>&, bool forzar = false,
                      std::function<bool (double)> progreso = {});
    // Instala desde un arquivo xa descargado (.zip ou .tar.gz).
    Informe instalarArquivo (const juce::File&, const Produto&, const std::vector<Formato>&,
                             std::optional<Version> esperada, bool forzar = false);
    // Leva ao Lixo as copias dos formatos pedidos (as do sistema só con incluirSistema).
    Informe desinstalar (const Produto&, const std::vector<Formato>&, bool incluirSistema);
    // Leva ao Lixo unhas copias concretas (p. ex. a vella de /Library cando a do usuario xa está ao día).
    Informe botar (const std::vector<Instalado>&);

    static std::vector<Atopado> bundles (const juce::File& dir, SO, const Produto&);
    static juce::Result extraer (const juce::File& arquivo, const juce::File& destino, SO);

    // Mac: cuarentena e firma
    static void quitarCuarentena (const juce::File&);
    static bool temCuarentena (const juce::File&);
    static bool firmaValida (const juce::File&);
    // se a firma non vale, asina «ad hoc»; devolve un aviso se houbo que asinar
    static juce::Result asegurarFirma (const juce::File&, juce::String& aviso);

private:
    struct ParaOLixo { juce::File ficheiro; juce::String plugin; std::optional<Formato> formato; bool sistema; };
    // Leva ao Lixo; o que non se poida sen permiso, sácao co Elevador (un só permiso para todo)
    void botarVarios (const std::vector<ParaOLixo>&, Informe&, const juce::String& motivo);
    void instalarBundle (const Atopado&, const Produto&, std::optional<Version> esperada, bool forzar, Informe&,
                         std::vector<ParaOLixo>& vellasSistema);
    bool sistemaBloqueado (const Ubicacion&, Formato) const;

    Rutas rutas;
    const Papeleira& papeleira;
    Elevador* elevador;
    Transporte& transporte;
    juce::File temporal;
    Rexistro* rexistro;
    SO so;
};
} // namespace xestor

#include "Estado.h"

namespace xestor
{
juce::String clave (Estado e)
{
    switch (e)
    {
        case Estado::alDia:         return "alDia";
        case Estado::actualizacion: return "actualizacion";
        case Estado::copiaVella:    return "copiaVella";
        case Estado::incompleto:    return "incompleto";
        case Estado::nonInstalado:  return "nonInstalado";
        case Estado::soLocal:       return "soLocal";
        case Estado::senDatos:      return "senDatos";
    }
    return {};
}

juce::String etiqueta (Estado e)
{
    switch (e)
    {
        case Estado::alDia:         return juce::CharPointer_UTF8 ("ao día");
        case Estado::actualizacion: return juce::CharPointer_UTF8 ("actualización");
        case Estado::copiaVella:    return "copia vella no sistema";
        case Estado::incompleto:    return "incompleto";
        case Estado::nonInstalado:  return "non instalado";
        case Estado::soLocal:       return juce::CharPointer_UTF8 ("só local");
        case Estado::senDatos:      return "sen datos";
    }
    return {};
}

std::vector<Instalado> Situacion::relevantes() const
{
    std::vector<Instalado> r;
    for (auto& i : instalados)
        if (std::find (formatos.begin(), formatos.end(), i.formato) != formatos.end()) r.push_back (i);
    return r;
}

std::vector<Instalado> Situacion::vellas() const
{
    std::vector<Instalado> r;
    if (! produto.ultima) return r;
    const auto u = produto.ultima->version();
    if (! u) return r;
    for (auto& i : relevantes())
        if (! i.version || *i.version < *u) r.push_back (i);
    return r;
}

std::vector<Instalado> Situacion::vellasDoSistema() const
{
    std::vector<Instalado> r;
    for (auto& i : vellas())
        if (i.ubicacion.eSistema()) r.push_back (i);
    return r;
}

juce::StringArray Situacion::faltan() const
{
    juce::StringArray con, r;
    for (auto& i : relevantes()) con.addIfNotAlreadyThere (i.plugin);
    if (con.isEmpty()) return r;
    for (auto& p : produto.plugins)
        if (! con.contains (p)) r.add (p);
    return r;
}

std::vector<Instalado> Situacion::duplicados() const
{
    const auto rel = relevantes();
    std::vector<Instalado> r;
    for (auto& a : rel)
        for (auto& b : rel)
            if (a.plugin == b.plugin && a.formato == b.formato && a.ubicacion.clase != b.ubicacion.clase) { r.push_back (a); break; }
    return r;
}

std::optional<Version> Situacion::versionInstalada() const
{
    std::optional<Version> m;
    for (auto& i : relevantes())
        if (i.version && (! m || *i.version < *m)) m = i.version;
    return m;
}

Estado Situacion::estado() const
{
    if (! produto.ultima || ! produto.ultima->version())
    {
        // sen versión publicada: ou non está en GitHub, ou aínda non ten ningunha (só local);
        // se non se puido preguntar, non se sabe
        return produto.repo.isEmpty() || produto.lanzamentoConsultado ? Estado::soLocal : Estado::senDatos;
    }
    const auto rel = relevantes();
    if (rel.empty()) return Estado::nonInstalado;
    const auto v = vellas();
    if (! v.empty())
    {
        // se o único vello é unha copia do sistema e no cartafol do usuario xa está a nova, non hai
        // que descargar nada: só quitar esa copia
        bool soSistema = true;
        for (auto& x : v)
        {
            bool novaNoUsuario = false;
            for (auto& y : rel)
                if (! y.ubicacion.eSistema() && y.plugin == x.plugin && y.formato == x.formato) novaNoUsuario = true;
            if (! x.ubicacion.eSistema() || ! novaNoUsuario) { soSistema = false; break; }
        }
        return soSistema ? Estado::copiaVella : Estado::actualizacion;
    }
    if (! faltan().isEmpty()) return Estado::incompleto;
    return Estado::alDia;
}
} // namespace xestor

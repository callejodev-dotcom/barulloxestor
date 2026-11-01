#include "Rutas.h"

namespace xestor
{
juce::File Ubicacion::cartafol (Formato f) const
{
    auto i = cartafoles.find (f);
    return i != cartafoles.end() ? i->second : juce::File();
}

juce::String nomeFicheiro (SO so, const juce::String& plugin, Formato f, const juce::String& binario)
{
    switch (f)
    {
        case Formato::vst3: return plugin + ".vst3";
        case Formato::au:   return plugin + ".component";
        case Formato::clap: return plugin + ".clap";
        case Formato::lv2:  return plugin + ".lv2";
        case Formato::app:
            if (so == SO::mac) return plugin + ".app";
            if (so == SO::windows) return plugin + ".exe";
            return binario.isNotEmpty() ? binario : plugin.toLowerCase().replace (" ", "-");
    }
    return plugin;
}

std::optional<Formato> formatoDoFicheiro (SO so, const juce::File& f)
{
    const auto ext = f.getFileExtension().toLowerCase();
    if (ext == ".vst3") return Formato::vst3;
    if (ext == ".component" && so == SO::mac) return Formato::au;
    if (ext == ".clap") return Formato::clap;
    if (ext == ".lv2" && so == SO::linux) return Formato::lv2;
    if (ext == ".app" && so == SO::mac && f.isDirectory()) return Formato::app;
    if (ext == ".exe" && so == SO::windows) return Formato::app;
    return std::nullopt;   // a app de Linux (sen extensión) recoñécea o instalador polo BINARIO
}

juce::File Ubicacion::ruta (const juce::String& plugin, Formato f, const juce::String& binario) const
{
    auto c = cartafol (f);
    if (f == Formato::app && appEnSubcartafol) c = c.getChildFile (plugin);
    return c.getChildFile (nomeFicheiro (so, plugin, f, binario));
}

bool Ubicacion::escribible (Formato f) const
{
    auto c = cartafol (f);
    // se o cartafol aínda non existe, mírase o primeiro pai que si
    while (c != juce::File() && ! c.exists() && c.getParentDirectory() != c) c = c.getParentDirectory();
    return c.exists() && c.hasWriteAccess();
}

std::vector<const Ubicacion*> Rutas::todas() const
{
    std::vector<const Ubicacion*> r { &usuario };
    for (auto& s : sistema) r.push_back (&s);
    return r;
}

Ubicacion Rutas::cartafol (const juce::File& dir, Ubicacion::Clase clase, SO so)
{
    Ubicacion u;
    u.clase = clase;
    u.so = so;
    u.cartafoles[Formato::vst3] = dir.getChildFile ("VST3");
    u.cartafoles[Formato::au]   = dir.getChildFile ("Components");
    u.cartafoles[Formato::clap] = dir.getChildFile ("CLAP");
    u.cartafoles[Formato::lv2]  = dir.getChildFile ("LV2");
    u.cartafoles[Formato::app]  = dir.getChildFile ("Applications");
    return u;
}

Rutas Rutas::reais (SO so)
{
    Rutas r;
    const auto casa = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    r.usuario.so = so;
    if (so == SO::mac)
    {
        const auto plug = casa.getChildFile ("Library/Audio/Plug-Ins");
        r.usuario.cartafoles = { { Formato::vst3, plug.getChildFile ("VST3") }, { Formato::au, plug.getChildFile ("Components") },
                                 { Formato::clap, plug.getChildFile ("CLAP") }, { Formato::app, casa.getChildFile ("Applications") } };
        Ubicacion s;
        s.clase = Ubicacion::Clase::sistema; s.so = so;
        const juce::File sp ("/Library/Audio/Plug-Ins");
        s.cartafoles = { { Formato::vst3, sp.getChildFile ("VST3") }, { Formato::au, sp.getChildFile ("Components") },
                         { Formato::clap, sp.getChildFile ("CLAP") }, { Formato::app, juce::File ("/Applications") } };
        r.sistema.push_back (s);
    }
    else if (so == SO::windows)
    {
        // VST3 e CLAP por usuario: %LOCALAPPDATA%\Programs\Common\... (as rutas oficiais das dúas normas)
        auto local = juce::File (juce::SystemStats::getEnvironmentVariable ("LOCALAPPDATA", casa.getChildFile ("AppData\\Local").getFullPathName()));
        auto common = local.getChildFile ("Programs").getChildFile ("Common");
        r.usuario.cartafoles = { { Formato::vst3, common.getChildFile ("VST3") }, { Formato::clap, common.getChildFile ("CLAP") },
                                 { Formato::app, local.getChildFile ("Programs").getChildFile ("Escoita") } };
        Ubicacion s;
        s.clase = Ubicacion::Clase::sistema; s.so = so;
        auto pf = juce::File (juce::SystemStats::getEnvironmentVariable ("ProgramFiles", "C:\\Program Files"));
        auto cf = juce::File (juce::SystemStats::getEnvironmentVariable ("CommonProgramFiles", pf.getChildFile ("Common Files").getFullPathName()));
        s.cartafoles = { { Formato::vst3, cf.getChildFile ("VST3") }, { Formato::clap, cf.getChildFile ("CLAP") }, { Formato::app, pf } };
        s.appEnSubcartafol = true;   // o instalador de Inno Setup: Program Files\<Nome>\<Nome>.exe
        r.sistema.push_back (s);
    }
    else
    {
        r.usuario.cartafoles = { { Formato::vst3, casa.getChildFile (".vst3") }, { Formato::clap, casa.getChildFile (".clap") },
                                 { Formato::lv2, casa.getChildFile (".lv2") }, { Formato::app, casa.getChildFile (".local/bin") } };
        for (auto raiz : { "/usr/local", "/usr" })
        {
            Ubicacion s;
            s.clase = Ubicacion::Clase::sistema; s.so = so;
            const juce::File lib (juce::String (raiz) + "/lib");
            s.cartafoles = { { Formato::vst3, lib.getChildFile ("vst3") }, { Formato::clap, lib.getChildFile ("clap") },
                             { Formato::lv2, lib.getChildFile ("lv2") }, { Formato::app, juce::File (juce::String (raiz) + "/bin") } };
            r.sistema.push_back (s);
        }
    }
    return r;
}
} // namespace xestor

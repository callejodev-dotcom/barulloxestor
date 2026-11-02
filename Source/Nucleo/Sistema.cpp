#include "Sistema.h"
#include "Proceso.h"

namespace xestor::sistema
{
namespace
{
// Un DAW: o nome que se amosa e como se recoñece o seu proceso (sen distinguir maiúsculas):
// «exactos», o nome do executable (sen .exe); «prefixos», o comezo do executable ou do .app
struct Daw
{
    const char* nome;
    std::vector<const char*> exactos, prefixos;
};

const std::vector<Daw>& lista()
{
    static const std::vector<Daw> l {
        { "Ableton Live",      {},                          { "ableton live" } },
        { "Logic Pro",         {},                          { "logic pro" } },
        { "REAPER",            { "reaper", "reaper64" },    {} },
        { "Bitwig Studio",     { "bitwig-studio" },         { "bitwig studio", "bitwigstudio", "bitwigaudioengine" } },
        { "Cubase",            {},                          { "cubase" } },
        { "Nuendo",            {},                          { "nuendo" } },
        { "FL Studio",         { "fl", "fl64", "osxfl" },   { "fl studio" } },
        { "Studio One",        {},                          { "studio one" } },
        { "Pro Tools",         {},                          { "pro tools", "protools" } },
        { "GarageBand",        {},                          { "garageband" } },
        { "MainStage",         {},                          { "mainstage" } },
        { "Reason",            { "reason" },                { "reason 1" } },
        { "Ardour",            {},                          { "ardour" } },
        { "Mixbus",            {},                          { "mixbus" } },
        { "Tracktion Waveform", {},                         { "waveform" } },
        { "Renoise",           {},                          { "renoise" } },
        { "Digital Performer", {},                          { "digital performer" } },
        { "Audacity",          { "audacity" },              {} },
        { "Carla",             { "carla" },                 {} },
        { "LMMS",              { "lmms" },                  {} },
    };
    return l;
}

bool casa (const Daw& d, const juce::String& base, const juce::StringArray& apps)
{
    for (auto* e : d.exactos)
        if (base == e) return true;
    for (auto* p : d.prefixos)
    {
        if (base.startsWith (p)) return true;
        for (auto& a : apps) if (a.startsWith (p)) return true;
    }
    return false;
}
} // namespace

juce::StringArray todosOsDaws()
{
    juce::StringArray r;
    for (auto& d : lista()) r.add (d.nome);
    return r;
}

juce::StringArray daws (const juce::StringArray& procesos)
{
    std::vector<bool> aberto (lista().size(), false);
    for (auto linha : procesos)
    {
        linha = linha.trim();
        if (linha.isEmpty()) continue;
        // Windows (tasklist en CSV): o primeiro campo, entre comiñas (o nome pode levar espazos)
        const bool csv = linha.startsWithChar ('"');
        if (csv) linha = linha.substring (1).upToFirstOccurrenceOf ("\"", false, false);
        auto l = linha.toLowerCase().replaceCharacter ('\\', '/');
        // os .app da ruta (Mac): «/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live»
        juce::StringArray apps;
        for (auto& parte : juce::StringArray::fromTokens (l, "/", ""))
            if (parte.endsWith (".app")) apps.add (parte.dropLastCharacters (4));
        // Linux (args): o programa, sen os argumentos
        if (! csv && apps.isEmpty()) l = l.upToFirstOccurrenceOf (" ", false, false);
        // o executable: o último anaco da ruta, sen .exe
        auto base = l.fromLastOccurrenceOf ("/", false, false).trim();
        if (base.endsWith (".exe")) base = base.dropLastCharacters (4);
        for (size_t i = 0; i < lista().size(); ++i)
            if (! aberto[i] && casa (lista()[i], base, apps)) aberto[i] = true;
    }
    juce::StringArray r;
    for (size_t i = 0; i < lista().size(); ++i)
        if (aberto[i]) r.add (lista()[i].nome);
    return r;
}

juce::StringArray procesos()
{
    juce::StringArray r;
   #if JUCE_MAC
    r.addLines (executar ({ "/bin/ps", "-axo", "comm=" }, 5000).saida);
   #elif JUCE_WINDOWS
    const auto sys = juce::File::getSpecialLocation (juce::File::windowsSystemDirectory);
    r.addLines (executar ({ sys.getChildFile ("tasklist.exe").getFullPathName(), "/fo", "csv", "/nh" }, 8000).saida);
   #else
    const auto ps = atoparPrograma ("ps", { "/bin/ps", "/usr/bin/ps" });
    if (ps.isNotEmpty())
    {
        r.addLines (executar ({ ps, "-eo", "comm=" }, 5000).saida);
        r.addLines (executar ({ ps, "-eo", "args=" }, 5000).saida);
    }
   #endif
    r.removeEmptyStrings();
    return r;
}

juce::StringArray dawsAbertos() { return daws (procesos()); }

// ============================================================== notificacións
juce::StringArray ordeNotificacion (SO so, const juce::String& titulo, const juce::String& texto, const juce::String& notifySend)
{
    if (so == SO::mac)
    {
        auto citar = [] (const juce::String& s) { return "\"" + s.replace ("\\", "\\\\").replace ("\"", "\\\"") + "\""; };
        return { "/usr/bin/osascript", "-e", "display notification " + citar (texto) + " with title " + citar (titulo) };
    }
    if (so == SO::lin && notifySend.isNotEmpty())
        return { notifySend, "--app-name=Barullo Xestor", titulo, texto };
    return {};
}

bool notificar (const juce::String& titulo, const juce::String& texto)
{
    juce::String ns;
    if (soActual() == SO::lin)
    {
        ns = atoparPrograma ("notify-send", { "/usr/bin/notify-send" });
        if (ns.isEmpty()) return false;
    }
    const auto orde = ordeNotificacion (soActual(), titulo, texto, ns);
    if (orde.isEmpty()) return false;
    return executar (orde, 5000).ok();
}

// ============================================================== firma (Mac)
juce::String resumoFirma (bool valida, const juce::String& saida)
{
    juce::String autoridade;
    bool adhoc = false, sen = false;
    for (auto& l : juce::StringArray::fromLines (saida))
    {
        if (l.startsWith ("Authority=") && autoridade.isEmpty()) autoridade = l.fromFirstOccurrenceOf ("=", false, false).trim();
        if (l.contains ("Signature=adhoc")) adhoc = true;
        if (l.contains ("not signed at all")) sen = true;
    }
    if (sen) return "sen firma";
    juce::String q = adhoc ? juce::String ("ad hoc") : autoridade.isNotEmpty() ? autoridade.replace ("Developer ID Application: ", "Developer ID: ") : juce::String ("asinado");
    return valida ? q : U ("non válida (") + q + ")";
}

juce::String estadoFirma (const juce::File& f)
{
   #if JUCE_MAC
    const bool valida = executar ({ "/usr/bin/codesign", "--verify", "--deep", "--strict", f.getFullPathName() }, 30000).ok();
    const auto info = executar ({ "/usr/bin/codesign", "-dv", "--verbose=2", f.getFullPathName() }, 30000);
    return resumoFirma (valida, info.saida);
   #else
    juce::ignoreUnused (f);
    return {};
   #endif
}

juce::String descricionSO()
{
    auto s = juce::SystemStats::getOperatingSystemName();
   #if JUCE_MAC
    s << " (" << (juce::SystemStats::getCpuModel().containsIgnoreCase ("Apple") ? "Apple Silicon" : "Intel") << ")";
   #elif JUCE_LINUX
    const juce::File os ("/etc/os-release");
    if (os.existsAsFile())
        for (auto& l : juce::StringArray::fromLines (os.loadFileAsString()))
            if (l.startsWith ("PRETTY_NAME=")) { s << " (" << l.fromFirstOccurrenceOf ("=", false, false).unquoted() << ")"; break; }
   #endif
    s << (juce::SystemStats::isOperatingSystem64Bit() ? ", 64 bits" : ", 32 bits");
    return s;
}
} // namespace xestor::sistema

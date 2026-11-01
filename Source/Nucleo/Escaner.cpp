#include "Escaner.h"

namespace xestor
{
// ============================================================== rexistro
juce::String Rexistro::version (const juce::File& f) const
{
    const juce::ScopedLock l (bloqueo);
    if (ficheiro == juce::File() || ! ficheiro.existsAsFile()) return {};
    const auto o = juce::JSON::parse (ficheiro);
    const auto e = o[juce::Identifier (f.getFullPathName())];
    if (! e.isObject()) return {};
    // se o ficheiro cambiou despois de instalalo (outro instalador), xa non se sabe
    if ((juce::int64) e["data"] != f.getLastModificationTime().toMilliseconds()) return {};
    return e["version"].toString();
}

void Rexistro::anotar (const juce::File& f, const juce::String& version)
{
    const juce::ScopedLock l (bloqueo);
    if (ficheiro == juce::File()) return;
    auto o = ficheiro.existsAsFile() ? juce::JSON::parse (ficheiro) : juce::var();
    if (! o.isObject()) o = juce::var (new juce::DynamicObject());
    auto* e = new juce::DynamicObject();
    e->setProperty ("version", version);
    e->setProperty ("data", f.getLastModificationTime().toMilliseconds());
    o.getDynamicObject()->setProperty (juce::Identifier (f.getFullPathName()), juce::var (e));
    ficheiro.getParentDirectory().createDirectory();
    ficheiro.replaceWithText (juce::JSON::toString (o));
}

// ============================================================== info dos bundles
namespace infoBundle
{
juce::String valorPlist (const juce::String& plist, const juce::String& chave)
{
    // os Info.plist dos bundles de JUCE son XML: <key>chave</key><string>valor</string>
    auto xml = juce::XmlDocument::parse (plist);
    if (xml == nullptr) return {};
    auto* dict = xml->getChildByName ("dict");
    if (dict == nullptr) return {};
    for (auto* e = dict->getFirstChildElement(); e != nullptr; e = e->getNextElement())
        if (e->hasTagName ("key") && e->getAllSubText().trim() == chave)
            if (auto* v = e->getNextElement(); v != nullptr && v->hasTagName ("string"))
                return v->getAllSubText().trim();
    return {};
}

juce::String identificadorPlist (const juce::String& plist) { return valorPlist (plist, "CFBundleIdentifier"); }

juce::String versionPlist (const juce::File& bundle)
{
    const auto p = bundle.getChildFile ("Contents/Info.plist");
    if (! p.existsAsFile()) return {};
    const auto t = p.loadFileAsString();
    auto v = valorPlist (t, "CFBundleShortVersionString");
    return v.isNotEmpty() ? v : valorPlist (t, "CFBundleVersion");
}

static juce::String moduleInfo (const juce::File& bundle)
{
    const auto m = bundle.getChildFile ("Contents/Resources/moduleinfo.json");
    return m.existsAsFile() ? m.loadFileAsString() : juce::String();
}

// o moduleinfo.json leva comas ao final (JSON5): lese co que fai falta, sen analizador
static juce::String campoJson (const juce::String& t, const juce::String& chave)
{
    const int i = t.indexOf ("\"" + chave + "\"");
    if (i < 0) return {};
    const int dous = t.indexOfChar (i, ':');
    const int a = t.indexOfChar (dous, '"');
    const int b = a >= 0 ? t.indexOfChar (a + 1, '"') : -1;
    return (dous >= 0 && a >= 0 && b > a) ? t.substring (a + 1, b) : juce::String();
}

juce::String versionModuleInfo (const juce::File& bundle) { return campoJson (moduleInfo (bundle), "Version"); }
bool vendedorEscoita (const juce::File& bundle) { return campoJson (moduleInfo (bundle), "Vendor") == "Escoita"; }
} // namespace infoBundle

// ============================================================== escáner
std::optional<Instalado> Escaner::ler (const juce::String& plugin, Formato f, const Ubicacion& u, const juce::String& binario) const
{
    const auto fich = u.ruta (plugin, f, binario);
    if (! fich.exists()) return std::nullopt;
    Instalado i;
    i.plugin = plugin; i.formato = f; i.ubicacion = u; i.ficheiro = fich;
    if (fich.isDirectory())
    {
        const auto plist = fich.getChildFile ("Contents/Info.plist");
        if (plist.existsAsFile())
        {
            const auto t = plist.loadFileAsString();
            i.textoVersion = infoBundle::valorPlist (t, "CFBundleShortVersionString");
            if (i.textoVersion.isEmpty()) i.textoVersion = infoBundle::valorPlist (t, "CFBundleVersion");
            i.deEscoita = infoBundle::identificadorPlist (t).startsWith ("link.escoita.");
        }
        if (i.textoVersion.isEmpty()) i.textoVersion = infoBundle::versionModuleInfo (fich);
        if (! i.deEscoita) i.deEscoita = infoBundle::vendedorEscoita (fich);
    }
    if (i.textoVersion.isEmpty() && rexistro != nullptr)
    {
        i.textoVersion = rexistro->version (fich);
        if (i.textoVersion.isNotEmpty()) i.deEscoita = true;
    }
    i.version = Version::ler (i.textoVersion);
    return i;
}

std::vector<Instalado> Escaner::instalados (const Produto& p) const
{
    std::vector<Instalado> r;
    for (auto& plugin : p.plugins)
        for (auto* u : rutas.todas())
            for (auto f : todosOsFormatos())
                if (u->cartafoles.count (f) > 0)
                    if (auto i = ler (plugin, f, *u, p.plugins.size() == 1 ? p.binario : juce::String()))
                        r.push_back (*i);
    return r;
}

std::vector<Instalado> Escaner::orfos (const std::vector<Produto>& conecidos) const
{
    juce::StringArray nomes;
    for (auto& p : conecidos) nomes.addArray (p.plugins);
    std::vector<Instalado> r;
    for (auto* u : rutas.todas())
        for (auto& [f, cartafol] : u->cartafoles)
        {
            if (! cartafol.isDirectory()) continue;
            auto lista = cartafol.findChildFiles (juce::File::findFilesAndDirectories, false, "Barullo *");
            lista.sort();
            for (auto& fich : lista)
            {
                if (formatoDoFicheiro (u->so, fich) != f) continue;
                const auto plugin = fich.getFileNameWithoutExtension();
                if (nomes.contains (plugin) || plugin == "Barullo Xestor") continue;
                auto i = ler (plugin, f, *u);
                // só os que son de Escoita de verdade (polo identificador ou o fabricante)
                if (i && i->deEscoita) r.push_back (*i);
            }
        }
    return r;
}

std::optional<Produto> Escaner::produtoSolto (const juce::String& nome) const
{
    for (auto& i : orfos ({}))
    {
        Produto p;
        p.nome = i.plugin;
        p.curto = i.plugin.removeCharacters (" ");
        p.plugins.add (i.plugin);
        if (p.casa (nome)) return p;
    }
    return std::nullopt;
}
} // namespace xestor

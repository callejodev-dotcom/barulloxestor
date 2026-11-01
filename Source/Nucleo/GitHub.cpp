#include "GitHub.h"
#include "Proceso.h"

namespace xestor
{
juce::File GitHub::cacheEstandar()
{
   #if JUCE_MAC
    return juce::File ("~/Library/Caches/link.escoita.barulloxestor");
   #elif JUCE_WINDOWS
    return juce::File (juce::SystemStats::getEnvironmentVariable ("LOCALAPPDATA", "~")).getChildFile ("Barullo Xestor").getChildFile ("cache");
   #else
    auto xdg = juce::SystemStats::getEnvironmentVariable ("XDG_CACHE_HOME", {});
    return (xdg.isNotEmpty() ? juce::File (xdg) : juce::File ("~/.cache")).getChildFile ("barulloxestor");
   #endif
}

juce::String GitHub::tokenDisponible()
{
    for (auto k : { "GITHUB_TOKEN", "GH_TOKEN" })
    {
        auto t = juce::SystemStats::getEnvironmentVariable (k, {});
        if (t.isNotEmpty()) return t;
    }
    const auto gh = atoparPrograma ("gh", { "/opt/homebrew/bin/gh", "/usr/local/bin/gh", "/usr/bin/gh",
                                            "C:\\Program Files\\GitHub CLI\\gh.exe" });
    if (gh.isNotEmpty())
    {
        auto r = executar ({ gh, "auth", "token" }, 5000);
        auto t = r.saida.trim();
        if (r.ok() && t.isNotEmpty() && ! t.containsChar (' ')) return t;
    }
    return {};
}

// ============================================================== caché
juce::File GitHub::ficheiro (const juce::String& url, const char* ext) const
{
    // un nome estable e curto para cada URL (FNV-1a)
    juce::uint64 h = 1469598103934665603ull;
    for (auto* p = url.toRawUTF8(); *p != 0; ++p) { h ^= (juce::uint8) *p; h *= 1099511628211ull; }
    return cache.getChildFile (juce::String::toHexString ((juce::int64) h).paddedLeft ('0', 16) + ext);
}

Resposta GitHub::pedir (const juce::String& url, bool forzar)
{
    const auto meta = ficheiro (url, ".json"), corpo = ficheiro (url, ".bin");
    juce::var vello;
    {
        const juce::ScopedLock l (bloqueoCache);
        if (meta.existsAsFile() && corpo.existsAsFile()) vello = juce::JSON::parse (meta);
    }
    auto deCache = [&]
    {
        Resposta r;
        r.estado = (int) vello["estado"];
        corpo.loadFileAsData (r.corpo);
        return r;
    };
    const bool hai = vello.isObject();
    const auto idade = juce::Time::getCurrentTime() - juce::Time ((juce::int64) vello["data"]);
    if (hai && (soCache || (! forzar && idade < vixencia))) return deCache();
    if (soCache) throw ErroGitHub ("sen rede e sen nada gardado de " + url.toStdString());

    juce::StringPairArray cab;
    const bool api = url.startsWith ("https://api.github.com/");
    if (api)
    {
        cab.set ("Accept", "application/vnd.github+json");
        cab.set ("X-GitHub-Api-Version", "2022-11-28");
        if (token.isNotEmpty()) cab.set ("Authorization", "Bearer " + token);
    }
    if (hai && vello["etag"].toString().isNotEmpty()) cab.set ("If-None-Match", vello["etag"].toString());

    auto r = transporte.pedir (url, cab);
    if (api)
    {
        const auto q = r.cabeceiras["x-ratelimit-remaining"];
        if (q.isNotEmpty()) restantesAPI = q.getIntValue();
    }
    auto gardar = [&] (const Resposta& x, const juce::String& etag)
    {
        const juce::ScopedLock l (bloqueoCache);
        cache.createDirectory();
        auto* o = new juce::DynamicObject();
        o->setProperty ("url", url);
        o->setProperty ("etag", etag);
        o->setProperty ("data", juce::Time::getCurrentTime().toMilliseconds());
        o->setProperty ("estado", x.estado);
        corpo.replaceWithData (x.corpo.getData(), x.corpo.getSize());
        meta.replaceWithText (juce::JSON::toString (juce::var (o)));
    };
    switch (r.estado)
    {
        case 304:
            if (hai)
            {
                auto c = deCache();
                gardar (c, vello["etag"].toString());   // renova a data
                return c;
            }
            throw ErroGitHub ("GitHub respondeu 304 sen nada gardado");
        case 200:
        case 404:
            gardar (r, r.cabeceiras["ETag"]);
            return r;
        default:
            if (hai) { cacheVella = true; return deCache(); }
            if (r.estado == 403 || r.estado == 429)
                throw ErroGitHub (juce::String (juce::CharPointer_UTF8 ("GitHub di que xa se lle preguntou de máis; volve probar dentro dun pouco.")).toStdString());
            if (r.estado == 0) throw ErroGitHub (juce::String (juce::CharPointer_UTF8 ("Sen conexión con GitHub")).toStdString());
            throw ErroGitHub (("GitHub respondeu " + juce::String (r.estado) + " a " + url).toStdString());
    }
}

juce::var GitHub::json (const juce::String& url, bool forzar)
{
    auto r = pedir (url, forzar);
    if (r.estado == 404) return {};
    juce::var v;
    if (juce::JSON::parse (r.texto(), v).failed()) throw ErroGitHub (("Non se entende a resposta de " + url).toStdString());
    return v;
}

// ============================================================== o que se lle pregunta
static RepoInfo repoDe (const juce::var& o, const juce::String& porDefecto = {})
{
    RepoInfo r;
    r.nomeCompleto = o["full_name"].isString() ? o["full_name"].toString() : porDefecto;
    r.descricion = o["description"].isString() ? o["description"].toString() : juce::String();
    r.rama = o["default_branch"].isString() ? o["default_branch"].toString() : juce::String ("main");
    r.privado = (bool) o["private"];
    r.arquivado = (bool) o["archived"];
    r.bifurcacion = (bool) o["fork"];
    return r;
}

std::vector<RepoInfo> GitHub::repos (bool forzar)
{
    std::vector<RepoInfo> r;
    for (int paxina = 1; paxina < 20; ++paxina)
    {
        const auto u = "https://api.github.com/users/" + propietario + "/repos?per_page=100&type=owner&page=" + juce::String (paxina);
        const auto lista = json (u, forzar);
        if (! lista.isArray()) throw ErroGitHub (("Non se entende a resposta de " + u).toStdString());
        for (auto& o : *lista.getArray())
            if (o["full_name"].isString()) r.push_back (repoDe (o));
        if (lista.size() < 100) break;
    }
    return r;
}

std::optional<RepoInfo> GitHub::repo (const juce::String& nomeCompleto, bool forzar)
{
    const auto o = json ("https://api.github.com/repos/" + nomeCompleto, forzar);
    if (! o.isObject()) return std::nullopt;
    return repoDe (o, nomeCompleto);
}

std::optional<juce::String> GitHub::produtoEnv (const RepoInfo& repo, bool forzar)
{
    auto r = pedir ("https://raw.githubusercontent.com/" + repo.nomeCompleto + "/" + repo.rama + "/packaging/produto.env", forzar);
    if (r.estado != 200) return std::nullopt;
    return r.texto();
}

std::optional<Lanzamento> GitHub::ultimoLanzamento (const juce::String& nomeCompleto, bool forzar)
{
    const auto o = json ("https://api.github.com/repos/" + nomeCompleto + "/releases/latest", forzar);
    if (! o.isObject()) return std::nullopt;
    return lanzamento (o);
}

std::optional<Lanzamento> GitHub::lanzamento (const juce::var& o)
{
    if (! o["tag_name"].isString()) return std::nullopt;
    Lanzamento l;
    l.tag = o["tag_name"].toString();
    l.nome = o["name"].isString() ? o["name"].toString() : l.tag;
    l.paxina = o["html_url"].toString();
    if (o["published_at"].isString()) l.publicado = juce::Time::fromISO8601 (o["published_at"].toString());
    if (auto* a = o["assets"].getArray())
        for (auto& x : *a)
        {
            if (! x["name"].isString() || ! x["browser_download_url"].isString()) continue;
            Activo act;
            act.nome = x["name"].toString();
            act.url = x["browser_download_url"].toString();
            act.tamano = (juce::int64) x["size"];
            const auto d = x["digest"].toString();
            if (d.startsWith ("sha256:")) act.sha256 = d.substring (7);
            l.activos.push_back (act);
        }
    return l;
}

// ============================================================== capturas
juce::StringArray imaxesMarkdown (const juce::String& md)
{
    juce::StringArray r;
    auto valida = [] (const juce::String& ruta)
    {
        const auto s = ruta.toLowerCase().upToFirstOccurrenceOf ("?", false, false);
        return s.endsWith (".png") || s.endsWith (".jpg") || s.endsWith (".jpeg") || s.endsWith (".gif");
    };
    for (int i = 0; i < md.length(); ++i)
    {
        juce::String ruta;
        if (md.substring (i, i + 2) == "![")
        {
            const int pecha = md.indexOf (i, "](");
            const int fin = pecha >= 0 ? md.indexOfChar (pecha + 2, ')') : -1;
            if (pecha < 0 || fin < 0) continue;
            ruta = md.substring (pecha + 2, fin).trim().upToFirstOccurrenceOf (" ", false, false).unquoted();
            i = fin;
        }
        else if (md.substring (i, i + 4).equalsIgnoreCase ("<img"))
        {
            const int src = md.indexOfIgnoreCase (i, "src=");
            const int pecha = md.indexOfChar (i, '>');
            if (src < 0 || (pecha >= 0 && src > pecha)) continue;
            const auto q = md[src + 4];
            const int fin = md.indexOfChar (src + 5, q);
            if (fin < 0) continue;
            ruta = md.substring (src + 5, fin);
            i = fin;
        }
        if (ruta.isNotEmpty() && valida (ruta)) r.addIfNotAlreadyThere (ruta);
    }
    return r;
}

// «docs/MANUAL.md» + «img/a.png» → «docs/img/a.png»; «../x.png» tamén
static juce::String resolver (const juce::String& dirMd, const juce::String& ruta)
{
    if (ruta.startsWith ("http://") || ruta.startsWith ("https://")) return ruta;
    auto partes = juce::StringArray::fromTokens (ruta.startsWith ("/") ? ruta.substring (1) : (dirMd.isEmpty() ? ruta : dirMd + "/" + ruta), "/", "");
    juce::StringArray r;
    for (auto& p : partes)
    {
        if (p == "." || p.isEmpty()) continue;
        if (p == "..") { if (! r.isEmpty()) r.remove (r.size() - 1); continue; }
        r.add (p);
    }
    return r.joinIntoString ("/");
}

juce::StringArray GitHub::capturas (const RepoInfo& repo, const juce::StringArray& declaradas, bool forzar)
{
    const auto raw = "https://raw.githubusercontent.com/" + repo.nomeCompleto + "/" + repo.rama + "/";
    juce::StringArray r;
    auto engadir = [&] (const juce::String& ruta) { r.addIfNotAlreadyThere (ruta.startsWith ("http") ? ruta : raw + ruta); };
    for (auto& d : declaradas) engadir (resolver ({}, d));
    try
    {
        if (pedir (raw + "docs/img/captura.png", forzar).estado == 200) engadir ("docs/img/captura.png");
        for (auto md : { "LEEME.md", "README.md", "docs/MANUAL.md" })
        {
            auto resp = pedir (raw + md, forzar);
            if (resp.estado != 200) continue;
            const juce::String dir = juce::String (md).containsChar ('/') ? juce::String (md).upToLastOccurrenceOf ("/", false, false) : juce::String();
            for (auto& i : imaxesMarkdown (resp.texto()))
                if (! i.containsIgnoreCase ("shields.io") && ! i.containsIgnoreCase ("badge")) engadir (resolver (dir, i));
        }
    }
    catch (const ErroGitHub&) {}   // sen capturas: a tarxeta leva a inicial
    return r;
}

juce::MemoryBlock GitHub::imaxe (const juce::String& url)
{
    if (! url.startsWith ("http"))
    {
        juce::MemoryBlock m;
        juce::File (url).loadFileAsData (m);
        return m;
    }
    try
    {
        auto r = pedir (url, false);
        if (r.estado == 200) return r.corpo;
    }
    catch (const ErroGitHub&) {}
    return {};
}
} // namespace xestor

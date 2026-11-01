#include "Descubrimento.h"
#include "Escaner.h"

namespace xestor
{
juce::File cartafolDatos()
{
    auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    base = base.getChildFile ("Application Support");   // en Mac, JUCE dá ~/Library
   #endif
    return base.getChildFile ("Barullo Xestor");
}

juce::File Rexistro::estandar() { return cartafolDatos().getChildFile ("instalados.json"); }

// ============================================================== axustes
Axustes Axustes::cargar (const juce::File& f)
{
    Axustes a;
    if (! f.existsAsFile()) return a;
    const auto o = juce::JSON::parse (f);
    if (! o.isObject()) return a;
    auto lista = [&] (const char* k, juce::StringArray& dest)
    {
        if (auto* v = o[k].getArray()) { dest.clear(); for (auto& x : *v) dest.add (x.toString()); }
    };
    if (o["propietario"].isString()) a.propietario = o["propietario"].toString();
    lista ("reposExtra", a.reposExtra);
    lista ("reposOcultos", a.reposOcultos);
    lista ("cartafolesLocais", a.cartafolesLocais);
    if (auto* v = o["formatos"].getArray())
    {
        std::vector<Formato> fs;
        for (auto& x : *v)
            if (auto ff = formatoDeClave (x.toString())) fs.push_back (*ff);
        if (! fs.empty()) a.formatos = fs;
    }
    if (o["usarGh"].isBool()) a.usarGh = (bool) o["usarGh"];
    return a;
}

bool Axustes::gardar (const juce::File& f) const
{
    auto* o = new juce::DynamicObject();
    auto lista = [] (const juce::StringArray& s) { juce::Array<juce::var> a; for (auto& x : s) a.add (x); return juce::var (a); };
    o->setProperty ("propietario", propietario);
    o->setProperty ("reposExtra", lista (reposExtra));
    o->setProperty ("reposOcultos", lista (reposOcultos));
    o->setProperty ("cartafolesLocais", lista (cartafolesLocais));
    juce::StringArray fs;
    for (auto x : formatos) fs.add (clave (x));
    o->setProperty ("formatos", lista (fs));
    o->setProperty ("usarGh", usarGh);
    f.getParentDirectory().createDirectory();
    return f.replaceWithText (juce::JSON::toString (juce::var (o)));
}

juce::Array<juce::File> Axustes::urlsLocais() const
{
    juce::Array<juce::File> r;
    for (auto& c : cartafolesLocais)
        if (juce::File::isAbsolutePath (c) || c.startsWith ("~")) r.add (juce::File (c));
    return r;
}

const Produto* Catalogo::buscar (const juce::String& q) const
{
    for (auto& p : produtos)
        if (p.casa (q)) return &p;
    return nullptr;
}

// ============================================================== descubrir
Catalogo Descubridor::descubrir (bool forzar)
{
    Catalogo c;
    if (github != nullptr)
    {
        try
        {
            std::vector<RepoInfo> repos;
            for (auto& r : github->repos (forzar))
                if (! r.privado && ! r.arquivado && ! r.bifurcacion) repos.push_back (r);
            for (auto& extra : axustes.reposExtra)
            {
                bool xa = false;
                for (auto& r : repos) xa = xa || r.nomeCompleto.equalsIgnoreCase (extra);
                if (xa) continue;
                try
                {
                    if (auto r = github->repo (extra, forzar)) repos.push_back (*r);
                    else c.avisos.add ("Non se atopa o repo " + extra);
                }
                catch (const ErroGitHub& e) { c.avisos.add (extra + ": " + e.texto()); }
            }
            repos.erase (std::remove_if (repos.begin(), repos.end(), [&] (const RepoInfo& r)
            {
                for (auto& o : axustes.reposOcultos)
                    if (r.nomeCompleto.equalsIgnoreCase (o) || r.nomeCompleto.fromLastOccurrenceOf ("/", false, false).equalsIgnoreCase (o)) return true;
                return false;
            }), repos.end());

            // cada repo, no seu fío (o produto.env, a última versión e as capturas)
            std::vector<std::optional<Produto>> atopados (repos.size());
            std::vector<juce::String> avisos (repos.size());
            juce::ThreadPool pool (juce::ThreadPoolOptions{}.withNumberOfThreads (8));
            for (size_t i = 0; i < repos.size(); ++i)
                pool.addJob ([&, i]
                {
                    const auto& r = repos[i];
                    try
                    {
                        auto env = github->produtoEnv (r, forzar);
                        if (! env) return;
                        auto p = Produto::deProdutoEnv (*env, r.nomeCompleto, r.descricion);
                        if (! p) return;
                        p->rama = r.rama;
                        p->ultima = github->ultimoLanzamento (r.nomeCompleto, forzar);
                        p->lanzamentoConsultado = true;
                        p->capturas = github->capturas (r, p->capturasDeclaradas, forzar);
                        atopados[i] = std::move (p);
                    }
                    catch (const ErroGitHub& e) { avisos[i] = r.nomeCompleto + ": " + e.texto(); }
                });
            while (pool.getNumJobs() > 0) juce::Thread::sleep (5);
            for (size_t i = 0; i < repos.size(); ++i)
            {
                if (atopados[i]) c.produtos.push_back (std::move (*atopados[i]));
                if (avisos[i].isNotEmpty()) c.avisos.add (avisos[i]);
            }
            if (github->usouCacheVella()) c.avisos.add (juce::String (juce::CharPointer_UTF8 ("Sen conexión con GitHub: amósase o último que se soubo.")));
        }
        catch (const ErroGitHub& e)
        {
            c.avisos.add ("Non se puido preguntar a GitHub: " + e.texto());
            c.senRede = true;
        }
    }

    // os repositorios locais
    for (auto& local : locais (axustes.urlsLocais()))
    {
        bool xa = false;
        for (auto& p : c.produtos)
            if (p.curto.equalsIgnoreCase (local.curto) || p.nome == local.nome)
            {
                xa = true;
                if (p.rutaLocal == juce::File()) { p.rutaLocal = local.rutaLocal; p.versionLocal = local.versionLocal; }
                if (p.capturas.isEmpty()) p.capturas = local.capturas;
            }
        if (! xa) c.produtos.push_back (local);
    }
    std::sort (c.produtos.begin(), c.produtos.end(), [] (const Produto& a, const Produto& b)
    {
        if (a.tipo != b.tipo) return orde (a.tipo) < orde (b.tipo);
        return a.nome.compareIgnoreCase (b.nome) < 0;
    });
    c.avisos.sort (true);
    return c;
}

std::vector<Produto> Descubridor::locais (const juce::Array<juce::File>& cartafoles)
{
    std::vector<Produto> r;
    for (auto& c : cartafoles)
    {
        if (! c.isDirectory()) continue;
        auto subs = c.findChildFiles (juce::File::findDirectories, false);
        subs.sort();
        for (auto& s : subs)
        {
            if (s.getFileName().startsWith (".") || s.getFileName() == "escoita-nucleo") continue;   // o núcleo leva un de exemplo
            const auto env = s.getChildFile ("packaging/produto.env");
            if (! env.existsAsFile()) continue;
            auto p = Produto::deProdutoEnv (env.loadFileAsString());
            if (! p) continue;
            p->rutaLocal = s;
            p->versionLocal = versionCMake (s.getChildFile ("CMakeLists.txt"));
            p->capturas = capturasLocais (s, p->capturasDeclaradas);
            bool xa = false;
            for (auto& x : r) xa = xa || x.curto == p->curto;
            if (! xa) r.push_back (*p);
        }
    }
    return r;
}

std::optional<Version> Descubridor::versionCMake (const juce::File& f)
{
    if (! f.existsAsFile()) return std::nullopt;
    const auto t = f.loadFileAsString();
    for (int i = t.indexOfIgnoreCase ("project"); i >= 0; i = t.indexOfIgnoreCase (i + 7, "project"))
    {
        const int a = t.indexOfChar (i, '('), b = a >= 0 ? t.indexOfChar (a, ')') : -1;
        if (a < 0 || b < 0) break;
        const auto dentro = t.substring (a + 1, b);
        const int v = dentro.indexOf ("VERSION");
        if (v >= 0) return Version::ler (dentro.substring (v + 7).trim().upToFirstOccurrenceOf (" ", false, false));
    }
    return std::nullopt;
}

juce::StringArray Descubridor::capturasLocais (const juce::File& repo, const juce::StringArray& declaradas)
{
    juce::StringArray r;
    auto engadir = [&] (const juce::File& f) { if (f.existsAsFile()) r.addIfNotAlreadyThere (f.getFullPathName()); };
    for (auto& d : declaradas) engadir (repo.getChildFile (d));
    engadir (repo.getChildFile ("docs/img/captura.png"));
    for (auto md : { "LEEME.md", "README.md", "docs/MANUAL.md" })
    {
        const auto f = repo.getChildFile (md);
        if (! f.existsAsFile()) continue;
        for (auto& i : imaxesMarkdown (f.loadFileAsString()))
            if (! i.startsWith ("http")) engadir (f.getParentDirectory().getChildFile (i));
    }
    return r;
}
} // namespace xestor

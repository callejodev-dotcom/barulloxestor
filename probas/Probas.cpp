// As probas de Barullo Xestor: o núcleo e a liña de ordes. Traballan en cartafoles temporais e
// cun GitHub de mentira (sen rede): nunca tocan os plugins de verdade nin piden permisos de verdade.
#include "Nucleo/Nucleo.h"
#include "App/CLI.h"
#include <cstdio>
#include <set>

#if ! JUCE_WINDOWS
 #include <stdlib.h>
 #include <unistd.h>
#endif
#if JUCE_MAC
 #include <sys/xattr.h>
#endif

using namespace xestor;

namespace
{
int fallos = 0, total = 0;
void proba (const char* que, bool ok)
{
    ++total;
    std::printf ("%-64s %s\n", juce::String::fromUTF8 (que).toRawUTF8(), ok ? "OK" : "FALLO");
    if (! ok) ++fallos;
}

// un cartafol temporal que se borra ao final
struct Temporal
{
    juce::File dir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                         .getChildFile ("probas-barulloxestor-" + juce::Uuid().toString());
    Temporal() { dir.createDirectory(); }
    ~Temporal() { dir.deleteRecursively(); }
    juce::File sub (const juce::String& s) const { auto f = dir.getChildFile (s); f.createDirectory(); return f; }
};

const SO so = soActual();

bool executable (const juce::File& f)
{
   #if JUCE_WINDOWS
    return f.existsAsFile();
   #else
    return access (f.getFullPathName().toRawUTF8(), X_OK) == 0;
   #endif
}

// un bundle de proba coa versión no Info.plist e no moduleinfo.json (e, en Mac, un executable)
juce::File facerBundle (const juce::File& dir, const juce::String& nome, Formato f, const juce::String& version,
                        const juce::String& id = {})
{
    const auto fich = dir.getChildFile (nomeFicheiro (so, nome, f, nome.toLowerCase().replace (" ", "-")));
    const bool eCartafol = so == SO::mac || f == Formato::vst3 || f == Formato::lv2;
    if (! eCartafol)
    {
        fich.getParentDirectory().createDirectory();
        fich.replaceWithText ("binario de " + nome + " " + version);
        return fich;
    }
    const auto c = fich.getChildFile ("Contents");
    c.getChildFile ("Resources").createDirectory();
    const auto ident = id.isNotEmpty() ? id : "link.escoita." + nome.toLowerCase().removeCharacters (" ");
    c.getChildFile ("Info.plist").replaceWithText (
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<plist version=\"1.0\"><dict>"
        "<key>CFBundleIdentifier</key><string>" + ident + "</string>"
        "<key>CFBundleShortVersionString</key><string>" + version + "</string>"
        "<key>CFBundleName</key><string>" + nome + "</string>"
        "<key>CFBundleExecutable</key><string>" + nome + "</string>"
        "<key>CFBundlePackageType</key><string>BNDL</string></dict></plist>\n");
    c.getChildFile ("Resources/moduleinfo.json").replaceWithText ("{\n  \"Name\": \"" + nome + "\",\n  \"Version\": \"" + version
        + "\",\n  \"Factory Info\": {\n    \"Vendor\": \"" + (ident.startsWith ("link.escoita.") ? "Escoita" : "Outros") + "\",\n  },\n}\n");
   #if JUCE_MAC
    c.getChildFile ("MacOS").createDirectory();
    juce::File ("/usr/bin/true").copyFileTo (c.getChildFile ("MacOS/" + nome));
    c.getChildFile ("MacOS/" + nome).setExecutePermission (true);
   #endif
    return fich;
}

void asinar (const juce::File& b)
{
   #if JUCE_MAC
    executar ({ "/usr/bin/codesign", "--force", "--deep", "-s", "-", b.getFullPathName() });
   #else
    juce::ignoreUnused (b);
   #endif
}

// un arquivo coma os publicados: «<Curto>-<ver>-mac/» (ou -windows-x64, -linux-x86_64) cos bundles
juce::File facerArquivo (const Temporal& t, const juce::String& curto, const juce::String& version,
                         const std::vector<std::pair<juce::String, Formato>>& bundles, bool asinados = true, bool cuarentena = false)
{
    const juce::String sufixo = so == SO::mac ? "mac" : so == SO::win ? "windows-x64" : "linux-x86_64";
    const auto pai = t.sub ("fonte-" + juce::Uuid().toString());
    const auto d = pai.getChildFile (curto + "-" + version + "-" + sufixo);
    d.createDirectory();
    for (auto& [n, f] : bundles)
    {
        auto b = facerBundle (d, n, f, version);
        if (asinados) asinar (b);
       #if JUCE_MAC
        if (cuarentena) setxattr (b.getFullPathName().toRawUTF8(), "com.apple.quarantine", "0083;00000000;Safari;", 21, 0, 0);
       #else
        juce::ignoreUnused (cuarentena);
       #endif
    }
    d.getChildFile ("LEEME.txt").replaceWithText ("le-me");
    if (so == SO::lin)
    {
        const auto tgz = t.dir.getChildFile (curto + "-" + version + "-" + sufixo + ".tar.gz");
        executar ({ "tar", "-czf", tgz.getFullPathName(), "-C", pai.getFullPathName(), d.getFileName() });
        return tgz;
    }
    const auto zip = t.dir.getChildFile (curto + "-" + version + "-" + sufixo + ".zip");
    zip.deleteFile();
   #if JUCE_MAC
    executar ({ "/usr/bin/ditto", "-c", "-k", "--sequesterRsrc", "--keepParent", d.getFullPathName(), zip.getFullPathName() });
   #else
    juce::ZipFile::Builder zb;
    for (const auto& e : juce::RangedDirectoryIterator (d, true, "*", juce::File::findFiles))
        zb.addFile (e.getFile(), 6, e.getFile().getRelativePathFrom (pai).replaceCharacter ('\\', '/'));
    juce::FileOutputStream out (zip);
    zb.writeToStream (out, nullptr);
   #endif
    return zip;
}

juce::String nomeArquivo (const juce::String& curto, const juce::String& v)
{
    return curto + "-" + v + (so == SO::mac ? "-mac.zip" : so == SO::win ? "-windows-x64.zip" : "-linux-x86_64.tar.gz");
}

Lanzamento lanzamentoDe (const juce::String& v, const juce::File& arquivo = {}, const juce::String& sha = {}, juce::int64 tamano = 0)
{
    Lanzamento l;
    l.tag = "v" + v;
    l.nome = l.tag;
    if (arquivo != juce::File())
        l.activos.push_back ({ arquivo.getFileName(), juce::URL (arquivo).toString (false), tamano, sha });
    return l;
}

Produto produto (const juce::String& nome, juce::StringArray plugins = {}, TipoProduto tipo = TipoProduto::efecto)
{
    Produto p;
    p.nome = nome;
    p.curto = nome.removeCharacters (" ");
    p.binario = nome.toLowerCase().replace (" ", "-");
    p.plugins = plugins.isEmpty() ? juce::StringArray { nome } : plugins;
    p.tipo = tipo;
    p.repo = "callejodev-dotcom/" + nome.toLowerCase().removeCharacters (" ");
    p.lanzamentoConsultado = true;
    return p;
}

// ============================================================== un GitHub de mentira
struct GitHubFalso : Transporte
{
    struct R { int estado; juce::String corpo; juce::String etag; };
    std::map<juce::String, R> respostas;
    std::map<juce::String, juce::File> ficheiros;
    int pedidos = 0, conEtag = 0;
    bool senRede = false;
    juce::StringArray urls;

    void json (const juce::String& url, const juce::String& corpo, const juce::String& etag = "\"e1\"") { respostas[url] = { 200, corpo, etag }; }

    Resposta pedir (const juce::String& url, const juce::StringPairArray& cab) override
    {
        ++pedidos;
        urls.add (url);
        Resposta r;
        if (senRede) { r.erro = "sen rede"; return r; }
        auto i = respostas.find (url);
        if (i == respostas.end()) { r.estado = 404; r.corpo.append ("{}", 2); return r; }
        if (cab["If-None-Match"].isNotEmpty()) ++conEtag;
        if (i->second.etag.isNotEmpty() && cab["If-None-Match"] == i->second.etag) { r.estado = 304; return r; }
        r.estado = i->second.estado;
        r.corpo.append (i->second.corpo.toRawUTF8(), i->second.corpo.getNumBytesAsUTF8());
        r.cabeceiras.set ("ETag", i->second.etag);
        if (url.startsWith ("https://api.github.com")) r.cabeceiras.set ("x-ratelimit-remaining", "59");
        return r;
    }
    juce::Result descargar (const juce::String& url, const juce::File& destino, std::function<bool (double)> progreso) override
    {
        if (senRede) return juce::Result::fail ("sen rede");
        auto i = ficheiros.find (url);
        auto r = copiarLocal (i != ficheiros.end() ? i->second.getFullPathName() : url, destino);
        if (r.wasOk() && progreso) progreso (1.0);
        return r;
    }
};

// un GitHub que nunca contesta (coma unha conexión colgada): só volve se se cancela
struct GitHubMudo : Transporte
{
    std::atomic<int> agardando { 0 };
    Resposta pedir (const juce::String&, const juce::StringPairArray&) override
    {
        juce::WaitableEvent nunca;
        nunca.wait (-1);
        return {};
    }
    Resposta pedirCancelable (const juce::String&, const juce::StringPairArray&, const Cancelacion& c) override
    {
        ++agardando;
        c.agardar (-1);
        Resposta r;
        r.erro = "cancelado";
        return r;
    }
    juce::Result descargar (const juce::String&, const juce::File&, std::function<bool (double)>) override { return juce::Result::fail ("non"); }
};

juce::String envSinxelo (const juce::String& nome, const juce::String& au, const juce::String& extra = {})
{
    return "NOME=\"" + nome + "\"\nCURTO=\"" + nome.removeCharacters (" ") + "\"\nDESCRICION=\"Un plugin de proba\" # comentario\nAU=\"" + au + "\"\n" + extra;
}

// o GitHub de mentira cun instrumento (FM), un efecto (Ley), unha serie (Enxebres) e un repo que non é plugin
void encherGitHub (GitHubFalso& g, const Temporal& t)
{
    const juce::String api = "https://api.github.com/", raw = "https://raw.githubusercontent.com/callejodev-dotcom/";
    g.json (api + "users/callejodev-dotcom/repos?per_page=100&type=owner&page=1", R"([
      {"full_name":"callejodev-dotcom/barullofm","description":"FM","default_branch":"main","private":false,"archived":false,"fork":false},
      {"full_name":"callejodev-dotcom/barulloley","description":"Delay","default_branch":"main"},
      {"full_name":"callejodev-dotcom/enxebres","default_branch":"main"},
      {"full_name":"callejodev-dotcom/portnhse","default_branch":"main"},
      {"full_name":"callejodev-dotcom/vello","default_branch":"main","archived":true}])");
    g.json (raw + "barullofm/main/packaging/produto.env", envSinxelo ("Barullo FM", "aumu Bfm1 Esco"));
    g.json (raw + "barulloley/main/packaging/produto.env", envSinxelo ("Barullo Ley", "aufx Bley Esco"));
    g.json (raw + "enxebres/main/packaging/produto.env", "NOME=\"Barullo Enxebres\"\nCURTO=\"BarulloEnxebres\"\nDESCRICION=\"Nove\"\nAU=\"\"\n"
                                                          "PLUGINS=\"Barullo Filtro|BarulloFiltro|aufx Bfil Esco\nBarullo Chorus|BarulloChorus|aufx Bcho Esco\"\n");
    // FM: unha captura estándar; Ley: unha imaxe no LEEME; Enxebres: ningunha
    g.json (raw + "barullofm/main/docs/img/captura.png", "PNG");
    g.json (raw + "barulloley/main/LEEME.md", "# Ley\n![A interface](docs/img/ley.png)\n[![badge](https://img.shields.io/x.png)](x)\n");
    const auto zipFM = facerArquivo (t, "BarulloFM", "0.5.1", { { "Barullo FM", Formato::vst3 } });
    g.ficheiros["https://example.invalid/fm.zip"] = zipFM;
    g.json (api + "repos/callejodev-dotcom/barullofm/releases/latest",
            R"({"tag_name":"v0.5.1","name":"Barullo FM 0.5.1","html_url":"https://github.com/x","published_at":"2026-10-30T10:00:00Z","assets":[
               {"name":")" + nomeArquivo ("BarulloFM", "0.5.1") + R"(","browser_download_url":"https://example.invalid/fm.zip","size":)"
               + juce::String (zipFM.getSize()) + R"(,"digest":"sha256:)" + sha256 (zipFM) + R"("}]})");
    g.json (api + "repos/callejodev-dotcom/barulloley/releases/latest", R"({"tag_name":"v0.1.1","assets":[]})");
    // Enxebres sen versión publicada: 404 (non se engade)
}
} // namespace

int main (int argc, char** argv)
{
    juce::ignoreUnused (argc, argv);

    // ------------------------------------------------------------ versións e produto.env
    {
        auto v = [] (const char* s) { return *Version::ler (s); };
        proba ("versións: 0.10 despois de 0.9, «v», beta e números que faltan",
               v ("0.10.0") > v ("0.9.9") && v ("v0.4.0") == v ("0.4") && v ("1.2.3-beta") == v ("1.2.3")
               && v ("0.5.1") > v ("0.5.0") && ! Version::ler ("lixo") && ! Version::ler ("") && v ("1.0") > v ("0.99.99"));
    }
    {
        auto p = Produto::deProdutoEnv (envSinxelo ("Barullo Tune", "aumf Btun Esco", "BINARIO=\"barullo-tune\"\nCAPTURAS=\"docs/a.png docs/b.png\"\n"));
        proba ("produto.env: un plugin (nome, curto, tipo, binario e capturas)",
               p && p->nome == "Barullo Tune" && p->curto == "BarulloTune" && p->tipo == TipoProduto::efecto && p->temAU
               && p->descricion == "Un plugin de proba" && p->plugins == juce::StringArray { "Barullo Tune" } && p->binario == "barullo-tune"
               && p->capturasDeclaradas == juce::StringArray { "docs/a.png", "docs/b.png" });
        auto s = Produto::deProdutoEnv ("# serie\nNOME=\"Barullo Enxebres\"\nPLUGINS=\"Barullo Filtro|BarulloFiltro|aufx Bfil Esco\n  Barullo Volume|BarulloVolume|\"\n");
        proba ("produto.env: unha serie (PLUGINS en varias liñas)",
               s && s->eSerie() && s->plugins == juce::StringArray { "Barullo Filtro", "Barullo Volume" } && s->temAU && s->curto == "BarulloEnxebres");
        proba ("produto.env: sen NOME non é un produto", ! Produto::deProdutoEnv ("CURTO=x\n"));
        proba ("buscar polo nome: «tune», «Barullo Tune», «barullotune», «TUNE»",
               p->casa ("tune") && p->casa ("Barullo Tune") && p->casa ("barullotune") && p->casa ("TUNE") && ! p->casa ("ley")
               && normalizar (U ("Ambiénte Ñ")) == "ambienten");

        // a xanela: as pestanas, o buscador e os recontos
        auto ley = *p;
        ley.nome = "Barullo Ley"; ley.curto = "BarulloLey"; ley.plugins = { "Barullo Ley" };
        ley.descricion = U ("Un delay: dixital, cinta, analóxico");
        auto xuno = ley;
        xuno.nome = "Barullo Xuno"; xuno.curto = "BarulloXuno"; xuno.plugins = { "Barullo Xuno" };
        xuno.tipo = TipoProduto::instrumento; xuno.descricion = U ("Sintetizador analóxico de seis voces");
        const std::vector<Produto> todos { *p, ley, xuno, *s };
        const auto r = contar (todos);
        const auto rb = contar (todos, "ANALOXICO");
        proba ("filtros: pestanas, busca sen tiles en varias palabras e recontos",
               casaPestana (xuno, Pestana::instrumentos) && ! casaPestana (xuno, Pestana::efectos) && casaPestana (*s, Pestana::paquetes)
               && casaPestana (ley, Pestana::todo)
               && casaBusca (ley, "") && casaBusca (ley, "  ") && casaBusca (ley, U ("analóxico")) && casaBusca (ley, "cinta delay")
               && ! casaBusca (ley, "cinta reverb") && casaBusca (*s, "volume") && casaBusca (xuno, "xu") && ! casaBusca (xuno, "ley")
               && r.todo == 4 && r.instrumentos == 1 && r.efectos == 2 && r.paquetes == 1 && r.de (Pestana::efectos) == 2
               && rb.todo == 2 && rb.instrumentos == 1 && rb.efectos == 1 && rb.paquetes == 0);
    }
    {
        juce::var o;
        juce::JSON::parse (R"({"tag_name":"v0.4.0","name":"Barullo FM 0.4.0","published_at":"2026-10-22T09:00:00Z","assets":[
            {"name":"BarulloFM-0.4.0-mac.pkg","browser_download_url":"https://x/a.pkg","size":10},
            {"name":"BarulloFM-0.4.0-mac.zip","browser_download_url":"https://x/a.zip","size":20,"digest":"sha256:abc"},
            {"name":"BarulloFM-0.4.0-windows-x64.zip","browser_download_url":"https://x/w.zip","size":30},
            {"name":"BarulloFM-0.4.0-linux-x86_64.tar.gz","browser_download_url":"https://x/l.tgz","size":40}]})", o);
        auto l = GitHub::lanzamento (o);
        proba ("versión publicada: o arquivo de cada sistema, tamaño e SHA-256",
               l && l->version() && *l->version() == *Version::ler ("0.4.0") && l->arquivo (SO::mac)->url == "https://x/a.zip"
               && l->arquivo (SO::mac)->sha256 == "abc" && l->arquivo (SO::win)->tamano == 30
               && l->arquivo (SO::lin)->nome.endsWith (".tar.gz") && l->publicado.getYear() == 2026);
    }
    {
        auto m = Rutas::reais (SO::mac), w = Rutas::reais (SO::win), l = Rutas::reais (SO::lin);
        proba ("rutas: Mac, Windows e Linux (usuario e sistema)",
              #if ! JUCE_WINDOWS   // en Windows, juce::File pon a unidade diante das rutas de Mac
               m.usuario.ruta ("Barullo FM", Formato::vst3).getFullPathName().endsWith ("Library/Audio/Plug-Ins/VST3/Barullo FM.vst3")
               && m.sistema[0].ruta ("Barullo FM", Formato::vst3).getFullPathName() == "/Library/Audio/Plug-Ins/VST3/Barullo FM.vst3" &&
              #endif
               nomeFicheiro (SO::win, "Barullo FM", Formato::app) == "Barullo FM.exe"
               && w.sistema[0].appEnSubcartafol && w.usuario.cartafoles.count (Formato::au) == 0
               && nomeFicheiro (SO::lin, "Barullo Tune", Formato::app, "barullo-tune") == "barullo-tune"
               && l.sistema.size() == 2 && l.usuario.cartafoles.count (Formato::lv2) == 1);
        proba ("formatos: listas e claves", listaFormatos ("vst3, component,standalone") == std::vector<Formato> { Formato::vst3, Formato::au, Formato::app }
               && ! listaFormatos ("vst4") && formatosDe (SO::win).size() == 3);
    }

    // ------------------------------------------------------------ escáner e estados
    {
        Temporal t;
        const auto u = t.sub ("usuario"), s = t.sub ("sistema");
        Rutas r { Rutas::cartafol (u), { Rutas::cartafol (s, Ubicacion::Clase::sistema) } };
        Escaner e (r);
        auto fm = produto ("Barullo FM", {}, TipoProduto::instrumento);
        fm.ultima = lanzamentoDe ("0.5.1");
        const std::vector<Formato> vst { Formato::vst3 };
        proba ("estado: non instalado", Situacion { fm, e.instalados (fm), vst }.estado() == Estado::nonInstalado);
        facerBundle (u.getChildFile ("VST3"), "Barullo FM", Formato::vst3, "0.5.0");
        auto sit = Situacion { fm, e.instalados (fm), vst };
        proba ("estado: actualización (0.5.0 instalada, 0.5.1 publicada)",
               sit.estado() == Estado::actualizacion && sit.versionInstalada() && sit.versionInstalada()->texto() == "0.5.0");
        fm.ultima = lanzamentoDe ("0.5.0");
        proba ("estado: ao día", Situacion { fm, e.instalados (fm), vst }.estado() == Estado::alDia);
        facerBundle (s.getChildFile ("VST3"), "Barullo FM", Formato::vst3, "0.4.0");
        sit = Situacion { fm, e.instalados (fm), vst };
        proba ("estado: copia vella no sistema (o caso do FM 0.4.0 en /Library)",
               sit.estado() == Estado::copiaVella && sit.vellasDoSistema().size() == 1 && sit.duplicados().size() == 2);
        proba ("estado: os outros formatos non contan", Situacion { fm, e.instalados (fm), { Formato::clap } }.estado() == Estado::nonInstalado);
        auto local = produto ("Barullo ADT");
        local.repo = {};
        proba ("estado: só local (sen repo) e sen datos (non se puido preguntar)",
               Situacion { local, {}, vst }.estado() == Estado::soLocal
               && [&] { auto x = produto ("Barullo Ley"); x.lanzamentoConsultado = false; return Situacion { x, {}, vst }.estado() == Estado::senDatos; }());

        auto serie = produto ("Barullo Enxebres", { "Barullo Filtro", "Barullo Chorus" }, TipoProduto::paquete);
        serie.ultima = lanzamentoDe ("0.2.0");
        facerBundle (u.getChildFile ("VST3"), "Barullo Filtro", Formato::vst3, "0.2.0");
        auto ss = Situacion { serie, e.instalados (serie), vst };
        proba ("estado: serie incompleta (falta o Chorus)", ss.estado() == Estado::incompleto && ss.faltan() == juce::StringArray { "Barullo Chorus" });

        facerBundle (u.getChildFile ("VST3"), "Barullo Caseiro", Formato::vst3, "0.0.1");
        facerBundle (u.getChildFile ("VST3"), "Barullo Alleo", Formato::vst3, "1.0", "com.outro.alleo");
        auto orfos = e.orfos ({ fm, serie });
        proba ("órfos: os de Escoita sen repo coñecido (e non os alleos)",
               orfos.size() == 1 && orfos[0].plugin == "Barullo Caseiro" && e.produtoSolto ("caseiro").has_value());

        // a versión dun ficheiro que non a di (CLAP en Windows e Linux): o rexistro do xestor
        Rexistro rex (t.dir.getChildFile ("instalados.json"));
        const auto clap = u.getChildFile ("CLAP/Barullo FM.clap");
        clap.getParentDirectory().createDirectory();
        clap.replaceWithText ("x");
        rex.anotar (clap, "0.5.0");
        Escaner e2 (r, &rex);
        auto i = e2.ler ("Barullo FM", Formato::clap, r.usuario);
        proba ("rexistro: a versión do que instalou o xestor (mentres non cambie)",
               (so == SO::mac ? true : (i && i->textoVersion == "0.5.0")) && rex.version (clap) == "0.5.0");
    }

    // ------------------------------------------------------------ repos locais e capturas
    {
        Temporal t;
        const auto a = t.sub ("barullotune");
        a.getChildFile ("packaging").createDirectory();
        a.getChildFile ("packaging/produto.env").replaceWithText (envSinxelo ("Barullo Tune", "aumf Btun Esco"));
        a.getChildFile ("CMakeLists.txt").replaceWithText ("cmake_minimum_required(VERSION 3.22)\nproject(BarulloTune VERSION 0.4.1 LANGUAGES C CXX)\n");
        a.getChildFile ("docs/img").createDirectory();
        a.getChildFile ("docs/img/correccion.png").replaceWithText ("png");
        a.getChildFile ("docs/MANUAL.md").replaceWithText ("# Manual\n![A pestana](img/correccion.png)\n<img src=\"img/falta.png\">\n");
        const auto n = t.sub ("escoita-nucleo");
        n.getChildFile ("packaging").createDirectory();
        n.getChildFile ("packaging/produto.env").replaceWithText (envSinxelo ("Exemplo", ""));
        auto l = Descubridor::locais ({ t.dir });
        proba ("repos locais: produto.env, versión do CMakeLists e capturas do manual (sen o núcleo)",
               l.size() == 1 && l[0].nome == "Barullo Tune" && l[0].versionLocal && l[0].versionLocal->texto() == "0.4.1"
               && l[0].capturas.size() == 1 && l[0].capturas[0].endsWith ("correccion.png"));
        auto im = imaxesMarkdown ("x ![a](img/a.png \"t\") ![b](https://x/b.jpg) ![c](c.svg) <IMG alt='z' src='d.gif'>");
        proba ("imaxes no Markdown (sen SVG; ![...](...) e <img src>)", im == juce::StringArray { "img/a.png", "https://x/b.jpg", "d.gif" });
    }

    // ------------------------------------------------------------ GitHub: caché, ETag e sen rede
    {
        Temporal t;
        GitHubFalso g;
        encherGitHub (g, t);
        GitHub gh ("callejodev-dotcom", t.sub ("cache"), g);
        auto repos = gh.repos();
        const int despois = g.pedidos;
        gh.repos();
        proba ("GitHub: a segunda vez sae da caché (non se pregunta)", repos.size() == 5 && g.pedidos == despois);
        gh.vixencia = juce::RelativeTime::seconds (-1);
        auto r2 = gh.repos();
        proba ("GitHub: pasada a vixencia, pregúntase co ETag (304 e o gardado)", g.conEtag >= 1 && r2.size() == 5 && gh.restantes() == 59);
        g.senRede = true;
        auto r3 = gh.repos();
        proba ("GitHub: sen rede, o último que se soubo (e avísase)", r3.size() == 5 && gh.usouCacheVella());
        GitHub baleiro ("callejodev-dotcom", t.sub ("cache2"), g);
        bool lanzou = false;
        try { baleiro.repos(); } catch (const ErroGitHub&) { lanzou = true; }
        proba ("GitHub: sen rede e sen caché, erro claro", lanzou);
    }
    {
        Temporal t;
        GitHubFalso g;
        encherGitHub (g, t);
        GitHub gh ("callejodev-dotcom", t.sub ("cache"), g);
        Axustes ax;
        ax.cartafolesLocais = { t.sub ("locais").getFullPathName() };
        auto c = Descubridor (&gh, ax).descubrir();
        const auto* fm = c.buscar ("fm");
        const auto* ley = c.buscar ("ley");
        const auto* enx = c.buscar ("enxebres");
        proba ("descubrir: os repos con produto.env (e non os arquivados nin os outros)",
               c.produtos.size() == 3 && fm && ley && enx && c.buscar ("portnhse") == nullptr && c.avisos.isEmpty());
        proba ("descubrir: orde (instrumentos, efectos, paquetes) e versións publicadas",
               c.produtos[0].nome == "Barullo FM" && c.produtos[2].eSerie() && fm->ultima && fm->ultima->version()->texto() == "0.5.1"
               && enx->lanzamentoConsultado && ! enx->ultima);
        proba ("capturas: a estándar (docs/img/captura.png) e as do LEEME (sen insignias)",
               fm->capturas == juce::StringArray { "https://raw.githubusercontent.com/callejodev-dotcom/barullofm/main/docs/img/captura.png" }
               && ley->capturas == juce::StringArray { "https://raw.githubusercontent.com/callejodev-dotcom/barulloley/main/docs/img/ley.png" }
               && enx->capturas.isEmpty() && gh.imaxe (fm->capturas[0]).getSize() == 3);
        int apiAntes = 0;
        for (auto& u : g.urls) if (u.contains ("api.github.com")) ++apiAntes;
        proba ("capturas: sen gastar preguntas da API (só raw.githubusercontent.com)", apiAntes == 1 + 3);
    }

    // ------------------------------------------------------------ instalar
    {
        Temporal t;
        GitHubFalso g;
        const auto u = t.sub ("plugins");
        Rutas r { Rutas::cartafol (u), {} };
        CartafolLixo lixo (t.sub ("lixo"));
        Rexistro rex (t.dir.getChildFile ("rex.json"));
        Instalador inst (r, lixo, nullptr, g, t.sub ("tmp"), &rex);
        auto ley = produto ("Barullo Ley");
        const auto zip = facerArquivo (t, "BarulloLey", "0.1.1", { { "Barullo Ley", Formato::vst3 }, { "Barullo Ley", Formato::clap }, { "Outro", Formato::vst3 } },
                                       true, true);
        ley.ultima = lanzamentoDe ("0.1.1", zip, sha256 (zip), zip.getSize());
        auto inf = inst.instalar (ley, { Formato::vst3 });
        const auto b = u.getChildFile ("VST3/Barullo Ley.vst3");
        bool alleo = false;
        for (auto& a : inf.accions) alleo = alleo || (a.tipo == Accion::Tipo::aviso && a.plugin == "Outro");
        proba ("instalar: descarga, comproba a suma, instala o VST3 (e non o alleo)",
               inf.ok() && b.isDirectory() && ! u.getChildFile ("CLAP/Barullo Ley.clap").exists() && alleo && inf.cambios() == 1
               && infoBundle::versionModuleInfo (b) == "0.1.1");
       #if JUCE_MAC
        proba ("instalar (Mac): sen cuarentena e coa firma boa", ! Instalador::temCuarentena (b) && Instalador::firmaValida (b));
       #endif
        auto igual = inst.instalar (ley, { Formato::vst3 });
        proba ("instalar: a mesma versión non se pisa (omitido)", igual.ok() && igual.cambios() == 0 && igual.accions.back().tipo == Accion::Tipo::omitido);
        const auto zip2 = facerArquivo (t, "BarulloLey", "0.1.2", { { "Barullo Ley", Formato::vst3 } }, false);
        auto act = inst.instalarArquivo (zip2, ley, { Formato::vst3 }, Version::ler ("0.1.2"));
        proba ("actualizar: o vello ao Lixo (nunca se borra) e o novo no seu sitio",
               act.ok() && infoBundle::versionModuleInfo (b) == "0.1.2" && lixo.cartafol.getChildFile ("Barullo Ley.vst3").isDirectory()
               && infoBundle::versionModuleInfo (lixo.cartafol.getChildFile ("Barullo Ley.vst3")) == "0.1.1");
       #if JUCE_MAC
        proba ("actualizar (Mac): un bundle sen firma asínase «ad hoc»", Instalador::firmaValida (b));
       #endif
        auto baixar = inst.instalarArquivo (zip, ley, { Formato::vst3 }, Version::ler ("0.1.1"));
        auto forzar = inst.instalarArquivo (zip, ley, { Formato::vst3 }, Version::ler ("0.1.1"), true);
        proba ("instalar: non se baixa de versión sen --forzar; con --forzar si",
               baixar.cambios() == 0 && forzar.cambios() == 1 && infoBundle::versionModuleInfo (b) == "0.1.1");
        auto mala = ley;
        mala.ultima = lanzamentoDe ("0.1.1", zip, "00ff", zip.getSize());
        bool erroSuma = false;
        try { inst.instalar (mala, { Formato::vst3 }); } catch (const ErroInstalacion& e) { erroSuma = e.texto().contains ("SHA-256"); }
        auto curta = ley;
        curta.ultima = lanzamentoDe ("0.1.1", zip, {}, zip.getSize() + 1);
        bool erroTam = false;
        try { inst.instalar (curta, { Formato::vst3 }); } catch (const ErroInstalacion& e) { erroTam = e.texto().contains ("mide"); }
        proba ("instalar: unha suma ou un tamaño que non coinciden paran todo", erroSuma && erroTam);

        auto serie = produto ("Barullo Enxebres", { "Barullo Filtro", "Barullo Chorus" }, TipoProduto::paquete);
        const auto zs = facerArquivo (t, "BarulloEnxebres", "0.2.0", { { "Barullo Filtro", Formato::vst3 }, { "Barullo Chorus", Formato::vst3 } });
        auto is = inst.instalarArquivo (zs, serie, { Formato::vst3, Formato::clap }, Version::ler ("0.2.0"));
        bool avisoClap = false;
        for (auto& a : is.accions) avisoClap = avisoClap || (a.tipo == Accion::Tipo::aviso && a.mensaxe.contains ("CLAP"));
        proba ("instalar unha serie: todos os plugins (e avisa do formato que non trae)",
               is.cambios() == 2 && u.getChildFile ("VST3/Barullo Chorus.vst3").isDirectory() && avisoClap);

        auto des = inst.desinstalar (ley, { Formato::vst3 }, false);
        proba ("desinstalar: ao Lixo (e o Lixo garda as dúas co seu nome)",
               des.ok() && ! b.exists() && lixo.cartafol.getChildFile ("Barullo Ley 2.vst3").exists());
    }

    // ------------------------------------------------------------ permiso de administrador
    {
        // o caso do FM: 0.5.0 no cartafol do usuario e a 0.4.0 no do sistema (que precisa admin)
        Temporal t;
        GitHubFalso g;
        const auto u = t.sub ("usuario"), s = t.sub (U ("sistema con 'comiña'"));
        Rutas r { Rutas::cartafol (u), { Rutas::cartafol (s, Ubicacion::Clase::sistema) } };
        CartafolLixo lixo (t.sub ("lixo"));
        ElevadorSenPermiso elev;
        Instalador inst (r, lixo, &elev, g, t.sub ("tmp"));
        inst.simularAdmin = true;
        auto fm = produto ("Barullo FM", {}, TipoProduto::instrumento);
        fm.ultima = lanzamentoDe ("0.5.0");
        facerBundle (u.getChildFile ("VST3"), "Barullo FM", Formato::vst3, "0.5.0");
        const auto vella = facerBundle (s.getChildFile ("VST3"), "Barullo FM", Formato::vst3, "0.4.0");
        Escaner e (r);
        auto sit = Situacion { fm, e.instalados (fm), { Formato::vst3 } };
        elev.cancelar = true;
        auto non = inst.botar (sit.vellasDoSistema());
        proba ("admin: se se di que non, a copia do sistema queda onde estaba",
               non.cancelouAdmin && vella.isDirectory() && elev.chamadas == 1 && non.cambios() == 0);
        elev.cancelar = false;
        auto si = inst.botar (sit.vellasDoSistema());
        proba ("admin: a copia vella do sistema sae cun permiso e vai ao Lixo",
               si.ok() && ! vella.exists() && si.conAdmin == 1 && elev.chamadas == 2
               && lixo.cartafol.getChildFile ("Barullo FM.vst3").isDirectory()
               && Situacion { fm, e.instalados (fm), { Formato::vst3 } }.estado() == Estado::alDia
               && elev.ultimoMotivo.contains ("Barullo FM.vst3"));
       #if ! JUCE_WINDOWS
        proba ("admin: o guion (mv e chown ao usuario, sen pisar nada)",
               elev.ultimoGuion.contains ("mv ") && elev.ultimoGuion.contains ("chown -R " + juce::String ((int) getuid()))
               && elev.ultimoGuion.contains ("exit 3") && elev.ultimoGuion.contains ("'\\''"));
       #endif
        proba ("admin: non quedan restos no cartafol temporal", t.dir.getChildFile ("tmp").getNumberOfChildFiles (juce::File::findFilesAndDirectories) == 0);

        // actualizar con copia vella no sistema: instala no usuario e quita a do sistema cun só permiso
        const auto vella2 = facerBundle (s.getChildFile ("VST3"), "Barullo FM", Formato::vst3, "0.4.0");
        facerBundle (s.getChildFile ("CLAP"), "Barullo FM", Formato::clap, "0.4.0");
        const auto zip = facerArquivo (t, "BarulloFM", "0.5.1", { { "Barullo FM", Formato::vst3 }, { "Barullo FM", Formato::clap } });
        auto act = inst.instalarArquivo (zip, fm, { Formato::vst3, Formato::clap }, Version::ler ("0.5.1"));
        proba ("admin: actualizar quita as copias vellas do sistema (un só permiso para as dúas)",
               act.ok() && ! vella2.exists() && ! s.getChildFile ("CLAP/Barullo FM.clap").exists() && elev.chamadas == 3 && act.conAdmin == 2
               && infoBundle::versionModuleInfo (u.getChildFile ("VST3/Barullo FM.vst3")) == "0.5.1");

        facerBundle (s.getChildFile ("VST3"), "Barullo FM", Formato::vst3, "0.5.1");
        auto d1 = inst.desinstalar (fm, { Formato::vst3 }, false);
        auto d2 = inst.desinstalar (fm, { Formato::vst3 }, true);
        proba ("admin: desinstalar só toca o sistema con --incluir-sistema (e entón pide permiso)",
               d1.ok() && d1.conAdmin == 0 && s.getChildFile ("VST3/Barullo FM.vst3").exists() == false && d2.conAdmin == 1 && elev.chamadas == 4);

        ElevadorNon nunca;
        Instalador sen (r, lixo, &nunca, g, t.sub ("tmp"));
        sen.simularAdmin = true;
        const auto v3 = facerBundle (s.getChildFile ("VST3"), "Barullo FM", Formato::vst3, "0.3.0");
        auto ns = sen.desinstalar (fm, { Formato::vst3 }, true);
        proba ("admin: con --sen-admin, o que precisa permiso queda (e dise por que)", v3.exists() && ! ns.ok());
    }
    {
        // a orde de AppleScript (comiñas e barras) e o paso de Windows (só desde os cartafoles do sistema)
        const auto as = admin::appleScript ("mv 'a \"b\"' c", U ("Barullo Xestor quere levar ao Lixo «x»"));
        proba ("admin (Mac): a orde de osascript escapa as comiñas e leva o motivo",
               as == U ("do shell script \"mv 'a \\\"b\\\"' c\" with prompt \"Barullo Xestor quere levar ao Lixo «x»\" with administrator privileges"));
        Temporal t;
        const auto sis = t.sub ("sistema"), dest = t.sub ("tmp");
        const auto b = facerBundle (sis, "Barullo FM", Formato::vst3, "0.4.0");
        const auto fora = facerBundle (t.sub ("casa"), "Barullo FM", Formato::vst3, "0.4.0");
        auto mal = admin::executarOperacions (admin::operacionsJson ({ { fora, dest.getChildFile ("x.vst3") } }), { sis }, dest);
        auto mal2 = admin::executarOperacions (admin::operacionsJson ({ { b, t.dir.getChildFile ("fora.vst3") } }), { sis }, dest);
        auto ben = admin::executarOperacions (admin::operacionsJson ({ { b, dest.getChildFile ("Barullo FM.vst3") } }), { sis }, dest);
        proba ("admin (Windows): --operacions só move do sistema ao temporal",
               mal.failed() && mal2.failed() && fora.exists() && ben.wasOk() && ! b.exists() && dest.getChildFile ("Barullo FM.vst3").isDirectory());
    }

   #if JUCE_LINUX
    {
        Temporal t;
        setenv ("XDG_DATA_HOME", t.dir.getFullPathName().toRawUTF8(), 1);
        const auto f = t.sub ("x").getChildFile ("Barullo Proba.clap");
        f.replaceWithText ("x");
        juce::File onde;
        auto r = LixoDoSistema().botar (f, &onde);
        const auto info = t.dir.getChildFile ("Trash/info/Barullo Proba.clap.trashinfo");
        proba ("Lixo (Linux): freedesktop.org, co .trashinfo para restaurar",
               r.wasOk() && ! f.exists() && onde.existsAsFile() && info.loadFileAsString().contains ("Path=/") && info.loadFileAsString().contains ("Barullo%20Proba.clap"));
        unsetenv ("XDG_DATA_HOME");
    }
   #endif

    // ------------------------------------------------------------ liña de ordes
    {
        Temporal t;
        GitHubFalso g;
        encherGitHub (g, t);
        juce::StringArray saida, erros;
        cli::Contexto ctx;
        ctx.transporte = &g;
        ctx.axustes = t.dir.getChildFile ("axustes.json");
        ctx.cache = t.sub ("cache");
        ctx.rexistro = t.dir.getChildFile ("rex.json");
        ctx.temporal = t.sub ("tmp");
        ctx.usarToken = false;
        ctx.saida = [&] (const juce::String& s) { saida.add (s); };
        ctx.erro = [&] (const juce::String& s) { erros.add (s); };
        Axustes ax;
        ax.cartafolesLocais.clear();
        ax.gardar (ctx.axustes);
        const auto u = t.sub ("plugins"), s = t.sub ("sistema"), lixo = t.sub ("lixo");
        auto cli = [&] (juce::StringArray a) { saida.clear(); erros.clear(); return cli::executar (a, ctx); };
        const juce::StringArray base { "--dir", u.getFullPathName(), "--dir-sistema", s.getFullPathName(), "--papeleira", lixo.getFullPathName() };

        proba ("CLI: --axuda e unha opción que non existe", cli ({ "--axuda" }) == 0 && saida.joinIntoString ("\n").contains ("--instalar")
                                                              && cli ({ "--voar" }) == 2 && cli ({ "--simular-admin", "--listar" }) == 2);
        auto args = base; args.addArray ({ "--listar", "--json" });
        const int c1 = cli (args);
        const auto j = juce::JSON::parse (saida.joinIntoString ("\n"));
        proba ("CLI: --listar --json (produtos, estado e capturas)",
               c1 == 0 && j["produtos"].size() == 3 && j["produtos"][0]["nome"] == "Barullo FM" && j["produtos"][0]["estado"] == "nonInstalado"
               && j["produtos"][0]["capturas"].size() == 1);
        args = base; args.addArray ({ "--instalar", "fm" });
        const int c2 = cli (args);
        proba ("CLI: --instalar fm (descarga do GitHub de mentira)", c2 == 0 && u.getChildFile ("VST3/Barullo FM.vst3").isDirectory());
        facerBundle (s.getChildFile ("VST3"), "Barullo FM", Formato::vst3, "0.4.0");
        args = base; args.addArray ({ "--listar" });
        cli (args);
        proba ("CLI: --listar di «copia vella no sistema»", saida.joinIntoString ("\n").contains ("copia vella no sistema"));
        args = base; args.addArray ({ "--actualizar", "Barullo FM", "--simular-admin" });
        const int c3 = cli (args);
        proba ("CLI: --actualizar co permiso simulado leva a copia do sistema ao Lixo",
               c3 == 0 && ! s.getChildFile ("VST3/Barullo FM.vst3").exists() && saida.joinIntoString ("\n").contains ("permiso de administrador"));
        args = base; args.addArray ({ "--actualizar-todo" });
        cli (args);
        proba ("CLI: --actualizar-todo («Todo ao día» cando non hai nada)", saida.joinIntoString ("\n").contains (U ("ao día")));
        args = base; args.addArray ({ "--desinstalar", "fm" });
        const int c4 = cli (args);
        proba ("CLI: --desinstalar fm", c4 == 0 && ! u.getChildFile ("VST3/Barullo FM.vst3").exists() && lixo.getNumberOfChildFiles (juce::File::findFilesAndDirectories) >= 2);
        args = base; args.addArray ({ "--instalar", "non-existe" });
        proba ("CLI: un nome que non existe", cli (args) == 1 && erros.joinIntoString ("").contains ("Barullo Ley"));
        g.senRede = true;
        args = base; args.addArray ({ "--listar", "--sen-rede" });
        proba ("CLI: --sen-rede usa a caché", cli (args) == 0 && saida.joinIntoString ("\n").contains ("Barullo Ley"));
    }

    // ------------------------------------------------------------ idiomas
    {
        proba ("idioma: o do sistema (gl, es; se non, inglés)",
               idiomaDoCodigo ("gl-ES") == Idioma::gl && idiomaDoCodigo ("gl_ES.UTF-8") == Idioma::gl && idiomaDoCodigo ("es") == Idioma::es
               && idiomaDoCodigo ("es-419") == Idioma::es && idiomaDoCodigo ("en-US") == Idioma::en && idiomaDoCodigo ("pt-BR") == Idioma::en
               && idiomaDoCodigo ("") == Idioma::en && idiomaDeClave ("ES") == Idioma::es && ! idiomaDeClave (""));
        bool repetidas = false, baleiras = false, marcas = true;
        std::set<std::string> vistas;
        for (auto& e : textos::taboa())
        {
            repetidas = repetidas || ! vistas.insert (e.gl).second;
            baleiras = baleiras || juce::String::fromUTF8 (e.es).isEmpty() || juce::String::fromUTF8 (e.en).isEmpty();
            for (auto m : { "%1", "%2", "%3" })
            {
                const bool g = juce::String::fromUTF8 (e.gl).contains (m);
                if (g != juce::String::fromUTF8 (e.es).contains (m) || g != juce::String::fromUTF8 (e.en).contains (m))
                {
                    marcas = false;
                    std::printf ("  (as marcas non casan: %s)\n", e.gl);
                }
            }
        }
        proba ("traducións: sen chaves repetidas nin baleiras, e os %1 casan", ! repetidas && ! baleiras && marcas && textos::taboa().size() > 100);
        porIdioma (Idioma::es);
        const auto es = tr ("Instalar");
        porIdioma (Idioma::en);
        const auto en = tr ("Instalar"), falta = tr ("Isto non está na táboa");
        const auto conArg = tr ("Volver á %1", "0.5.0");
        porIdioma (Idioma::gl);
        proba ("traducións: tr segundo o idioma, os %1 e o que falta queda en galego (e anótase)",
               es == "Instalar" && en == "Install" && tr ("Instalar") == "Instalar" && falta == U ("Isto non está na táboa")
               && textos::senTraducion().contains (U ("Isto non está na táboa")) && conArg == "Go back to 0.5.0"
               && tr ("Volver á %1", "%2", "x") == U ("Volver á %2"));
        proba ("traducións: os meses", textos::mesCurto (6) == U ("xuñ") && [] { porIdioma (Idioma::en); auto m = textos::mesCurto (12); porIdioma (Idioma::gl); return m == "Dec"; }());
        const auto md = textos::markdownLexible (U ("## Novidades\n\n- **Máis** rápido\n  - un `detalle`\n* [Ligazón](https://x) e ![img](a.png)\n"));
        const auto md2 = textos::markdownLexible (U ("| Sistema | Descarga |\n|---|:--:|\n| Mac | *a.zip* |\nclic dereito → *Abrir*; 2 * 3"));
        proba ("notas: o Markdown, lexible (listas, ligazóns, táboas e cursivas)",
               md == juce::String::fromUTF8 ("Novidades\n\n• Máis rápido\n  • un detalle\n• Ligazón e ")
               && md2 == U ("Sistema · Descarga\nMac · a.zip\nclic dereito → Abrir; 2 * 3"));
    }

    // ------------------------------------------------------------ DAW abertos e sistema
    {
        const juce::StringArray mac { "/sbin/launchd", "/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live",
                                      "/Applications/REAPER.app/Contents/MacOS/REAPER", "/System/Library/CoreServices/Finder.app/Contents/MacOS/Finder",
                                      "/Users/x/Applications/Barullo Xestor.app/Contents/MacOS/Barullo Xestor", "/Applications/Logic Pro.app/Contents/MacOS/Logic Pro",
                                      "/Applications/Livestream Studio.app/Contents/MacOS/Livestream" };
        const juce::StringArray win { "\"System Idle Process\",\"0\",\"Services\",\"0\",\"8 K\"", "\"FL64.exe\",\"4242\",\"Console\",\"1\",\"300.000 K\"",
                                      "\"Studio One.exe\",\"77\",\"Console\",\"1\",\"1 K\"", "\"explorer.exe\",\"1\",\"Console\",\"1\",\"1 K\"" };
        const juce::StringArray lin { "systemd", "bitwig-studio", "/opt/bitwig-studio/bin/BitwigStudio --x /home/a/b", "ardour8",
                                      "/usr/bin/reaper /home/x/canción.rpp", "/usr/lib/firefox/firefox" };
        proba ("DAW: Mac (o .app), Windows (tasklist) e Linux (ps), sen falsos positivos",
               sistema::daws (mac) == juce::StringArray { "Ableton Live", "Logic Pro", "REAPER" }
               && sistema::daws (win) == juce::StringArray { "FL Studio", "Studio One" }
               && sistema::daws (lin) == juce::StringArray { "REAPER", "Bitwig Studio", "Ardour" }
               && sistema::daws ({ "/usr/bin/true", "", "Barullo Xestor" }).isEmpty() && sistema::todosOsDaws().contains ("Cubase"));
        const auto n = sistema::ordeNotificacion (SO::mac, U ("Barullo \"Xestor\""), U ("Hai 2 versións"));
        proba ("notificación: osascript (comiñas escapadas) e notify-send",
               n.size() == 3 && n[2] == U ("display notification \"Hai 2 versións\" with title \"Barullo \\\"Xestor\\\"\"")
               && sistema::ordeNotificacion (SO::lin, "a", "b")[0] == "notify-send" && sistema::ordeNotificacion (SO::win, "a", "b").isEmpty());
        proba ("firma: ad hoc, Developer ID, sen firma e non válida",
               sistema::resumoFirma (true, "Executable=/x\nSignature=adhoc\nTeamIdentifier=not set\n") == "ad hoc"
               && sistema::resumoFirma (true, "Authority=Developer ID Application: Escoita (AB12)\nAuthority=Apple Root CA\n") == "Developer ID: Escoita (AB12)"
               && sistema::resumoFirma (false, "/x: code object is not signed at all\n") == "sen firma"
               && sistema::resumoFirma (false, "Signature=adhoc\n") == U ("non válida (ad hoc)")
               && sistema::descricionSO().isNotEmpty());
    }

    // ------------------------------------------------------------ copias para volver atrás e presets
    {
        Temporal t;
        GitHubFalso g;
        const auto u = t.sub ("plugins");
        Rutas r { Rutas::cartafol (u), {} };
        CartafolLixo lixo (t.sub ("lixo"));
        Copias copias (t.dir.getChildFile ("copias"));
        Instalador inst (r, lixo, nullptr, g, t.sub ("tmp"));
        inst.copias = &copias;
        auto ley = produto ("Barullo Ley");
        const auto b = u.getChildFile ("VST3/Barullo Ley.vst3");
        inst.instalarArquivo (facerArquivo (t, "BarulloLey", "0.1.0", { { "Barullo Ley", Formato::vst3 } }), ley, { Formato::vst3 }, Version::ler ("0.1.0"));
        proba ("copias: instalar a primeira vez non garda nada", copias.versions (ley).empty());
        auto a1 = inst.instalarArquivo (facerArquivo (t, "BarulloLey", "0.1.1", { { "Barullo Ley", Formato::vst3 } }), ley, { Formato::vst3 }, Version::ler ("0.1.1"));
        const auto gardada = copias.cartafolDe (ley, *Version::ler ("0.1.0")).getChildFile ("vst3/Barullo Ley.vst3");
        proba ("copias: ao actualizar gárdase a de antes (e o vello vai igual ao Lixo)",
               a1.ok() && copias.versions (ley).size() == 1 && copias.versions (ley)[0].texto() == "0.1.0"
               && infoBundle::versionModuleInfo (gardada) == "0.1.0" && lixo.cartafol.getChildFile ("Barullo Ley.vst3").isDirectory());
        auto volta = inst.restaurar (ley, *Version::ler ("0.1.0"), { Formato::vst3 });
        auto vs = copias.versions (ley);
        proba ("copias: volver á 0.1.0 sen rede (e a 0.1.1 queda gardada para volver)",
               volta.ok() && volta.cambios() == 1 && infoBundle::versionModuleInfo (b) == "0.1.0" && vs.size() == 2
               && vs[0].texto() == "0.1.1" && vs[1].texto() == "0.1.0" && gardada.isDirectory() && g.pedidos == 0);
       #if JUCE_MAC
        proba ("copias (Mac): a restaurada coa firma boa", Instalador::firmaValida (b));
       #endif
        inst.instalarArquivo (facerArquivo (t, "BarulloLey", "0.2.0", { { "Barullo Ley", Formato::vst3 } }), ley, { Formato::vst3 }, Version::ler ("0.2.0"));
        vs = copias.versions (ley);
        proba ("copias: gárdanse as dúas máis novas (a máis vella quítase)", vs.size() == 2 && vs[0].texto() == "0.1.1" && vs[1].texto() == "0.1.0"
               && [&] { copias.maximo = 1; inst.instalarArquivo (facerArquivo (t, "BarulloLey", "0.2.1", { { "Barullo Ley", Formato::vst3 } }), ley, { Formato::vst3 }, Version::ler ("0.2.1"));
                        auto v2 = copias.versions (ley); return v2.size() == 1 && v2[0].texto() == "0.2.0"; }());
        bool sen = false;
        try { inst.restaurar (ley, *Version::ler ("0.0.1"), { Formato::vst3 }); } catch (const ErroInstalacion&) { sen = true; }
        proba ("copias: volver a unha que non está gardada, erro claro", sen);

        const auto docs = t.sub ("Documentos");
        const auto pr = docs.getChildFile ("Barullo Ley/Presets");
        pr.createDirectory();
        pr.getChildFile (U ("Cálido.bley")).replaceWithText ("<preset/>");
        pr.getChildFile ("Sub/Outro.bley").create();
        auto serie = produto ("Barullo Enxebres", { "Barullo Filtro", "Barullo Chorus" }, TipoProduto::paquete);
        docs.getChildFile ("Barullo Chorus/Presets").createDirectory();
        juce::File zip;
        auto rz = presets::copiaDeSeguridade (ley, t.dir.getChildFile ("copiasPresets"), zip, docs, juce::Time (2026, 10, 2, 10, 30));
        juce::ZipFile zf (zip);
        juce::File zip2;
        proba ("presets: os cartafoles (tamén os de cada plugin dunha serie) e a copia en .zip",
               presets::cartafoles (ley, docs).size() == 1 && presets::cartafoles (serie, docs).size() == 1
               && rz.wasOk() && zip.getFileName() == "Barullo Ley presets 2026-11-02 10.30.zip" && zf.getNumEntries() == 2
               && zf.getIndexOfFileName (U ("Barullo Ley/Presets/Cálido.bley")) >= 0
               && presets::copiaDeSeguridade (serie, t.dir, zip2, docs).failed()
               && presets::cartafolCopias (docs) == docs.getChildFile ("Barullo Xestor/Copias dos presets"));
    }

    // ------------------------------------------------------------ notas da versión e manual
    {
        Temporal t;
        GitHubFalso g;
        encherGitHub (g, t);
        juce::var o;
        juce::JSON::parse (U (R"({"tag_name":"v0.6.0","body":"## Novidades\r\n- Máis graves","assets":[]})"), o);
        auto l = GitHub::lanzamento (o);
        GitHub gh ("callejodev-dotcom", t.sub ("cache"), g);
        auto sen = GitHub::lanzamento (juce::JSON::parse (R"({"tag_name":"v0.1.1","body":null,"assets":[]})"));
        g.json ("https://raw.githubusercontent.com/callejodev-dotcom/barulloley/v0.1.1/NOTAS-VERSION.md", U ("- Arranxos\n"));
        proba ("notas: o texto da versión en GitHub e, se non o ten, o NOTAS-VERSION.md da etiqueta",
               l && l->notas == U ("## Novidades\n- Máis graves") && sen && sen->notas.isEmpty()
               && gh.notas ("callejodev-dotcom/barulloley", *sen) == U ("- Arranxos")
               && gh.notas ("callejodev-dotcom/barullofm", *l) == l->notas
               && gh.notas ("callejodev-dotcom/barullofm", *sen).isEmpty());
        Axustes ax;
        ax.cartafolesLocais.clear();
        auto c = Descubridor (&gh, ax).descubrir();
        proba ("manual: a páxina do LEEME (ou docs/MANUAL.md) en GitHub; sen ningún, baleiro",
               c.buscar ("ley")->manual == "https://github.com/callejodev-dotcom/barulloley/blob/main/LEEME.md" && c.buscar ("fm")->manual.isEmpty());
    }

    // ------------------------------------------------------------ o propio xestor ao día
    {
        juce::var o;
        juce::JSON::parse (R"({"tag_name":"v0.4.0","assets":[
            {"name":"BarulloXestor-0.4.0-mac.zip","browser_download_url":"https://x/m.zip","size":1},
            {"name":"BarulloXestor-0.4.0-windows-x64.zip","browser_download_url":"https://x/w.zip","size":1},
            {"name":"BarulloXestor-0.4.0-windows-x64-instalador.exe","browser_download_url":"https://x/w.exe","size":1},
            {"name":"BarulloXestor-0.4.0-linux-x86_64.tar.gz","browser_download_url":"https://x/l.tgz","size":1}]})", o);
        auto l = GitHub::lanzamento (o);
        proba ("xestor: o arquivo de cada sistema (en Windows, o instalador)",
               autoactualizacion::activo (*l, SO::mac)->url == "https://x/m.zip" && autoactualizacion::activo (*l, SO::win)->url == "https://x/w.exe"
               && autoactualizacion::activo (*l, SO::lin)->url == "https://x/l.tgz");
        proba ("xestor: hai versión nova só se é máis nova e trae o arquivo",
               autoactualizacion::nova (l, "0.3.0").has_value() && ! autoactualizacion::nova (l, "0.4.0") && ! autoactualizacion::nova (l, "1.0")
               && ! autoactualizacion::nova (std::nullopt, "0.1") && ! autoactualizacion::nova (GitHub::lanzamento (juce::JSON::parse (R"({"tag_name":"v9.0","assets":[]})")), "0.1"));

        Temporal t;
        GitHubFalso g;
        // o arquivo publicado, coma os de empaquetado/: Mac o .app nun .zip, Linux o binario nun .tar.gz, Windows o instalador
        const auto ver = juce::String ("0.9.0");
        juce::File arq;
        if (so == SO::win)
        {
            arq = t.dir.getChildFile ("BarulloXestor-0.9.0-windows-x64-instalador.exe");
            arq.replaceWithText ("instalador");
        }
        else
        {
            const juce::String suf = so == SO::mac ? "mac" : "linux-x86_64";
            const auto pai = t.sub ("fonte");
            const auto d = so == SO::mac ? pai : pai.getChildFile ("BarulloXestor-0.9.0-" + suf);
            facerBundle (d, "Barullo Xestor", Formato::app, ver);
            if (so == SO::mac)
            {
                arq = t.dir.getChildFile ("BarulloXestor-0.9.0-mac.zip");
                executar ({ "/usr/bin/ditto", "-c", "-k", "--sequesterRsrc", "--keepParent", d.getChildFile ("Barullo Xestor.app").getFullPathName(), arq.getFullPathName() });
            }
            else
            {
                arq = t.dir.getChildFile ("BarulloXestor-0.9.0-linux-x86_64.tar.gz");
                executar ({ "tar", "-czf", arq.getFullPathName(), "-C", pai.getFullPathName(), d.getFileName() });
            }
        }
        g.ficheiros["https://x/xestor"] = arq;
        Lanzamento lx;
        lx.tag = "v0.9.0";
        lx.activos.push_back ({ arq.getFileName(), "https://x/xestor", arq.getSize(), sha256 (arq) });
        juce::File lista;
        auto rp = autoactualizacion::preparar (lx, g, t.sub ("traballo"), lista);
        proba ("xestor: descarga, comproba a suma e deixa lista a nova",
               rp.wasOk() && lista.exists() && (so != SO::mac || (lista.getFileName() == "Barullo Xestor.app" && Instalador::firmaValida (lista)))
               && (so != SO::lin || (lista.getFileName() == "barullo-xestor" && executable (lista))));
        auto mal = lx;
        mal.activos[0].sha256 = "00";
        juce::File l2;
        proba ("xestor: unha suma que non casa, non se toca nada", autoactualizacion::preparar (mal, g, t.sub ("traballo2"), l2).failed());
        if (so != SO::win)
        {
            const auto apps = t.sub ("Applications");
            const auto vella = facerBundle (apps, "Barullo Xestor", Formato::app, "0.3.0");
            CartafolLixo lixo (t.sub ("lixo"));
            auto rs = autoactualizacion::substituir (lista, vella, lixo);
            proba ("xestor: a nova no sitio da vella (a vella, ao Lixo)",
                   rs.wasOk() && vella.exists() && ! lista.exists() && lixo.cartafol.getNumberOfChildFiles (juce::File::findFilesAndDirectories) == 1
                   && (so != SO::mac || infoBundle::versionPlist (vella) == "0.9.0"));
        }
        const auto om = autoactualizacion::ordeReinicio (SO::mac, 123, juce::File ("/Users/x/Applications/Barullo Xestor.app"));
        const auto ol = autoactualizacion::ordeReinicio (SO::lin, 123, juce::File ("/home/x/.local/bin/barullo-xestor"));
        const auto gw = autoactualizacion::guionWindows (77, juce::File ("C:\\Temp\\i.exe"), juce::File ("C:\\Users\\A B\\Barullo Xestor.exe"));
        proba ("xestor: reiniciar cando remata este proceso (sh → open; cmd → o instalador en silencio)",
               om[0] == "/bin/sh" && om[2].contains ("kill -0 123") && om[2].contains ("open \"$0\"") && om[3].endsWith ("Barullo Xestor.app")
               && ol[2].contains ("exec \"$0\"") && gw.contains ("PID eq 77") && gw.contains ("/VERYSILENT") && gw.contains ("Barullo Xestor.exe\"")
               && autoactualizacion::ordeReinicio (SO::win, 1, {}, juce::File ("C:\\t\\r.cmd"))[0] == "cmd.exe");
       #if ! JUCE_WINDOWS
        const auto casa = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
        proba ("xestor: onde vai a nova (o mesmo sitio; se o Mac a abriu dun sitio provisional, ~/Applications)",
               autoactualizacion::destino (juce::File ("/private/var/folders/x/AppTranslocation/y/d/Barullo Xestor.app"), SO::mac)
                   == casa.getChildFile ("Applications/Barullo Xestor.app")
               && autoactualizacion::destino (t.sub ("Apps").getChildFile ("Barullo Xestor.app"), SO::mac) == casa.getChildFile ("Applications/Barullo Xestor.app")
               && [&] { auto a = facerBundle (t.sub ("Apps2"), "Barullo Xestor", Formato::app, "0.1"); return so != SO::mac || autoactualizacion::destino (a, SO::mac) == a; }());
       #endif
        const auto agora = juce::Time::getCurrentTime();
        proba ("comprobación: unha vez ao día (e se o reloxo foi cara atrás)",
               comprobacion::toca (0, agora) && ! comprobacion::toca ((agora - juce::RelativeTime::hours (3)).toMilliseconds(), agora)
               && comprobacion::toca ((agora - juce::RelativeTime::hours (25)).toMilliseconds(), agora)
               && comprobacion::toca ((agora + juce::RelativeTime::hours (2)).toMilliseconds(), agora));
        proba ("comprobación: só se avisa do que é novo",
               comprobacion::novidades ({ "Barullo FM 0.5.1", "Barullo Ley 0.2.0" }, { "Barullo FM 0.5.1" }) == juce::StringArray { "Barullo Ley 0.2.0" }
               && comprobacion::novidades ({ "a" }, { "a" }).isEmpty());
    }

    // ------------------------------------------------------------ axustes e informe
    {
        Temporal t;
        Axustes a;
        a.idioma = "es"; a.bandeja = true; a.copiaPresets = true; a.ultimaComprobacion = 1234567; a.avisadas = { "Barullo FM 0.5.1" };
        a.gardar (t.dir.getChildFile ("ax.json"));
        auto b = Axustes::cargar (t.dir.getChildFile ("ax.json"));
        proba ("axustes: idioma, bandeja, copia dos presets e o xa avisado",
               b.idioma == "es" && b.bandeja && b.copiaPresets && b.ultimaComprobacion == 1234567 && b.avisadas == a.avisadas
               && ! Axustes::cargar (t.dir.getChildFile ("non-hai.json")).bandeja);

        const auto u = t.sub ("usuario"), s = t.sub ("sistema");
        Rutas r { Rutas::cartafol (u), { Rutas::cartafol (s, Ubicacion::Clase::sistema) } };
        Escaner e (r);
        auto fm = produto ("Barullo FM", {}, TipoProduto::instrumento);
        fm.ultima = lanzamentoDe ("0.5.1");
        facerBundle (u.getChildFile ("VST3"), "Barullo FM", Formato::vst3, "0.5.1");
        facerBundle (s.getChildFile ("VST3"), "Barullo FM", Formato::vst3, "0.4.0");
        Copias copias (t.dir.getChildFile ("copias"));
        copias.gardar (fm, Formato::vst3, u.getChildFile ("VST3/Barullo FM.vst3"), *Version::ler ("0.5.0"));
        DatosInforme d;
        d.versionXestor = "0.3.0"; d.so = "macOS"; d.rutas = r; d.copias = &copias; d.daws = { "REAPER" };
        d.situacions.push_back ({ fm, e.instalados (fm), { Formato::vst3 } });
        d.firma = [] (const juce::File&) { return juce::String ("ad hoc"); };
        const auto inf = diagnostico::informe (d);
        proba ("informe: versións, rutas, duplicadas, firma, copias e DAW",
               inf.contains ("Barullo Xestor 0.3.0") && inf.contains (U ("Barullo FM — copia vella no sistema · publicada 0.5.1"))
               && inf.contains ("[DUPLICADA]") && inf.contains ("[firma: ad hoc]") && inf.contains (U ("copias para volver atrás: 0.5.0"))
               && inf.contains ("DAW abertos: REAPER") && inf.contains (U ("con conexión")) && ! inf.contains (U ("Ã")) && inf.contains ("Copias duplicadas (usuario e sistema): 2")
               && diagnostico::curta (juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("x/y")) == "~" + juce::File::getSeparatorString() + "x" + juce::File::getSeparatorString() + "y");
    }

    {
        // GitHub non contesta: ao pasar o tempo máximo cancélase todo e vólvese cun erro (sen colgar)
        Temporal t;
        GitHubMudo mudo;
        GitHub gh ("callejodev-dotcom", t.sub ("cache"), mudo);
        Axustes ax;
        ax.cartafolesLocais.clear();
        Descubridor d (&gh, ax);
        d.limiteMs = 400;
        const auto antes = juce::Time::getMillisecondCounter();
        const auto c = d.descubrir (true);
        const auto tardou = juce::Time::getMillisecondCounter() - antes;
        proba ("descubrir: GitHub mudo → tempo esgotado en vez de agardar para sempre",
               c.tempoEsgotado && c.senRede && c.produtos.empty() && tardou < 5000 && mudo.agardando.load() >= 1
               && c.avisos.joinIntoString (" ").contains ("non respondeu a tempo"));
        // e o que se pida despois de cancelar xa non sae á rede
        const int pedidos = mudo.agardando.load();
        bool erro = false;
        try { gh.pedir ("https://api.github.com/x", true); } catch (const ErroGitHub&) { erro = true; }
        proba ("rede: despois de cancelar, os pedidos fallan xa (sen saír)", erro && mudo.agardando.load() == pedidos);
    }

    std::printf ("\n%d probas, %d fallos\n", total, fallos);
    return fallos == 0 ? 0 : 1;
}

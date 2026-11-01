// As probas de Barullo Xestor: o núcleo e a liña de ordes. Traballan en cartafoles temporais e
// cun GitHub de mentira (sen rede): nunca tocan os plugins de verdade nin piden permisos de verdade.
#include "Nucleo/Nucleo.h"
#include "App/CLI.h"
#include <cstdio>

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

    std::printf ("\n%d probas, %d fallos\n", total, fallos);
    return fallos == 0 ? 0 : 1;
}

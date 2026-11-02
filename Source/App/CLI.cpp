#include "CLI.h"
#include <cstdio>

namespace xestor::cli
{
juce::String axuda()
{
    return U (R"(Barullo Xestor — os plugins Barullo de Escoita

Uso (sen xanela):
  --listar                      lista os plugins, o instalado e a última versión
  --instalar <nome>             instala a última versión (nos formatos escollidos)
  --actualizar <nome>           o mesmo, se hai versión nova (e quita as copias vellas do sistema)
  --actualizar-todo             actualiza todo o que estea vello, incompleto ou con copias vellas
  --desinstalar <nome>          leva o instalado ao Lixo

Opcións:
  --dir <cartafol>              en vez dos cartafoles de verdade: <cartafol>/VST3, /Components, /CLAP,
                                /LV2 e /Applications (sen mirar os do sistema)
  --dir-sistema <cartafol>      (con --dir) un cartafol que fai de cartafol do sistema
  --formatos vst3,au,clap,lv2,app   formatos (por defecto os dos axustes: VST3)
  --papeleira <cartafol>        o que se substitúe ou quita vai alí en vez de ao Lixo
  --zip <ficheiro>              instalar desde un .zip (ou .tar.gz) xa descargado (con --instalar)
  --forzar                      instalar aínda que a versión instalada sexa igual ou máis nova
  --incluir-sistema             desinstalar tamén as copias dos cartafoles do sistema (pide permiso)
  --sen-admin                   non pedir nunca o permiso de administrador (o que o precise, queda)
  --simular-admin               (con --dir-sistema) facer coma se o sistema precisase permiso, sen pedilo
  --sen-rede                    só o que haxa na caché
  --refrescar                   preguntar a GitHub aínda que a caché sexa recente
  --json                        a lista en JSON
  --captura <ficheiro.png>      debuxa a xanela nun PNG (sen abrila), cos datos de verdade
      --ancho <px> --alto <px> --escala <x> --completa   (o tamaño; --completa: toda a lista)
      --escena sen-rede|baleiro|traballando|ao-dia|demo|ficha|aberta|captura|sen-capturas|efectos|actualizacions|busca|sobre|notas|daw|agardando|lembranza|xestor|atallos
      --idioma gl|es|en                                      (a interface neste idioma)
  --icona <ficheiro.png>        debuxa a icona da app (1024 px)
  --informe                     o informe para pedir axuda (versións, rutas, copias duplicadas, firma)
  --axuda

O <nome> pode ser «tune», «Barullo Tune», «barullotune», «enxebres»...)");
}

bool eCLI (const juce::StringArray& args)
{
    for (auto& a : args)
        if (a.startsWith ("--")) return true;
    return false;
}

juce::Result ler (const juce::StringArray& args, Opcions& o)
{
    for (int i = 0; i < args.size(); ++i)
    {
        const auto a = args[i];
        auto seguinte = [&] (juce::String& v) -> bool
        {
            if (i + 1 >= args.size()) return false;
            v = args[++i];
            return true;
        };
        auto ruta = [] (const juce::String& s) { return juce::File::getCurrentWorkingDirectory().getChildFile (s); };
        juce::String v;
        if (a == "--listar" || a == "--actualizar-todo" || a == "--informe" || a == "--axuda" || a == "-h" || a == "--help") o.orde = a;
        else if (a == "--instalar" || a == "--actualizar" || a == "--desinstalar")
        {
            o.orde = a;
            if (! seguinte (o.nome)) return juce::Result::fail (a + " precisa un nome");
        }
        else if (a == "--dir" || a == "--dir-sistema" || a == "--papeleira" || a == "--zip" || a == "--captura" || a == "--icona" || a == "--operacions")
        {
            if (! seguinte (v)) return juce::Result::fail (a + " precisa un valor");
            if (a == "--dir") o.dir = ruta (v);
            else if (a == "--dir-sistema") o.dirSistema = ruta (v);
            else if (a == "--papeleira") o.papeleira = ruta (v);
            else if (a == "--zip") o.arquivo = ruta (v);
            else if (a == "--captura") { o.orde = a; o.captura = ruta (v); }
            else if (a == "--icona") { o.orde = a; o.icona = ruta (v); }
            else { o.orde = a; o.operacions = ruta (v); }
        }
        else if (a == "--formatos")
        {
            if (! seguinte (v)) return juce::Result::fail ("--formatos precisa un valor");
            o.formatos = listaFormatos (v);
            if (! o.formatos) return juce::Result::fail (U ("formatos descoñecidos: ") + v);
        }
        else if (a == "--ancho" || a == "--alto" || a == "--escala" || a == "--escena" || a == "--idioma")
        {
            if (! seguinte (v)) return juce::Result::fail (a + " precisa un valor");
            if (a == "--ancho") o.ancho = juce::jlimit (480, 4000, v.getIntValue());
            else if (a == "--alto") o.alto = juce::jlimit (320, 8000, v.getIntValue());
            else if (a == "--escala") o.escala = juce::jlimit (0.5f, 4.0f, v.getFloatValue());
            else if (a == "--idioma")
            {
                if (! idiomaDeClave (v)) return juce::Result::fail (U ("idioma descoñecido: ") + v + " (gl, es, en)");
                o.idioma = v;
            }
            else o.escena = v;
        }
        else if (a == "--forzar") o.forzar = true;
        else if (a == "--incluir-sistema") o.incluirSistema = true;
        else if (a == "--sen-rede") o.senRede = true;
        else if (a == "--refrescar") o.refrescar = true;
        else if (a == "--json") o.json = true;
        else if (a == "--sen-admin") o.senAdmin = true;
        else if (a == "--simular-admin") o.simularAdmin = true;
        else if (a == "--completa") o.completa = true;
        else if (a.startsWith ("-psn_") || a.startsWith ("-NS")) {}   // o que engade macOS ao abrir
        else return juce::Result::fail (U ("non se entende «") + a + U ("» (--axuda)"));
    }
    if (o.dirSistema != juce::File() && o.dir == juce::File()) return juce::Result::fail ("--dir-sistema vai con --dir");
    if (o.simularAdmin && o.dirSistema == juce::File()) return juce::Result::fail ("--simular-admin vai con --dir e --dir-sistema (nunca cos cartafoles de verdade)");
    return juce::Result::ok();
}

static juce::String pad (const juce::String& s, int n) { return s.length() >= n ? s : s + juce::String::repeatedString (" ", n - s.length()); }

static juce::var aJson (const Produto& p, const Situacion& s)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("nome", p.nome);
    o->setProperty ("curto", p.curto);
    o->setProperty ("tipo", clave (p.tipo));
    o->setProperty ("descricion", p.descricion);
    o->setProperty ("repo", p.repo.isNotEmpty() ? juce::var (p.repo) : juce::var());
    o->setProperty ("rutaLocal", p.rutaLocal != juce::File() ? juce::var (p.rutaLocal.getFullPathName()) : juce::var());
    o->setProperty ("ultima", p.ultima && p.ultima->version() ? juce::var (p.ultima->version()->texto()) : juce::var());
    o->setProperty ("estado", clave (s.estado()));
    juce::Array<juce::var> pl, cap, ins;
    for (auto& x : p.plugins) pl.add (x);
    for (auto& x : p.capturas) cap.add (x);
    o->setProperty ("plugins", pl);
    o->setProperty ("capturas", cap);
    for (auto& i : s.instalados)
    {
        auto* e = new juce::DynamicObject();
        e->setProperty ("plugin", i.plugin);
        e->setProperty ("formato", clave (i.formato));
        e->setProperty ("ubicacion", i.ubicacion.etiqueta());
        e->setProperty ("version", i.textoVersion.isNotEmpty() ? juce::var (i.textoVersion) : juce::var());
        e->setProperty ("ruta", i.ficheiro.getFullPathName());
        e->setProperty ("precisaAdmin", i.precisaAdmin());
        ins.add (juce::var (e));
    }
    o->setProperty ("instalados", ins);
    return juce::var (o);
}

int executar (const juce::StringArray& args, Contexto& ctx)
{
    if (! ctx.saida) ctx.saida = [] (const juce::String& t) { std::fputs ((t + "\n").toRawUTF8(), stdout); std::fflush (stdout); };
    if (! ctx.erro) ctx.erro = [] (const juce::String& t) { std::fputs ((t + "\n").toRawUTF8(), stderr); std::fflush (stderr); };
    auto& out = ctx.saida;
    auto& err = ctx.erro;

    Opcions o;
    if (auto r = ler (args, o); r.failed()) { err (r.getErrorMessage()); return 2; }
    if (o.orde.isEmpty() || o.orde == "--axuda" || o.orde == "-h" || o.orde == "--help")
    {
        out (axuda());
        return o.orde.isEmpty() ? 2 : 0;
    }
    // o paso con permiso de administrador en Windows (o xestor lánzase a si mesmo con «runas»)
    if (o.orde == "--operacions")
    {
        auto r = admin::executarOperacions (o.operacions.loadFileAsString(), admin::cartafolesDoSistema(),
                                            juce::File::getSpecialLocation (juce::File::tempDirectory));
        if (r.failed()) err (r.getErrorMessage());
        return r.wasOk() ? 0 : 1;
    }
    if (o.orde == "--captura") return ctx.captura ? ctx.captura (o) : 1;
    if (o.orde == "--icona") return ctx.icona ? ctx.icona (o) : 1;

    auto axustes = Axustes::cargar (ctx.axustes);
    const auto formatos = o.formatos ? *o.formatos : axustes.formatos;
    Rutas rutas = Rutas::reais();
    if (o.dir != juce::File())
    {
        rutas.usuario = Rutas::cartafol (o.dir);
        rutas.sistema.clear();
        if (o.dirSistema != juce::File()) rutas.sistema.push_back (Rutas::cartafol (o.dirSistema, Ubicacion::Clase::sistema));
    }
    std::unique_ptr<Papeleira> papeleira;
    if (o.papeleira != juce::File()) papeleira = std::make_unique<CartafolLixo> (o.papeleira);
    else papeleira = std::make_unique<LixoDoSistema>();

    TransporteHttp http;
    Transporte& t = ctx.transporte != nullptr ? *ctx.transporte : http;
    GitHub gh (axustes.propietario, ctx.cache, t, (ctx.usarToken && axustes.usarGh && ! o.senRede) ? GitHub::tokenDisponible() : juce::String());
    gh.soCache = o.senRede;
    auto catalogo = Descubridor (&gh, axustes).descubrir (o.refrescar);
    for (auto& a : catalogo.avisos) err ("aviso: " + a);

    Rexistro rexistro (ctx.rexistro);
    Escaner escaner (rutas, &rexistro);
    ElevadorSistema elevSistema;
    ElevadorSenPermiso elevSimulado;
    ElevadorNon elevNon;
    Elevador* elev = ctx.elevador != nullptr ? ctx.elevador : &elevSistema;
    if (o.simularAdmin) elev = &elevSimulado;
    if (o.senAdmin) elev = &elevNon;
    Instalador inst (rutas, *papeleira, elev, t, ctx.temporal, &rexistro);
    inst.simularAdmin = o.simularAdmin;
    // a versión de antes, gardada para volver a ela (só cos cartafoles de verdade)
    Copias copias (Copias::estandar());
    if (o.dir == juce::File()) inst.copias = &copias;

    auto informar = [&] (const Informe& inf)
    {
        for (auto& a : inf.accions) out ("  " + a.texto());
        if (inf.conAdmin > 0) out ("  (" + juce::String (inf.conAdmin) + U (" co permiso de administrador)"));
        return inf.ok() ? 0 : 1;
    };
    auto instalar = [&] (const Produto& p, bool forzar) -> int
    {
        try
        {
            if (o.arquivo != juce::File())
            {
                out (p.nome + ": instalando desde " + o.arquivo.getFullPathName());
                return informar (inst.instalarArquivo (o.arquivo, p, formatos, std::nullopt, forzar));
            }
            const auto* arq = p.ultima ? p.ultima->arquivo() : nullptr;
            out (p.nome + ": descargando " + (arq ? arq->nome + " (" + tamanoLexible (arq->tamano) + ")" : juce::String ("?")));
            int ultimo = -1;
            auto inf = inst.instalar (p, formatos, forzar, [&] (double v)
            {
                const int pc = (int) (v * 10);
                if (pc != ultimo) { ultimo = pc; err ("  " + juce::String (pc * 10) + " %"); }
                return true;
            });
            return informar (inf);
        }
        catch (const ErroInstalacion& e) { err (p.nome + ": " + e.texto()); return 1; }
    };

    if (o.orde == "--informe")
    {
        DatosInforme d;
        d.versionXestor = JUCE_APPLICATION_VERSION_STRING;
        d.so = sistema::descricionSO();
        d.rutas = rutas;
        d.avisos = catalogo.avisos;
        d.senRede = catalogo.senRede || gh.usouCacheVella();
        d.restantesAPI = gh.restantes();
        d.dataCatalogo = catalogo.data;
        d.daws = sistema::dawsAbertos();
        for (auto& p : catalogo.produtos) d.situacions.push_back ({ p, escaner.instalados (p), formatos });
        d.orfos = escaner.orfos (catalogo.produtos);
        if (o.dir == juce::File()) d.copias = &copias;
        if (soActual() == SO::mac) d.firma = [] (const juce::File& f) { return sistema::estadoFirma (f); };
        out (diagnostico::informe (d));
        return 0;
    }

    if (o.orde == "--listar")
    {
        const auto orfos = escaner.orfos (catalogo.produtos);
        if (o.json)
        {
            auto* raiz = new juce::DynamicObject();
            juce::Array<juce::var> fs, ps, os;
            for (auto f : formatos) fs.add (clave (f));
            for (auto& p : catalogo.produtos) ps.add (aJson (p, { p, escaner.instalados (p), formatos }));
            for (auto& i : orfos)
            {
                auto* e = new juce::DynamicObject();
                e->setProperty ("plugin", i.plugin);
                e->setProperty ("formato", clave (i.formato));
                e->setProperty ("version", i.textoVersion);
                e->setProperty ("ruta", i.ficheiro.getFullPathName());
                os.add (juce::var (e));
            }
            raiz->setProperty ("formatos", fs);
            raiz->setProperty ("produtos", ps);
            raiz->setProperty ("orfos", os);
            out (juce::JSON::toString (juce::var (raiz)));
            return 0;
        }
        juce::StringArray fe;
        for (auto f : formatos) fe.add (etiqueta (f));
        out ("Formatos: " + fe.joinIntoString (", ") + U ("   ·   cartafol: ") + rutas.usuario.cartafol (Formato::vst3).getParentDirectory().getFullPathName()
             + (rutas.sistema.empty() ? juce::String() : U (" e os do sistema")));
        std::optional<TipoProduto> tipo;
        for (auto& p : catalogo.produtos)
        {
            if (tipo != p.tipo) { tipo = p.tipo; out ("\n" + titulo (p.tipo).toUpperCase()); }
            Situacion s { p, escaner.instalados (p), formatos };
            const auto ultima = p.ultima && p.ultima->version() ? "v" + p.ultima->version()->texto()
                                                                  : (p.versionLocal ? "local " + p.versionLocal->texto() : U ("—"));
            out ("  " + pad (p.nome, 20) + " " + pad (ultima, 12) + " " + pad (etiqueta (s.estado()), 24) + " " + p.descricion);
            std::map<juce::String, std::vector<Instalado>> grupos;
            for (auto& i : s.relevantes()) grupos[etiqueta (i.formato) + " " + i.ubicacion.etiqueta()].push_back (i);
            for (auto& [k, v] : grupos)
            {
                juce::StringArray vers;
                bool admin = false;
                for (auto& i : v) { vers.addIfNotAlreadyThere (i.textoVersion.isNotEmpty() ? i.textoVersion : "?"); admin = admin || i.precisaAdmin(); }
                vers.sort (true);
                out ("      " + k + ": " + vers.joinIntoString ("/") + (p.eSerie() ? " (" + juce::String ((int) v.size()) + "/" + juce::String (p.plugins.size()) + ")" : juce::String())
                     + (admin ? U (" · precisa admin") : juce::String()));
            }
            if (! s.faltan().isEmpty()) out ("      faltan: " + s.faltan().joinIntoString (", "));
            if (! p.capturas.isEmpty()) out ("      capturas: " + juce::String (p.capturas.size()));
        }
        if (! orfos.empty())
        {
            out (U ("\nOUTROS DE ESCOITA INSTALADOS (sen repo coñecido)"));
            for (auto& i : orfos)
                out ("  " + pad (i.plugin, 20) + " " + pad (i.textoVersion.isNotEmpty() ? i.textoVersion : "?", 12) + " " + etiqueta (i.formato) + " " + i.ubicacion.etiqueta());
        }
        if (gh.restantes() >= 0) out ("\nGitHub: quedan " + juce::String (gh.restantes()) + " preguntas nesta hora.");
        return 0;
    }

    if (o.orde == "--instalar" || o.orde == "--actualizar")
    {
        const auto* p = catalogo.buscar (o.nome);
        if (p == nullptr)
        {
            juce::StringArray ns;
            for (auto& x : catalogo.produtos) ns.add (x.nome);
            err (U ("Non se atopa «") + o.nome + U ("». Hai: ") + ns.joinIntoString (", "));
            return 1;
        }
        Situacion s { *p, escaner.instalados (*p), formatos };
        const auto e = s.estado();
        // só copias vellas no sistema: non hai nada que descargar, só quitalas
        if (e == Estado::copiaVella && ! o.forzar && o.arquivo == juce::File())
        {
            out (p->nome + U (": a copia do cartafol do sistema é vella; ao Lixo."));
            return informar (inst.botar (s.vellasDoSistema()));
        }
        const bool nada = o.orde == "--actualizar" ? ! pideActualizar (e) : e == Estado::alDia;
        if (nada && ! o.forzar && o.arquivo == juce::File())
        {
            out (p->nome + ": " + etiqueta (e) + U ("; nada que facer (--forzar para instalalo igual)."));
            return 0;
        }
        return instalar (*p, o.forzar);
    }

    if (o.orde == "--actualizar-todo")
    {
        int codigo = 0;
        bool algo = false;
        for (auto& p : catalogo.produtos)
        {
            Situacion s { p, escaner.instalados (p), formatos };
            const auto e = s.estado();
            if (! pideActualizar (e)) continue;
            algo = true;
            const int c = e == Estado::copiaVella ? informar (inst.botar (s.vellasDoSistema())) : instalar (p, false);
            if (c != 0) codigo = 1;
        }
        if (! algo) out (U ("Todo ao día."));
        return codigo;
    }

    if (o.orde == "--desinstalar")
    {
        std::optional<Produto> p;
        if (auto* x = catalogo.buscar (o.nome)) p = *x;
        else p = escaner.produtoSolto (o.nome);
        if (! p) { err (U ("Non se atopa «") + o.nome + U ("»")); return 1; }
        return informar (inst.desinstalar (*p, formatos, o.incluirSistema));
    }

    out (axuda());
    return 2;
}
} // namespace xestor::cli

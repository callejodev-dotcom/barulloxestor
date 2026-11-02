#include "Modelo.h"

using namespace xestor;

static juce::Image miniaturaDe (const juce::MemoryBlock& m)
{
    auto im = juce::ImageFileFormat::loadFrom (m.getData(), m.getSize());
    if (! im.isValid()) return {};
    // abonda con 720 px de ancho (as tarxetas son pequenas; o visor carga a enteira)
    const int w = juce::jmin (720, im.getWidth());
    return im.rescaled (w, juce::jmax (1, w * im.getHeight() / im.getWidth()), juce::Graphics::highResamplingQuality);
}

Modelo::Modelo()
{
    axustes = Axustes::cargar();
    porIdioma (idiomaDeClave (axustes.idioma).value_or (idiomaDoSistema()));
    github = std::make_shared<GitHub> (axustes.propietario, GitHub::cacheEstandar(), transporte);
}

Instalador Modelo::instalador (const Rutas& r)
{
    Instalador inst (r, lixo, &elevador, transporte, Instalador::cartafolTemporal(), &rexistro);
    inst.copias = &copias;
    return inst;
}

Modelo::~Modelo()
{
    carga.removeAllJobs (true, 20000);
    fios.removeAllJobs (true, 60000);
    masterReference.clear();
}

void Modelo::noFioPrincipal (std::function<void()> f)
{
    juce::WeakReference<Modelo> w (this);
    juce::MessageManager::callAsync ([wr = w, f]
    {
        if (wr != nullptr) f();
    });
}

const std::vector<Produto>& Modelo::produtos() const
{
    static const std::vector<Produto> baleiro;
    return catalogo ? catalogo->produtos : baleiro;
}

Situacion Modelo::situacion (const Produto& p) const
{
    auto i = situacions.find (p.id());
    return i != situacions.end() ? i->second : Situacion { p, {}, axustes.formatos };
}

std::vector<const Produto*> Modelo::porActualizar() const
{
    std::vector<const Produto*> r;
    for (auto& p : produtos())
        if (pideActualizar (situacion (p).estado())) r.push_back (&p);
    return r;
}

bool Modelo::senRede() const
{
    return catalogo && (catalogo->senRede || github->usouCacheVella());
}

juce::Image Modelo::miniatura (const Produto& p) const
{
    auto i = miniaturas.find (p.id());
    return i != miniaturas.end() ? i->second : juce::Image();
}

juce::Image Modelo::captura (const juce::String& url)
{
    auto i = capturas.find (url);
    if (i != capturas.end()) return i->second;
    auto m = github->imaxe (url);
    auto im = juce::ImageFileFormat::loadFrom (m.getData(), m.getSize());
    capturas[url] = im;
    return im;
}

juce::Image Modelo::capturaSeHai (const juce::String& url) const
{
    auto i = capturas.find (url);
    return i != capturas.end() ? i->second : juce::Image();
}

void Modelo::pedirCaptura (const juce::String& url, std::function<void (juce::Image)> feito)
{
    if (auto i = capturas.find (url); i != capturas.end()) { feito (i->second); return; }
    juce::WeakReference<Modelo> w (this);
    auto gh = github;
    juce::Thread::launch ([w, gh, url, feito]
    {
        const auto m = gh->imaxe (url);
        const auto im = juce::ImageFileFormat::loadFrom (m.getData(), m.getSize());
        juce::MessageManager::callAsync ([wr = w, url, im, f = feito]
        {
            if (wr == nullptr) return;
            wr->capturas[url] = im;
            f (im);
        });
    });
}

// ================================================================ cargar
static juce::String mensaxeDe (const Catalogo& c)
{
    return c.avisos.isEmpty() ? tr ("Comprobado ás %1", c.data.formatted ("%H:%M")) : c.avisos[0];
}

void Modelo::cargarAgora (bool forzar)
{
    const auto token = axustes.usarGh ? GitHub::tokenDisponible() : juce::String();
    github = std::make_shared<GitHub> (axustes.propietario, GitHub::cacheEstandar(), transporte, token);
    catalogo = Descubridor (github.get(), axustes).descubrir (forzar);
    restantes = github->restantes();
    miniaturas.clear();
    for (auto& p : catalogo->produtos)
        if (! p.capturas.isEmpty())
            if (auto im = miniaturaDe (github->imaxe (p.capturas[0])); im.isValid()) miniaturas[p.id()] = im;
    reescanear();
    mensaxe = mensaxeDe (*catalogo);
}

void Modelo::cargar (bool forzar)
{
    if (cargando) return;
    cargando = true;
    mensaxe = tr ("Preguntando a GitHub…");
    avisar();
    juce::WeakReference<Modelo> w (this);
    const auto ax = axustes;
    auto* t = &transporte;
    carga.addJob ([w, ax, forzar, t]
    {
        const auto token = ax.usarGh ? GitHub::tokenDisponible() : juce::String();
        auto gh = std::make_shared<GitHub> (ax.propietario, GitHub::cacheEstandar(), *t, token);
        auto c = std::make_shared<Catalogo> (Descubridor (gh.get(), ax).descubrir (forzar));
        // as miniaturas, xa neste fío (a primeira captura de cada un)
        auto minis = std::make_shared<std::map<juce::String, juce::Image>>();
        for (auto& p : c->produtos)
            if (! p.capturas.isEmpty())
                if (auto im = miniaturaDe (gh->imaxe (p.capturas[0])); im.isValid()) (*minis)[p.id()] = im;
        // o propio xestor: hai versión nova?
        auto xestor = std::make_shared<std::optional<Lanzamento>>();
        try { *xestor = autoactualizacion::nova (gh->ultimoLanzamento (autoactualizacion::repo(), forzar), JUCE_APPLICATION_VERSION_STRING); }
        catch (const ErroGitHub&) {}
        juce::MessageManager::callAsync ([wr = w, gh, c, minis, xestor, forzar]
        {
            if (wr == nullptr) return;
            auto& m = *wr;
            m.github = gh;
            m.catalogo = *c;
            m.miniaturas = *minis;
            m.capturas.clear();
            m.restantes = gh->restantes();
            m.cargando = false;
            m.reescanear();
            m.mensaxe = mensaxeDe (*c);
            m.xestorNovo = *xestor;
            if (forzar && ! m.senRede())
            {
                m.axustes.ultimaComprobacion = juce::Time::currentTimeMillis();
                m.axustes.gardar();
            }
            m.avisar();
        });
    });
}

void Modelo::reescanear()
{
    if (! catalogo) return;
    Escaner e (rutas, &rexistro);
    situacions.clear();
    for (auto& p : catalogo->produtos) situacions[p.id()] = Situacion { p, e.instalados (p), axustes.formatos };
    orfos.clear();
    for (auto& i : e.orfos (catalogo->produtos))
        if (std::find (axustes.formatos.begin(), axustes.formatos.end(), i.formato) != axustes.formatos.end()) orfos.push_back (i);
    avisar();
}

void Modelo::cambiarFormato (Formato f)
{
    auto& l = axustes.formatos;
    auto i = std::find (l.begin(), l.end(), f);
    if (i != l.end()) { if (l.size() > 1) l.erase (i); }
    else l.push_back (f);
    std::vector<Formato> orde;
    for (auto x : todosOsFormatos())
        if (std::find (l.begin(), l.end(), x) != l.end()) orde.push_back (x);
    l = orde;
    axustes.gardar();
    reescanear();
}

// ================================================================ accións
void Modelo::instalar (const Produto& p, bool forzar)
{
    if (traballando (p)) return;
    const auto id = p.id();
    traballos[id] = 0.0;
    fases[id] = "descargando";
    mensaxe = tr ("%1: descargando…", p.nome);
    avisar();
    juce::WeakReference<Modelo> w (this);
    auto prod = p;
    const auto formatos = axustes.formatos;
    const auto r = rutas;
    const bool presetsAntes = axustes.copiaPresets && ! situacion (p).relevantes().empty();
    fios.addJob ([this, w, prod, formatos, r, forzar, id, presetsAntes]
    {
        auto inst = instalador (r);
        // a copia dos presets antes de actualizar (se se pediu); sen presets, nada
        if (presetsAntes)
        {
            juce::File zip;
            presets::copiaDeSeguridade (prod, presets::cartafolCopias(), zip);
        }
        int ultimo = -1;
        Informe inf;
        juce::String erroTxt;
        // copia local: MSVC non deixa que a lambda [&] de dentro colla por referencia o «w»
        // capturado (const) pola lambda de fóra
        auto wLocal = w;
        try
        {
            inf = inst.instalar (prod, formatos, forzar, [&] (double v)
            {
                const int n = (int) (v * 100.0);
                if (n != ultimo)
                {
                    ultimo = n;
                    juce::MessageManager::callAsync ([wr = wLocal, id, v]
                    {
                        if (wr == nullptr || wr->traballos.count (id) == 0) return;
                        wr->traballos[id] = v;
                        if (v >= 1.0) wr->fases[id] = "instalando";
                        wr->avisar();
                    });
                }
                return true;
            });
        }
        catch (const ErroInstalacion& e) { erroTxt = e.texto(); }
        juce::MessageManager::callAsync ([wr = w, prod, inf, erroTxt]
        {
            if (wr == nullptr) return;
            if (erroTxt.isNotEmpty())
            {
                wr->traballos.erase (prod.id());
                wr->erro = prod.nome + ": " + erroTxt;
                wr->mensaxe = wr->erro;
                wr->avisar();
                return;
            }
            wr->rematar (prod, inf);
        });
    });
}

void Modelo::desinstalar (const Produto& p)
{
    if (traballando (p)) return;
    traballos[p.id()] = -1.0;
    fases[p.id()] = "ao Lixo";
    avisar();
    juce::WeakReference<Modelo> w (this);
    auto prod = p;
    const auto formatos = axustes.formatos;
    const auto r = rutas;
    fios.addJob ([this, w, prod, formatos, r]
    {
        auto inst = instalador (r);
        auto inf = inst.desinstalar (prod, formatos, true);
        juce::MessageManager::callAsync ([wr = w, prod, inf] { if (wr != nullptr) wr->rematar (prod, inf); });
    });
}

void Modelo::quitarCopiasVellas (const Produto& p)
{
    if (traballando (p)) return;
    const auto vellas = situacion (p).vellasDoSistema();
    traballos[p.id()] = -1.0;
    fases[p.id()] = "pedindo permiso";
    avisar();
    juce::WeakReference<Modelo> w (this);
    auto prod = p;
    const auto r = rutas;
    fios.addJob ([this, w, prod, vellas, r]
    {
        auto inst = instalador (r);
        auto inf = inst.botar (vellas);
        juce::MessageManager::callAsync ([wr = w, prod, inf] { if (wr != nullptr) wr->rematar (prod, inf); });
    });
}

void Modelo::accionPrincipal (const Produto& p)
{
    switch (situacion (p).estado())
    {
        case Estado::copiaVella:    quitarCopiasVellas (p); break;
        case Estado::actualizacion:
        case Estado::incompleto:
        case Estado::nonInstalado:  instalar (p); break;
        case Estado::soLocal:       if (p.rutaLocal != juce::File()) p.rutaLocal.revealToUser(); break;
        case Estado::senDatos:      cargar (true); break;
        case Estado::alDia:         break;
    }
}

void Modelo::actualizarTodo()
{
    for (auto* p : porActualizar()) accionPrincipal (*p);
}

void Modelo::desinstalarOrfo (const Instalado& i)
{
    juce::WeakReference<Modelo> w (this);
    const auto r = rutas;
    fios.addJob ([this, w, i, r]
    {
        auto inst = instalador (r);
        auto inf = inst.botar ({ i });
        juce::MessageManager::callAsync ([wr = w, inf, i]
        {
            if (wr == nullptr) return;
            juce::StringArray t;
            for (auto& a : inf.accions) t.add (a.texto());
            wr->mensaxe = t.joinIntoString (U (" · "));
            if (! inf.ok()) wr->erro = i.plugin + ": " + wr->mensaxe;
            wr->reescanear();
        });
    });
}

void Modelo::rematar (const Produto& p, const Informe& inf)
{
    traballos.erase (p.id());
    fases.erase (p.id());
    juce::StringArray erros;
    int feitos = 0, asinados = 0;
    bool algoVello = false;
    for (auto& a : inf.accions)
    {
        if (a.tipo == Accion::Tipo::erro) erros.add (a.texto());
        if (a.tipo == Accion::Tipo::instalado || a.tipo == Accion::Tipo::actualizado || a.tipo == Accion::Tipo::aoLixo) ++feitos;
        if (a.tipo == Accion::Tipo::actualizado || a.tipo == Accion::Tipo::aoLixo) algoVello = true;
        if (a.tipo == Accion::Tipo::aviso && a.mensaxe.contains ("ad hoc")) ++asinados;
    }
    if (! erros.isEmpty()) erro = p.nome + ":\n" + erros.joinIntoString ("\n");
    if (inf.cancelouAdmin)
        mensaxe = tr ("%1: sen o permiso de administrador, a copia do sistema queda onde estaba.", p.nome);
    else if (feitos == 0)
    {
        mensaxe = tr ("%1: nada que cambiar", p.nome);
        for (auto& a : inf.accions) if (a.tipo == Accion::Tipo::omitido) { mensaxe = p.nome + ": " + a.mensaxe; break; }
    }
    else
        mensaxe = tr (feitos == 1 ? "%1: 1 cambio" : "%1: %2 cambios", p.nome, juce::String (feitos))
                  + (asinados > 0 ? " (" + tr ("%1 asinado de novo", juce::String (asinados)) + ")" : juce::String())
                  + (algoVello ? tr ("; o vello, no Lixo.") : juce::String ("."));
    if (feitos > 0)
    {
        // o DAW non ve o cambio ata que volve escanear (ou se reinicia)
        const auto daws = sistema::dawsAbertos();
        lembranza = daws.isEmpty() ? tr ("Feito. Lembra volver escanear os plugins no teu DAW (ou reinicialo) para que vexa o cambio.")
                                   : tr ("%1 está aberto: volve escanear os plugins (ou reinícialo) para que vexa o cambio.", daws.joinIntoString (", "));
    }
    reescanear();
}

// ================================================================ escenas (para revisar o deseño)
void Modelo::escena (const juce::String& e)
{
    escenaActual = e;
    if (e == "sen-rede")
    {
        Catalogo c;
        c.senRede = true;
        c.avisos.add (U ("Non se puido preguntar a GitHub: Sen conexión con GitHub"));
        catalogo = c;
        miniaturas.clear();
        reescanear();
        mensaxe = c.avisos[0];
    }
    else if (e == "baleiro")
    {
        catalogo.reset();
        cargando = true;
        mensaxe = tr ("Preguntando a GitHub…");
    }
    else if (e == "ao-dia")
    {
        // coma se todo estivese instalado na última versión
        for (auto& [id, s] : situacions)
        {
            s.instalados.clear();
            if (! s.produto.ultima) continue;
            for (auto& pl : s.produto.plugins)
            {
                Instalado i;
                i.plugin = pl; i.formato = Formato::vst3; i.ubicacion = rutas.usuario;
                i.version = s.produto.ultima->version();
                i.textoVersion = texto (i.version);
                s.instalados.push_back (i);
            }
        }
    }
    else if (e == "traballando")
    {
        // o primeiro descargando (ao 42 %) e o que teña unha copia vella, pedindo permiso
        bool descarga = false;
        for (auto& p : produtos())
        {
            const auto est = situacion (p).estado();
            if (est == Estado::copiaVella) { traballos[p.id()] = -1.0; fases[p.id()] = "pedindo permiso"; }
            else if (! descarga && p.ultima) { traballos[p.id()] = 0.42; fases[p.id()] = "descargando"; descarga = true; mensaxe = tr ("%1: descargando…", p.nome); }
        }
    }
    else if (e == "demo" || e == "ficha")
    {
        // coma na maqueta: tres actualizacións, unha copia vella e o primeiro descargando
        finxirActualizacions();
        if (e == "demo")
            for (auto& p : produtos())
                if (p.ultima && situacion (p).estado() == Estado::alDia)
                {
                    traballos[p.id()] = 0.62;
                    fases[p.id()] = "descargando";
                    mensaxe = tr ("%1: descargando…", p.nome);
                    break;
                }
    }
    else if (e == "sen-capturas")
    {
        // sen ningunha captura: as tarxetas co degradado da súa cor
        miniaturas.clear();
        if (catalogo)
            for (auto& p : catalogo->produtos) p.capturas.clear();
        reescanear();
        finxirActualizacions();
    }
    else if (e == "lembranza")
        lembranza = tr ("%1 está aberto: volve escanear os plugins (ou reinícialo) para que vexa o cambio.", "Ableton Live");
    else if (e == "agardando")
        dawsAgardados = { "Ableton Live", "REAPER" };
    else if (e == "xestor" || e == "xestor-descarga")
    {
        Lanzamento l;
        l.tag = "v9.9.0";
        l.notas = U ("- Exemplo de notas da versión nova do xestor.");
        xestorNovo = l;
        if (e == "xestor-descarga") progresoXestor = 0.37;
    }
}

void Modelo::finxirActualizacions()
{
    const juce::StringArray quen { "Barullo Xuno", "Barullo Xuntos", "Barullo Ley" };
    for (auto& [id, s] : situacions)
    {
        if (! s.produto.ultima || ! s.produto.ultima->version() || s.instalados.empty()) continue;
        auto vella = *s.produto.ultima->version();
        vella.partes.resize (3, 0);
        if (vella.partes[2] > 0) --vella.partes[2];
        else if (vella.partes[1] > 0) { --vella.partes[1]; vella.partes[2] = 9; }
        if (quen.contains (s.produto.nome))
        {
            for (auto& i : s.instalados) { i.version = vella; i.textoVersion = vella.texto(); }
        }
        else if (s.produto.nome == "Barullo FM" && ! rutas.sistema.empty())
        {
            Instalado i = s.instalados.front();
            i.ubicacion = rutas.sistema.front();
            i.ficheiro = i.ubicacion.ruta (i.plugin, i.formato);
            i.version = vella;
            i.textoVersion = vella.texto();
            s.instalados.push_back (i);
        }
    }
}

juce::String Modelo::notasXa (const Produto& p)
{
    if (! p.ultima) return {};
    if (p.ultima->notas.isNotEmpty() || p.repo.isEmpty()) return p.ultima->notas;
    try { return github->notas (p.repo, *p.ultima); }
    catch (...) { return {}; }
}

// ================================================================ volver atrás, notas, presets e manual
std::vector<Version> Modelo::copiasDe (const Produto& p) const
{
    const auto ins = situacion (p).versionInstalada();
    std::vector<Version> r;
    for (auto& v : copias.versions (p))
        if (! ins || v != *ins) r.push_back (v);
    return r;
}

void Modelo::volverA (const Produto& p, const Version& v)
{
    if (traballando (p)) return;
    traballos[p.id()] = -1.0;
    fases[p.id()] = "volvendo";
    mensaxe = tr ("%1: volvendo á %2…", p.nome, v.texto());
    avisar();
    juce::WeakReference<Modelo> w (this);
    auto prod = p;
    const auto formatos = axustes.formatos;
    const auto r = rutas;
    fios.addJob ([this, w, prod, formatos, r, v]
    {
        auto inst = instalador (r);
        Informe inf;
        juce::String erroTxt;
        try { inf = inst.restaurar (prod, v, formatos); }
        catch (const ErroInstalacion& e) { erroTxt = e.texto(); }
        juce::MessageManager::callAsync ([wr = w, prod, inf, erroTxt]
        {
            if (wr == nullptr) return;
            if (erroTxt.isNotEmpty())
            {
                wr->traballos.erase (prod.id());
                wr->fases.erase (prod.id());
                wr->erro = prod.nome + ": " + erroTxt;
                wr->mensaxe = wr->erro;
                wr->avisar();
                return;
            }
            wr->rematar (prod, inf);
        });
    });
}

void Modelo::pedirNotas (const Produto& p, std::function<void (const juce::String&)> feito)
{
    if (! p.ultima)
    {
        // só local: o NOTAS-VERSION.md do repositorio no disco
        const auto f = p.rutaLocal.getChildFile ("NOTAS-VERSION.md");
        feito (p.rutaLocal != juce::File() && f.existsAsFile() ? f.loadFileAsString().trim() : juce::String());
        return;
    }
    if (p.ultima->notas.isNotEmpty() || p.repo.isEmpty()) { feito (p.ultima->notas); return; }
    juce::WeakReference<Modelo> w (this);
    auto gh = github;
    const auto repo = p.repo;
    const auto l = *p.ultima;
    juce::Thread::launch ([w, gh, repo, l, feito]
    {
        const auto n = gh->notas (repo, l);
        juce::MessageManager::callAsync ([wr = w, n, f = feito] { if (wr != nullptr) f (n); });
    });
}

juce::Array<juce::File> Modelo::cartafolesPresets (const Produto& p) const { return presets::cartafoles (p); }

void Modelo::copiaPresets (const Produto& p)
{
    juce::File zip;
    auto r = presets::copiaDeSeguridade (p, presets::cartafolCopias(), zip);
    if (r.failed()) { mensaxe = r.getErrorMessage(); avisar(); return; }
    mensaxe = tr ("Copia dos presets de %1 gardada: %2", p.nome, zip.getFileName());
    zip.revealToUser();
    avisar();
}

void Modelo::abrirManual (const Produto& p)
{
    if (p.manual.startsWith ("http")) juce::URL (p.manual).launchInDefaultBrowser();
    else if (p.manual.isNotEmpty()) juce::File (p.manual).startAsProcess();
}

// ================================================================ os DAW abertos
void Modelo::agardarDaw (const juce::StringArray& daws, std::function<void()> f)
{
    dawsAgardados = daws;
    pendentes.push_back (std::move (f));
    mensaxe = tr ("Agardando a que se peche %1…", daws.joinIntoString (", "));
    startTimer (1500);
    avisar();
}

void Modelo::deixarDeAgardar()
{
    stopTimer();
    pendentes.clear();
    dawsAgardados.clear();
    mensaxe = tr ("Xa non se agarda polo DAW: non se fixo nada.");
    avisar();
}

void Modelo::timerCallback()
{
    const auto abertos = sistema::dawsAbertos();
    if (! abertos.isEmpty())
    {
        if (abertos != dawsAgardados) { dawsAgardados = abertos; avisar(); }
        return;
    }
    stopTimer();
    dawsAgardados.clear();
    auto feitos = std::move (pendentes);
    pendentes.clear();
    for (auto& f : feitos) f();
    avisar();
}

// ================================================================ o propio xestor
void Modelo::actualizarXestor()
{
    if (! xestorNovo || progresoXestor >= 0.0) return;
    const auto l = *xestorNovo;
    progresoXestor = 0.0;
    mensaxe = tr ("Descargando Barullo Xestor %1…", texto (l.version()));
    avisar();
    juce::WeakReference<Modelo> w (this);
    fios.addJob ([this, w, l]
    {
        const auto so = soActual();
        const auto traballo = Instalador::cartafolTemporal().getChildFile ("xestor-" + juce::Uuid().toString());
        juce::File lista;
        int ultimo = -1;
        auto wLocal = w;   // (MSVC: a lambda de dentro non pode coller o «w» da de fóra)
        auto r = autoactualizacion::preparar (l, transporte, traballo, lista, [&] (double v)
        {
            const int n = (int) (v * 100.0);
            if (n != ultimo)
            {
                ultimo = n;
                juce::MessageManager::callAsync ([wr = wLocal, v] { if (wr != nullptr) { wr->progresoXestor = v; wr->avisar(); } });
            }
            return true;
        });
        juce::StringArray orde;
        if (r.wasOk())
        {
            if (so == SO::win)
            {
                const auto guion = traballo.getChildFile ("actualizar-xestor.cmd");
                guion.replaceWithText (autoactualizacion::guionWindows (autoactualizacion::pidActual(), lista,
                                                                        autoactualizacion::exeTrasInstalar (autoactualizacion::appActual())));
                orde = autoactualizacion::ordeReinicio (so, autoactualizacion::pidActual(), {}, guion);
            }
            else
            {
                const auto destino = autoactualizacion::destino (autoactualizacion::appActual());
                r = autoactualizacion::substituir (lista, destino, lixo);
                traballo.deleteRecursively();
                orde = autoactualizacion::ordeReinicio (so, autoactualizacion::pidActual(), destino);
            }
        }
        else traballo.deleteRecursively();
        juce::MessageManager::callAsync ([wr = w, r, orde]
        {
            if (wr == nullptr) return;
            if (r.failed() || ! autoactualizacion::lanzar (orde))
            {
                wr->progresoXestor = -1.0;
                wr->erro = tr ("Non se puido actualizar Barullo Xestor: %1", r.failed() ? r.getErrorMessage() : tr ("non se puido reiniciar"));
                wr->mensaxe = wr->erro;
                wr->avisar();
                return;
            }
            wr->mensaxe = tr ("Reiniciando Barullo Xestor…");
            wr->avisar();
            if (wr->sair) wr->sair();
        });
    });
}

// ================================================================ axustes e informe
void Modelo::cambiarIdioma (std::optional<Idioma> i)
{
    axustes.idioma = i ? clave (*i) : juce::String();
    axustes.gardar();
    porIdioma (i.value_or (idiomaDoSistema()));
    if (alCambiarIdioma) alCambiarIdioma();
    avisar();
}

void Modelo::cambiarBandeja (bool b)
{
    axustes.bandeja = b;
    axustes.gardar();
    if (alCambiarBandeja) alCambiarBandeja();
    avisar();
}

void Modelo::cambiarCopiaPresets (bool b)
{
    axustes.copiaPresets = b;
    axustes.gardar();
    avisar();
}

juce::StringArray Modelo::chavesPorActualizar() const
{
    juce::StringArray r;
    for (auto* p : porActualizar())
        r.add (p->nome + " " + (p->ultima ? texto (p->ultima->version()) : juce::String()));
    if (xestorNovo) r.add ("Barullo Xestor " + texto (xestorNovo->version()));
    return r;
}

DatosInforme Modelo::datosInforme() const
{
    DatosInforme d;
    d.versionXestor = JUCE_APPLICATION_VERSION_STRING;
    d.so = sistema::descricionSO();
    d.idioma = clave (idioma());
    for (auto& p : produtos()) d.situacions.push_back (situacion (p));
    d.orfos = orfos;
    d.rutas = rutas;
    d.avisos = avisos();
    d.restantesAPI = restantes;
    d.senRede = senRede();
    d.dataCatalogo = dataCatalogo();
    d.copias = &copias;
    return d;
}

void Modelo::copiarInforme()
{
    mensaxe = tr ("Preparando o informe…");
    avisar();
    juce::WeakReference<Modelo> w (this);
    auto d = std::make_shared<DatosInforme> (datosInforme());
    auto cp = std::make_shared<Copias> (copias);   // (o fío pode durar máis ca o modelo)
    d->copias = cp.get();
    juce::Thread::launch ([w, d, cp]
    {
        d->daws = sistema::dawsAbertos();
        if (soActual() == SO::mac) d->firma = [] (const juce::File& f) { return sistema::estadoFirma (f); };
        const auto t = diagnostico::informe (*d);
        juce::MessageManager::callAsync ([wr = w, t]
        {
            if (wr == nullptr) return;
            juce::SystemClipboard::copyTextToClipboard (t);
            wr->mensaxe = tr ("Informe copiado: pégao onde pidas axuda.");
            wr->avisar();
        });
    });
}

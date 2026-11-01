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
    github = std::make_shared<GitHub> (axustes.propietario, GitHub::cacheEstandar(), transporte);
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

// ================================================================ cargar
static juce::String mensaxeDe (const Catalogo& c)
{
    return c.avisos.isEmpty() ? U ("Comprobado ás ") + c.data.formatted ("%H:%M") : c.avisos[0];
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
    mensaxe = U ("Preguntando a GitHub…");
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
        juce::MessageManager::callAsync ([wr = w, gh, c, minis]
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
    mensaxe = p.nome + U (": descargando…");
    avisar();
    juce::WeakReference<Modelo> w (this);
    auto prod = p;
    const auto formatos = axustes.formatos;
    const auto r = rutas;
    fios.addJob ([this, w, prod, formatos, r, forzar, id]
    {
        Instalador inst (r, lixo, &elevador, transporte, Instalador::cartafolTemporal(), &rexistro);
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
        Instalador inst (r, lixo, &elevador, transporte, Instalador::cartafolTemporal(), &rexistro);
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
        Instalador inst (r, lixo, &elevador, transporte, Instalador::cartafolTemporal(), &rexistro);
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
        Instalador inst (r, lixo, &elevador, transporte, Instalador::cartafolTemporal(), &rexistro);
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
        mensaxe = p.nome + U (": sen o permiso de administrador, a copia do sistema queda onde estaba.");
    else if (feitos == 0)
    {
        mensaxe = p.nome + ": nada que cambiar";
        for (auto& a : inf.accions) if (a.tipo == Accion::Tipo::omitido) { mensaxe = p.nome + ": " + a.mensaxe; break; }
    }
    else
        mensaxe = p.nome + ": " + juce::String (feitos) + (feitos == 1 ? " cambio" : " cambios")
                  + (asinados > 0 ? " (" + juce::String (asinados) + U (" asinado de novo)") : juce::String())
                  + (algoVello ? U ("; o vello, no Lixo.") : juce::String ("."));
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
        mensaxe = U ("Preguntando a GitHub…");
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
            else if (! descarga && p.ultima) { traballos[p.id()] = 0.42; fases[p.id()] = "descargando"; descarga = true; mensaxe = p.nome + U (": descargando…"); }
        }
    }
}

#include "Vistas.h"
#include <escoita_nucleo/escoita_nucleo.h>

using namespace aspecto;
using namespace xestor;

namespace vistas
{
namespace
{
// ---------------------------------------------------------------- textos e cores do estado
juce::Colour corEstado (Estado e)
{
    switch (e)
    {
        case Estado::alDia:         return col::verde;
        case Estado::actualizacion:
        case Estado::incompleto:    return col::laranxa;
        case Estado::copiaVella:    return col::ouro;
        case Estado::soLocal:       return col::azul;
        case Estado::nonInstalado:
        case Estado::senDatos:      return col::crema3;
    }
    return col::crema3;
}

bool ledAceso (Estado e) { return e != Estado::nonInstalado && e != Estado::senDatos; }

juce::String textoAccion (Estado e)
{
    switch (e)
    {
        case Estado::nonInstalado:  return tr ("Instalar");
        case Estado::actualizacion: return tr ("Actualizar");
        case Estado::incompleto:    return tr ("Completar");
        case Estado::copiaVella:    return tr ("Quitar a vella");
        case Estado::soLocal:       return tr ("Cartafol");
        case Estado::senDatos:      return tr ("Volver probar");
        case Estado::alDia:         return {};
    }
    return {};
}

juce::String axudaAccion (const Situacion& s)
{
    const auto& p = s.produto;
    switch (s.estado())
    {
        case Estado::nonInstalado:  return tr ("Descarga a %1 de GitHub, comproba a suma e instálaa no teu cartafol de plugins (sen contrasinal).", texto (p.ultima->version()));
        case Estado::actualizacion: return tr ("Instala a %1. A versión de agora gárdase para poder volver a ela e vai ao Lixo (nunca se borra); se hai unha copia vella no cartafol do sistema, pídese o contrasinal para quitala.", texto (p.ultima->version()));
        case Estado::incompleto:    return tr ("Instala os plugins da serie que faltan.");
        case Estado::copiaVella:    return tr ("No teu cartafol xa está a última, pero hai unha copia vella no cartafol do sistema que o DAW tamén ve. Lévaa ao Lixo (o sistema pedirá o contrasinal).");
        case Estado::soLocal:       return tr ("Aínda non está publicado: abre o repositorio no disco.");
        case Estado::senDatos:      return tr ("Non se puido preguntar a GitHub por este plugin.");
        case Estado::alDia:         return {};
    }
    return {};
}

juce::String dataCurta (juce::Time t)
{
    return juce::String (t.getDayOfMonth()) + " " + textos::mesCurto (t.getMonth() + 1) + " " + juce::String (t.getYear());
}

// «Barullo Ambiente» → «AMB», «Barullo 7FM» → «7FM»: o rótulo curto dun aparello
juce::String abreviatura (const juce::String& nome)
{
    auto curto = nome.startsWith ("Barullo ") ? nome.substring (8) : nome;
    auto pal = juce::StringArray::fromTokens (curto, " ", "");
    pal.removeEmptyStrings();
    if (pal.isEmpty()) return "?";
    if (pal.size() > 1) return (pal[0].substring (0, 1) + pal[1].substring (0, 2)).toUpperCase();
    return pal[0].substring (0, 3).toUpperCase();
}

void textoUnhaLinha (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j = juce::Justification::centredLeft)
{
    g.drawFittedText (t, r.toNearestInt(), j, 1, 1.0f);
}

// a copia «onde e en que versión», curto: «VST3 0.5.0 · sistema 0.4.0»
juce::String resumoCopias (const Situacion& s)
{
    const auto rel = s.relevantes();
    if (rel.empty()) return {};
    std::map<juce::String, juce::StringArray> g;
    juce::StringArray orde;
    for (auto& i : rel)
    {
        const auto k = etiqueta (i.formato) + (i.ubicacion.eSistema() ? " (" + tr ("sistema") + ")" : juce::String());
        if (! g.count (k)) orde.add (k);
        g[k].addIfNotAlreadyThere (i.textoVersion.isNotEmpty() ? i.textoVersion : "?");
    }
    juce::StringArray partes;
    for (auto& k : orde)
    {
        auto& v = g[k];
        v.sort (true);
        partes.add (k + " " + v.joinIntoString ("/"));
    }
    return partes.joinIntoString (U ("  ·  "));
}
// o que se fai coa tecla principal (e co menú) pasa antes polo aviso do DAW aberto
bool cambiaPlugins (Estado e)
{
    return e == Estado::nonInstalado || e == Estado::actualizacion || e == Estado::incompleto || e == Estado::copiaVella;
}
} // namespace

// ==================================================================== tarxeta
Tarxeta::Tarxeta (Modelo& m, std::function<void (const Produto&, int)> av, std::function<void()> rf)
    : modelo (m), abrirVisor (std::move (av)), refacer (std::move (rf))
{
    accion.getProperties().set ("primaria", true);
    accion.onClick = [this]
    {
        if (! cambiaPlugins (modelo.situacion (produto).estado())) { modelo.accionPrincipal (produto); return; }
        auto* mo = &modelo;
        const auto p = produto;
        paneis::protexer (modelo, this, [mo, p] { mo->accionPrincipal (p); });
    };
    mais.getProperties().set ("icona", "mais");
    mais.setTitle (tr ("Máis"));
    mais.setTooltip (tr ("Notas da versión, volver atrás, manual, presets, reinstalar, desinstalar…"));
    mais.onClick = [this] { menu(); };
    abrir.getProperties().set ("noLed", true);
    abrir.getProperties().set ("fontSize", 8.5f);
    abrir.onClick = [this] { aberta = ! aberta; refacer(); };
    for (auto* b : { &accion, &mais, &abrir }) addAndMakeVisible (b);
    setWantsKeyboardFocus (false);
}

void Tarxeta::mostrar (const Produto& p)
{
    produto = p;
    if (modelo.escenaActual == "aberta" && p.eSerie()) aberta = true;
    const auto s = modelo.situacion (p);
    const auto e = s.estado();
    const auto luz = acento (p.nome);
    accion.getProperties().set ("led", (juce::int64) luz.getARGB());
    mais.getProperties().set ("led", (juce::int64) luz.getARGB());
    abrir.getProperties().set ("led", (juce::int64) luz.getARGB());
    accion.setButtonText (textoAccion (e));
    accion.setTooltip (axudaAccion (s));
    accion.setVisible (! textoAccion (e).isEmpty() && ! modelo.traballando (p));
    accion.setEnabled (! modelo.traballando (p));
    abrir.setVisible (p.eSerie());
    abrir.setButtonText (aberta ? tr ("Pechar") : tr ("Ver os %1", juce::String (p.plugins.size())));
    abrir.setTooltip (tr ("Os plugins da serie, un a un."));
    mais.setDescription (tr ("Máis opcións de %1", p.nome));
    setTitle (p.nome);
    setDescription (p.descricion + ". " + tr (etiqueta (e)));
    repaint();
}

int Tarxeta::alto (int) const
{
    if (! (produto.eSerie() && aberta)) return altoBase;
    const int filas = (produto.plugins.size() + 2) / 3;
    return altoBase + filas * altoFila + 22;
}

juce::Rectangle<int> Tarxeta::zonaMiniatura() const
{
    const int h = altoBase - 28;
    return { 14, 14, juce::roundToInt ((float) h * 1.45f), h };
}

void Tarxeta::resized()
{
    const int y = altoBase - 14 - 32;
    const int dereita = getWidth() - 14;
    mais.setBounds (dereita - 36, y, 36, 32);
    const int aw = juce::jlimit (110, 150, getWidth() / 4);
    accion.setBounds (mais.getX() - 6 - aw, y, aw, 32);
    abrir.setBounds (getWidth() - 14 - 92, 57, 92, 24);   // na liña dos plugins da serie
}

void Tarxeta::paintSerie (juce::Graphics& g, juce::Rectangle<float> r)
{
    // unha serie sen captura: unha reixa coas iniciais de cada plugin, cada unha na súa cor
    paintGlass (g, r, 10.0f);
    const int n = produto.plugins.size();
    const int cols = n <= 4 ? 2 : 3, filas = (n + cols - 1) / cols;
    auto in = r.reduced (12.0f, 10.0f);
    const float cw = in.getWidth() / (float) cols, ch = in.getHeight() / (float) filas;
    for (int i = 0; i < n; ++i)
    {
        auto c = juce::Rectangle<float> (in.getX() + cw * (float) (i % cols), in.getY() + ch * (float) (i / cols), cw, ch).reduced (3.0f, 2.0f);
        const auto a = acento (produto.plugins[i]);
        g.setColour (a.withAlpha (0.10f));
        g.fillRoundedRectangle (c, 4.0f);
        g.setFont (caps (juce::jmin (10.5f, ch * 0.42f), 0.12f));
        glowText (g, abreviatura (produto.plugins[i]), c, juce::Justification::centred, a);
    }
}

void Tarxeta::paintMiniatura (juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto luz = acento (produto.nome);
    auto im = modelo.miniatura (produto);
    if (! im.isValid())
    {
        if (produto.eSerie()) { paintSerie (g, r); return; }
        // sen captura: un visor coa inicial na cor do plugin
        paintGlass (g, r, 10.0f);
        g.setGradientFill (juce::ColourGradient (luz.withAlpha (0.20f), r.getCentreX(), r.getCentreY(), luz.withAlpha (0.0f),
                                                 r.getCentreX() + r.getWidth() * 0.5f, r.getCentreY(), true));
        g.fillRoundedRectangle (r, 10.0f);
        // o nome curto, coma o rótulo do aparello, e debaixo unha liña de LED apagados
        const auto nome = upper (produto.nomeCurto());
        float tam = 22.0f;
        while (tam > 11.0f && anchoTexto (cond (tam).withExtraKerningFactor (0.08f), nome) > r.getWidth() - 28.0f) tam -= 1.0f;
        g.setFont (cond (tam).withExtraKerningFactor (0.08f));
        glowText (g, nome, r.withTrimmedBottom (14.0f), juce::Justification::centred, luz);
        const float ly = r.getCentreY() + tam * 0.5f + 8.0f;
        for (int i = 0; i < 5; ++i) paintLed (g, { r.getCentreX() + (float) (i - 2) * 12.0f, ly }, i == 0, luz);
        return;
    }
    // a captura, recortada para encher (centrada, desde arriba), nun cristal coma as pantallas
    juce::Path p;
    p.addRoundedRectangle (r, 10.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 10.0f);
    {
        juce::Graphics::ScopedSaveState gs (g);
        g.reduceClipRegion (p);
        g.drawImage (im, r, juce::RectanglePlacement (juce::RectanglePlacement::xMid | juce::RectanglePlacement::yTop | juce::RectanglePlacement::fillDestination));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), r.getX(), r.getY(),
                                                 juce::Colours::transparentWhite, r.getX() + r.getWidth() * 0.4f, r.getY() + r.getHeight() * 0.6f, false));
        g.fillRect (r);
        if (sobreMiniatura)
        {
            g.setColour (col::noite.withAlpha (0.45f));
            g.fillRect (r);
            g.setColour (col::crema);
            drawCaps (g, tr ("Ampliar"), r, juce::Justification::centred, 9.0f, 0.3f);
        }
    }
    g.setColour (juce::Colours::white.withAlpha (sobreMiniatura ? 0.30f : 0.12f));
    g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
    if (produto.capturas.size() > 1)
    {
        // cantas capturas hai: unhas pastillas abaixo á dereita
        const int n = juce::jmin (6, produto.capturas.size());
        for (int i = 0; i < n; ++i)
        {
            const float x = r.getRight() - 10.0f - (float) (n - i) * 7.0f;
            g.setColour (i == 0 ? col::crema : col::crema.withAlpha (0.45f));
            g.fillEllipse (x, r.getBottom() - 10.0f, 4.0f, 4.0f);
        }
    }
}

void Tarxeta::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    const auto s = modelo.situacion (produto);
    const auto e = s.estado();
    const auto luz = acento (produto.nome);
    paintPanel (g, r.reduced (2.0f, 1.0f).withTrimmedBottom (4.0f), 14.0f);

    const auto mini = zonaMiniatura().toFloat();
    paintMiniatura (g, mini);

    const float x = mini.getRight() + 16.0f, dereita = (float) getWidth() - 16.0f;
    // 1) o nome (e o tipo) e, á dereita, o estado cun LED
    {
        auto fila = juce::Rectangle<float> (x, 15.0f, dereita - x, 22.0f);
        const auto est = upper (tr (etiqueta (e)));
        const auto fe = caps (8.0f, 0.22f);
        const float we = anchoTexto (fe, est) + 16.0f;
        auto zonaEstado = fila.removeFromRight (we);
        paintLed (g, { zonaEstado.getX() + 4.0f, zonaEstado.getCentreY() }, ledAceso (e), corEstado (e));
        g.setColour (ledAceso (e) ? corEstado (e) : col::crema3);
        g.setFont (fe);
        g.drawText (est, zonaEstado.withTrimmedLeft (13.0f), juce::Justification::centredLeft, false);
        g.setColour (col::crema);
        g.setFont (cond (17.0f).withExtraKerningFactor (0.02f));
        textoUnhaLinha (g, produto.nome, fila.withTrimmedRight (8.0f));
    }
    // 2) a descrición
    g.setColour (col::crema2);
    g.setFont (body (13.0f));
    textoUnhaLinha (g, produto.descricion.isNotEmpty() ? produto.descricion : juce::String (U ("—")), { x, 40.0f, dereita - x, 18.0f });

    // 3) o que hai instalado (ou o tipo e a data se non hai nada)
    {
        juce::String t = resumoCopias (s);
        bool aviso = false;
        if (produto.eSerie())
        {
            const int con = (int) produto.plugins.size() - s.faltan().size();
            t = tr ("%1 plugins dun mando", juce::String (produto.plugins.size()))
                + (s.relevantes().empty() ? juce::String() : U ("  ·  ") + tr ("%1 instalados", juce::String (con) + "/" + juce::String (produto.plugins.size())));
        }
        if (e == Estado::copiaVella || ! s.vellasDoSistema().empty())
        {
            const auto v = s.vellasDoSistema();
            if (! v.empty())
            {
                t = tr ("Copia vella %1 no cartafol do sistema · pedirá o contrasinal", texto (v.front().version));
                aviso = true;
            }
        }
        if (t.isEmpty())
        {
            if (e == Estado::soLocal) t = produto.rutaLocal != juce::File() ? tr ("Só no disco: %1", diagnostico::curta (produto.rutaLocal)) : tr ("Aínda sen publicar");
            else if (produto.ultima && produto.ultima->publicado != juce::Time()) t = tr ("Publicado o %1", dataCurta (produto.ultima->publicado));
        }
        g.setColour (aviso ? col::ouro.withAlpha (0.85f) : col::crema3);
        g.setFont (body (11.5f));
        textoUnhaLinha (g, t, { x, 61.0f, (abrir.isVisible() ? (float) abrir.getX() - 8.0f : dereita) - x, 16.0f });
    }

    // 4) abaixo: o visor da versión (ou o progreso) e as teclas
    const float y = (float) (altoBase - 14 - 32);
    const float finChip = accion.isVisible() ? (float) accion.getX() - 8.0f : (float) mais.getX() - 8.0f;
    auto chip = juce::Rectangle<float> (x, y, juce::jmin (168.0f, finChip - x), 32.0f);
    const bool traballa = modelo.traballando (produto);
    if (traballa)
    {
        // o progreso, no sitio do visor e da tecla: unha barra na cor do plugin
        auto barra = juce::Rectangle<float> (x, y, (float) accion.getRight() - x, 32.0f);
        paintWell (g, barra, 8.0f);
        const double v = modelo.traballos.at (produto.id());
        auto dentro = barra.reduced (4.0f);
        if (v >= 0.0)
        {
            auto feito = dentro.withWidth (juce::jmax (8.0f, dentro.getWidth() * (float) v));
            g.setGradientFill (juce::ColourGradient (luz.withAlpha (0.35f), feito.getX(), 0.0f, luz.withAlpha (0.75f), feito.getRight(), 0.0f, false));
            g.fillRoundedRectangle (feito, 5.0f);
        }
        else
        {
            // sen saber canto queda: un anaco que vai e vén
            const float w = dentro.getWidth() * 0.28f;
            const float t = 0.5f + 0.5f * std::sin ((float) juce::Time::getMillisecondCounter() * 0.004f);
            g.setColour (luz.withAlpha (0.55f));
            g.fillRoundedRectangle (dentro.withWidth (w).withX (dentro.getX() + (dentro.getWidth() - w) * t), 5.0f);
        }
        auto fase = tr (modelo.fases.count (produto.id()) ? modelo.fases.at (produto.id()) : juce::String ("traballando"));
        g.setFont (caps (9.0f, 0.2f));
        glowText (g, upper (fase + (v >= 0.0 && v < 1.0 ? "  " + juce::String (juce::roundToInt (v * 100.0)) + " %" : juce::String (U ("…")))),
                  barra, juce::Justification::centred, col::crema);
    }
    else
    {
        paintWell (g, chip, 8.0f);
        juce::String vt;
        const auto ult = produto.ultima ? produto.ultima->version() : std::nullopt;
        const auto ins = s.versionInstalada();
        if (e == Estado::soLocal) vt = produto.versionLocal ? tr ("local") + " " + produto.versionLocal->texto() : tr ("local");
        else if (e == Estado::actualizacion && ins && ult) vt = ins->texto() + U ("  →  ") + ult->texto();
        else if (ult) vt = "v" + ult->texto();
        else vt = U ("—");
        g.setFont (caps (11.5f, 0.10f));
        glowText (g, vt, chip.reduced (10.0f, 0.0f), juce::Justification::centred, e == Estado::soLocal ? col::azul : col::ouro);
    }

    // 5) a serie aberta: os plugins un a un
    if (produto.eSerie() && aberta)
    {
        auto zona = juce::Rectangle<float> (14.0f, (float) altoBase - 4.0f, (float) getWidth() - 28.0f, (float) getHeight() - altoBase - 8.0f);
        paintWell (g, zona, 10.0f);
        auto in = zona.reduced (14.0f, 8.0f);
        const float cw = in.getWidth() / 3.0f;
        const auto rel = s.relevantes();
        const auto ult = produto.ultima ? produto.ultima->version() : std::nullopt;
        for (int i = 0; i < produto.plugins.size(); ++i)
        {
            auto c = juce::Rectangle<float> (in.getX() + cw * (float) (i % 3), in.getY() + (float) (i / 3) * altoFila, cw - 10.0f, (float) altoFila);
            std::optional<Version> v;
            bool ten = false;
            for (auto& co : rel) if (co.plugin == produto.plugins[i]) { ten = true; if (co.version && (! v || *co.version < *v)) v = co.version; }
            const bool vello = ten && ult && (! v || *v < *ult);
            paintLed (g, { c.getX() + 5.0f, c.getCentreY() }, ten, vello ? col::laranxa : col::verde);
            g.setColour (ten ? col::crema : col::crema3);
            g.setFont (body (12.5f));
            textoUnhaLinha (g, produto.plugins[i].startsWith ("Barullo ") ? produto.plugins[i].substring (8) : produto.plugins[i], c.withTrimmedLeft (16.0f).withTrimmedRight (50.0f));
            g.setFont (caps (8.5f, 0.1f));
            g.setColour (vello ? col::laranxa : col::ouro.withAlpha (0.8f));
            g.drawText (ten ? texto (v) : U ("—"), c.removeFromRight (50.0f), juce::Justification::centredRight, false);
        }
    }
}

// o visor da versión «0.5.0 → 0.5.1»: un clic abre as notas da nova
bool Tarxeta::sobreVersion (juce::Point<int> p) const
{
    const auto x = zonaMiniatura().getRight() + 16, y = altoBase - 14 - 32;
    return produto.ultima && ! modelo.traballando (produto) && juce::Rectangle<int> (x, y, 168, 32).contains (p) && p.x < accion.getX() - 8;
}

void Tarxeta::mouseMove (const juce::MouseEvent& e)
{
    const bool s = zonaMiniatura().contains (e.getPosition()) && ! produto.capturas.isEmpty() && modelo.miniatura (produto).isValid();
    if (s != sobreMiniatura) { sobreMiniatura = s; repaint (zonaMiniatura()); }
    setMouseCursor (s || sobreVersion (e.getPosition()) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void Tarxeta::mouseExit (const juce::MouseEvent&)
{
    if (sobreMiniatura) { sobreMiniatura = false; repaint (zonaMiniatura()); }
}

void Tarxeta::mouseUp (const juce::MouseEvent& e)
{
    if (zonaMiniatura().contains (e.getPosition()) && ! produto.capturas.isEmpty() && abrirVisor) abrirVisor (produto, 0);
    else if (sobreVersion (e.getPosition())) paneis::notas (modelo, this, produto);
}

void Tarxeta::menu()
{
    const auto s = modelo.situacion (produto);
    const auto e = s.estado();
    juce::PopupMenu m;
    m.addSectionHeader (produto.nome);
    const bool hai = ! s.relevantes().empty();
    const bool libre = ! modelo.traballando (produto);
    // o que cambia os plugins pasa antes polo aviso do DAW aberto
    auto* mo = &modelo;
    const auto p = produto;
    juce::Component::SafePointer<Tarxeta> sp (this);
    auto protexido = [mo, sp] (std::function<void()> f) { return [m2 = mo, s2 = sp, f] { paneis::protexer (*m2, s2.getComponent(), f); }; };
    if (produto.ultima)
        m.addItem (tr ("Notas da versión %1", texto (produto.ultima->version())), [this] { paneis::notas (modelo, this, produto); });
    if (produto.ultima)
        m.addItem (tr ("Reinstalar a %1", texto (produto.ultima->version())), hai && libre, false, protexido ([mo, p] { mo->instalar (p, true); }));
    for (auto& v : modelo.copiasDe (produto))
        m.addItem (tr ("Volver á %1 (gardada, sen rede)", v.texto()), hai && libre, false, protexido ([mo, p, v] { mo->volverA (p, v); }));
    if (! s.vellasDoSistema().empty() && e != Estado::copiaVella)
        m.addItem (tr ("Quitar a copia vella do sistema…"), libre, false, protexido ([mo, p] { mo->quitarCopiasVellas (p); }));
    m.addItem (tr ("Desinstalar…"), hai && libre, false, [this] { confirmarDesinstalar(); });
    m.addSeparator();
    if (produto.manual.isNotEmpty())
        m.addItem (tr ("Abrir o manual"), [mo, p] { mo->abrirManual (p); });
    const auto pres = modelo.cartafolesPresets (produto);
    if (! pres.isEmpty())
    {
        m.addItem (pres.size() == 1 ? tr ("Abrir o cartafol dos presets") : tr ("Abrir os cartafoles dos presets"), [pres] { for (auto& f : pres) f.revealToUser(); });
        m.addItem (tr ("Facer unha copia dos presets"), [mo, p] { mo->copiaPresets (p); });
    }
    if (! produto.capturas.isEmpty())
        m.addItem (produto.capturas.size() == 1 ? tr ("Ver a captura") : tr ("Ver as %1 capturas", juce::String (produto.capturas.size())), [this] { abrirVisor (produto, 0); });
    if (hai)
    {
        const auto f = s.relevantes().front().ficheiro;
        m.addItem (tr ("Amosar no cartafol"), [f] { f.revealToUser(); });
    }
    if (produto.repo.isNotEmpty())
        m.addItem (tr ("O repositorio en GitHub"), [u = produto.paxinaRepo()] { juce::URL (u).launchInDefaultBrowser(); });
    if (produto.rutaLocal != juce::File())
        m.addItem (tr ("O repositorio no disco"), [f = produto.rutaLocal] { f.revealToUser(); });
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&mais));
}

void Tarxeta::confirmarDesinstalar()
{
    const auto s = modelo.situacion (produto);
    juce::StringArray copias;
    bool admin = false;
    for (auto& i : s.relevantes())
    {
        copias.add (U ("· ") + i.ficheiro.getFullPathName() + (i.textoVersion.isNotEmpty() ? " (" + i.textoVersion + ")" : juce::String()));
        admin = admin || i.ubicacion.eSistema();
    }
    auto msg = tr ("Vai ao Lixo (de alí pódese recuperar):") + "\n\n" + copias.joinIntoString ("\n");
    if (admin) msg << "\n\n" << tr ("Hai copias no cartafol do sistema: o sistema pedirá o contrasinal de administrador.");
    juce::Component::SafePointer<Tarxeta> sp (this);
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                      .withTitle (tr ("Desinstalar %1", produto.nome))
                                      .withMessage (msg)
                                      .withButton (tr ("Levar ao Lixo"))
                                      .withButton (tr ("Cancelar"))
                                      .withAssociatedComponent (this),
                                  [sp] (int r)
                                  {
                                      if (sp == nullptr || r != 1) return;
                                      auto* m = &sp->modelo;
                                      const auto p = sp->produto;
                                      paneis::protexer (*m, sp.getComponent(), [m, p] { m->desinstalar (p); });
                                  });
}

// ==================================================================== órfos
FilaOrfo::FilaOrfo (Modelo& m, const Instalado& i) : modelo (m), inst (i)
{
    quitar.setButtonText (tr ("Ao Lixo"));
    quitar.getProperties().set ("noLed", true);
    quitar.setTooltip (tr ("Un plugin de Escoita que non está en GitHub (compilado na casa?). Lévao ao Lixo."));
    quitar.setDescription (tr ("Levar %1 ao Lixo", i.plugin));
    quitar.onClick = [this]
    {
        auto* mo = &modelo;
        const auto x = inst;
        paneis::protexer (modelo, this, [mo, x] { mo->desinstalarOrfo (x); });
    };
    addAndMakeVisible (quitar);
}

void FilaOrfo::resized() { quitar.setBounds (getWidth() - 14 - 96, (getHeight() - 28) / 2, 96, 28); }

void FilaOrfo::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f, 2.0f);
    paintWell (g, r, 10.0f);
    paintLed (g, { r.getX() + 16.0f, r.getCentreY() }, true, acento (inst.plugin));
    g.setColour (col::crema);
    g.setFont (semi (13.5f));
    g.drawText (inst.plugin, r.withTrimmedLeft (30.0f).withWidth (220.0f), juce::Justification::centredLeft, false);
    g.setColour (col::crema3);
    g.setFont (body (12.0f));
    g.drawText (etiqueta (inst.formato) + U ("  ·  ") + (inst.textoVersion.isNotEmpty() ? inst.textoVersion : juce::String ("?")) + U ("  ·  ")
                    + inst.ficheiro.getParentDirectory().getFullPathName().replace (juce::File::getSpecialLocation (juce::File::userHomeDirectory).getFullPathName(), "~"),
                r.withTrimmedLeft (260.0f).withTrimmedRight (120.0f), juce::Justification::centredLeft, true);
}

// ==================================================================== lista
Lista::Lista (Modelo& m, std::function<void (const Produto&, int)> av) : modelo (m), abrirVisor (std::move (av))
{
    pecharFaixa.getProperties().set ("icona", "pechar");
    pecharFaixa.setTitle (tr ("Pechar o aviso"));
    pecharFaixa.onClick = [this]
    {
        if (! modelo.dawsAgardados.isEmpty()) { modelo.deixarDeAgardar(); return; }
        if (modelo.erro.isNotEmpty()) modelo.erro.clear();
        else modelo.lembranza.clear();
        actualizar();
    };
    addChildComponent (pecharFaixa);
}

void Lista::actualizar()
{
    const auto& ps = modelo.produtos();
    // reutilízanse as tarxetas (así non se perde «aberta» nin o foco)
    std::vector<std::unique_ptr<Tarxeta>> novas;
    for (auto& p : ps)
    {
        std::unique_ptr<Tarxeta> t;
        for (auto& v : tarxetas)
            if (v != nullptr && v->getProduto().id() == p.id()) { t = std::move (v); break; }
        if (t == nullptr)
        {
            t = std::make_unique<Tarxeta> (modelo, abrirVisor, [this] { colocar (getWidth()); });
            addAndMakeVisible (*t);
        }
        t->mostrar (p);
        novas.push_back (std::move (t));
    }
    tarxetas = std::move (novas);
    orfos.clear();
    for (auto& i : modelo.getOrfos())
    {
        orfos.push_back (std::make_unique<FilaOrfo> (modelo, i));
        addAndMakeVisible (*orfos.back());
    }
    colocar (getWidth());
    repaint();
}

void Lista::colocar (int ancho)
{
    if (ancho <= 0) return;
    const int m = 16, gap = 14;
    const int cols = ancho >= 900 ? 2 : 1;
    const int cw = (ancho - 2 * m - (cols - 1) * gap) / cols;
    int y = 10;
    titulos.clear();

    // o aviso de arriba: un erro (que se pecha), agardando polo DAW, «volve escanear» ou «sen conexión»
    const auto aviso = textoFaixa();
    const bool hai = aviso.isNotEmpty();
    pecharFaixa.setVisible (hai && ! (modelo.erro.isEmpty() && modelo.dawsAgardados.isEmpty() && modelo.lembranza.isEmpty()));
    if (hai)
    {
        const int lin = juce::jlimit (1, 4, juce::StringArray::fromLines (aviso).size());
        faixa = { m, y, ancho - 2 * m, 24 + lin * 18 };
        pecharFaixa.setBounds (faixa.getRight() - 40, faixa.getY() + 6, 32, 28);
        y = faixa.getBottom() + 14;
    }
    else faixa = {};

    std::optional<TipoProduto> tipo;
    std::vector<Tarxeta*> fila;
    auto pecharFila = [&]
    {
        if (fila.empty()) return;
        int h = 0;
        for (auto* t : fila) h = juce::jmax (h, t->alto (cw));
        int x = m;
        for (auto* t : fila) { t->setBounds (x, y, cw, t->alto (cw)); x += cw + gap; }
        y += h + gap;
        fila.clear();
    };
    for (auto& t : tarxetas)
    {
        const auto& p = t->getProduto();
        const auto s = modelo.situacion (p);
        // os que aínda non están publicados van ao final, xuntos
        const auto tp = s.estado() == Estado::soLocal ? TipoProduto::desconecido : p.tipo;
        if (tipo != tp)
        {
            pecharFila();
            tipo = tp;
            int n = 0;
            for (auto& o : tarxetas)
            {
                const auto oe = modelo.situacion (o->getProduto()).estado();
                n += (oe == Estado::soLocal ? TipoProduto::desconecido : o->getProduto().tipo) == tp ? 1 : 0;
            }
            titulos.push_back ({ tp == TipoProduto::desconecido ? tr ("Só neste ordenador (sen publicar)") : tr (titulo (tp)), { m + 4, y + 6, ancho - 2 * m - 8, 16 }, n });
            y += 32;
        }
        fila.push_back (t.get());
        if ((int) fila.size() == cols || t->alto (cw) > 136) pecharFila();
    }
    pecharFila();
    if (! orfos.empty())
    {
        titulos.push_back ({ tr ("Outros de Escoita neste ordenador"), { m + 4, y + 6, ancho - 2 * m - 8, 16 }, (int) orfos.size() });
        y += 32;
        for (auto& o : orfos) { o->setBounds (m, y, ancho - 2 * m, 46); y += 52; }
    }
    setSize (ancho, y + 8);
}

juce::String Lista::textoFaixa() const
{
    if (modelo.erro.isNotEmpty()) return modelo.erro;
    if (! modelo.dawsAgardados.isEmpty())
        return tr ("Agardando a que se peche %1: faise só cando o peches. (Esc ou a × para non agardar)", modelo.dawsAgardados.joinIntoString (", "));
    if (modelo.lembranza.isNotEmpty()) return modelo.lembranza;
    if (modelo.senRede() && ! modelo.produtos().empty())
        return tr ("Sen conexión con GitHub: amósase o último que se soubo. As versións poden non ser as últimas.");
    return {};
}

void Lista::paint (juce::Graphics& g)
{
    for (auto& t : titulos)
    {
        groupTitle (g, t.r.toFloat(), t.texto, col::laranxa);
        const float w = anchoTexto (caps (9.0f, 0.28f), upper (t.texto));
        g.setColour (col::crema3);
        drawCaps (g, juce::String (t.n), t.r.toFloat().withTrimmedLeft (w + 6.0f).withWidth (30.0f).translated (0.0f, 0.0f), juce::Justification::centredLeft, 8.0f, 0.2f);
    }
    if (! faixa.isEmpty())
    {
        auto r = faixa.toFloat();
        paintWell (g, r, 10.0f);
        const bool erro = modelo.erro.isNotEmpty();
        const bool agarda = ! erro && ! modelo.dawsAgardados.isEmpty();
        const bool lembra = ! erro && ! agarda && modelo.lembranza.isNotEmpty();
        const auto c = erro ? col::laranxa : lembra ? col::verde : agarda ? col::azul : col::ouro;
        paintLed (g, { r.getX() + 18.0f, r.getY() + 21.0f }, true, c);
        g.setColour (erro || agarda ? col::crema : col::crema2);
        g.setFont (body (13.0f));
        g.drawFittedText (textoFaixa(), r.withTrimmedLeft (34.0f).withTrimmedRight (pecharFaixa.isVisible() ? 48.0f : 14.0f).reduced (0.0f, 10.0f).toNearestInt(),
                          juce::Justification::topLeft, 4, 1.0f);
    }
}

// ==================================================================== visor
Visor::Visor (Modelo& m) : modelo (m)
{
    anterior.getProperties().set ("icona", "anterior");
    seguinte.getProperties().set ("icona", "seguinte");
    pecharBtn.getProperties().set ("icona", "pechar");
    anterior.setTitle (tr ("Anterior")); seguinte.setTitle (tr ("Seguinte")); pecharBtn.setTitle (tr ("Pechar"));
    anterior.onClick = [this] { ir (-1); };
    seguinte.onClick = [this] { ir (1); };
    pecharBtn.onClick = [this] { pechar(); };
    for (auto* b : { &anterior, &seguinte, &pecharBtn }) addAndMakeVisible (b);
    setWantsKeyboardFocus (true);
    addKeyListener (this);
}

void Visor::abrir (const Produto& p, int i)
{
    if (p.capturas.isEmpty()) return;
    produto = p;
    indice = juce::jlimit (0, p.capturas.size() - 1, i);
    imaxe = modelo.captura (p.capturas[indice]);
    if (! imaxe.isValid()) imaxe = modelo.miniatura (p);
    anterior.setVisible (p.capturas.size() > 1);
    seguinte.setVisible (p.capturas.size() > 1);
    setVisible (true);
    toFront (true);
    if (isShowing()) grabKeyboardFocus();
    resized();
    repaint();
}

void Visor::pechar() { setVisible (false); }

void Visor::ir (int d)
{
    if (produto.capturas.size() < 2) return;
    abrir (produto, (indice + d + produto.capturas.size()) % produto.capturas.size());
}

juce::Rectangle<float> Visor::zonaImaxe() const
{
    auto area = getLocalBounds().toFloat().reduced (76.0f, 0.0f).withTrimmedTop (64.0f).withTrimmedBottom (40.0f);
    if (! imaxe.isValid()) return area;
    return juce::RectanglePlacement (juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize)
        .appliedTo (juce::Rectangle<float> ((float) imaxe.getWidth() / 2.0f, (float) imaxe.getHeight() / 2.0f), area);
}

void Visor::resized()
{
    const int cy = getHeight() / 2 + 12;
    anterior.setBounds (20, cy - 22, 40, 44);
    seguinte.setBounds (getWidth() - 60, cy - 22, 40, 44);
    pecharBtn.setBounds (getWidth() - 56, 14, 38, 34);
}

void Visor::paint (juce::Graphics& g)
{
    g.fillAll (col::noite.withAlpha (0.93f));
    const auto luz = acento (produto.nome);
    auto cab = juce::Rectangle<float> (24.0f, 18.0f, (float) getWidth() - 100.0f, 26.0f);
    paintLed (g, { cab.getX() + 4.0f, cab.getCentreY() }, true, luz);
    g.setColour (col::crema);
    g.setFont (cond (16.0f).withExtraKerningFactor (0.06f));
    g.drawText (upper (produto.nome), cab.withTrimmedLeft (18.0f), juce::Justification::centredLeft, false);
    if (produto.capturas.size() > 1)
    {
        g.setColour (col::crema3);
        drawCaps (g, juce::String (indice + 1) + " / " + juce::String (produto.capturas.size()), cab, juce::Justification::centredRight, 9.0f, 0.3f);
    }
    const auto r = zonaImaxe();
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.translated (0.0f, 6.0f).expanded (2.0f), 14.0f);
    if (imaxe.isValid())
    {
        juce::Path p; p.addRoundedRectangle (r, 12.0f);
        juce::Graphics::ScopedSaveState gs (g);
        g.reduceClipRegion (p);
        g.drawImage (imaxe, r, juce::RectanglePlacement::stretchToFit);
    }
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.0f);
    g.setColour (col::crema3);
    drawCaps (g, tr ("Esc para pechar") + (produto.capturas.size() > 1 ? U ("  ·  ") + tr ("‹ › para pasar") : juce::String()),
              juce::Rectangle<float> (0.0f, (float) getHeight() - 30.0f, (float) getWidth(), 14.0f), juce::Justification::centred, 7.5f, 0.3f);
}

void Visor::mouseUp (const juce::MouseEvent& e)
{
    if (! zonaImaxe().contains (e.position)) pechar();
}

bool Visor::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { pechar(); return true; }
    if (k == juce::KeyPress::leftKey) { ir (-1); return true; }
    if (k == juce::KeyPress::rightKey || k == juce::KeyPress::spaceKey) { ir (1); return true; }
    return false;
}

// ==================================================================== a xanela
Principal::Principal (Modelo& m)
    : modelo (m),
      lista (m, [this] (const Produto& p, int i) { abrirVisor (p, i); }),
      visor (m)
{
    setLookAndFeel (&look);
    setTitle ("Barullo Xestor");
    for (auto f : formatosDe (soActual()))
    {
        auto b = std::make_unique<juce::TextButton> (etiqueta (f));
        b->setClickingTogglesState (false);
        b->setTooltip (tr ("Mirar e instalar %1. Pódense escoller varios; por defecto, só VST3.", etiqueta (f)));
        b->onClick = [this, f] { modelo.cambiarFormato (f); };
        addAndMakeVisible (*b);
        formatoBtn.push_back (std::move (b));
        formatos.push_back (f);
    }
    refrescar.getProperties().set ("icona", "refrescar");
    refrescar.setTitle (tr ("Comprobar agora"));
    refrescar.setTooltip (tr ("Preguntar agora a GitHub polas últimas versións (Cmd/Ctrl + R)."));
    refrescar.onClick = [this] { modelo.cargar (true); };
    actualizarTodo.getProperties().set ("primaria", true);
    actualizarTodo.setTooltip (tr ("Actualizar todo o que ten versión nova (e quitar as copias vellas do sistema). O que se substitúe vai ao Lixo."));
    actualizarTodo.onClick = [this] { paneis::protexer (modelo, this, [mo = &modelo] { mo->actualizarTodo(); }); };
    volverProbar.getProperties().set ("primaria", true);
    volverProbar.setButtonText (tr ("Volver probar"));
    volverProbar.onClick = [this] { modelo.cargar (true); };
    xestorBtn.getProperties().set ("icona", "mais");
    xestorBtn.setTitle (tr ("Barullo Xestor: axustes"));
    xestorBtn.setTooltip (tr ("Idioma, comprobar cada día, copia dos presets, copiar o informe, atallos do teclado…"));
    xestorBtn.onClick = [this] { paneis::menuXestor (modelo, &xestorBtn); };
    addAndMakeVisible (refrescar);
    addAndMakeVisible (actualizarTodo);
    addAndMakeVisible (xestorBtn);
    addChildComponent (volverProbar);
    novaVersion.getProperties().set ("noLed", true);
    novaVersion.onClick = [this] { paneis::xestorNovo (modelo, this); };
    addChildComponent (novaVersion);

    vista.setViewedComponent (&lista, false);
    vista.setScrollBarsShown (true, false);
    vista.setScrollOnDragMode (juce::Viewport::ScrollOnDragMode::never);
    vista.getVerticalScrollBar().setAutoHide (true);
    vista.setSingleStepSizes (0, 40);
    addAndMakeVisible (vista);
    addChildComponent (visor);
    addChildComponent (modal);
    setWantsKeyboardFocus (true);
    // co teclado: Tab percorre a cabeceira e despois as tarxetas, na orde en que se ven
    setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);
    juce::Desktop::getInstance().addFocusChangeListener (this);
    modelo.addChangeListener (this);
    setSize (anchoInicial, altoInicial);
    actualizar();
    startTimerHz (30);
}

Principal::~Principal()
{
    juce::Desktop::getInstance().removeFocusChangeListener (this);
    modelo.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void Principal::changeListenerCallback (juce::ChangeBroadcaster*) { actualizar(); }

bool Principal::baleiro() const { return modelo.produtos().empty(); }

void Principal::actualizar()
{
    for (size_t i = 0; i < formatos.size(); ++i)
    {
        const bool on = std::find (modelo.axustes.formatos.begin(), modelo.axustes.formatos.end(), formatos[i]) != modelo.axustes.formatos.end();
        formatoBtn[i]->setToggleState (on, juce::dontSendNotification);
    }
    const auto n = modelo.porActualizar().size();
    actualizarTodo.setButtonText ((getWidth() < 940 ? tr ("Actualizar") : tr ("Actualizar todo")) + U ("  ·  ") + juce::String ((int) n));
    actualizarTodo.setVisible (n > 0);
    bool algunTraballo = ! modelo.traballos.empty();
    actualizarTodo.setEnabled (! algunTraballo);
    refrescar.setEnabled (! modelo.cargando);
    volverProbar.setVisible (baleiro() && ! modelo.cargando);
    vista.setVisible (! baleiro());
    lista.actualizar();
    resized();
    repaint();
}

void Principal::timerCallback()
{
    // o LED de «comprobando» e as barras sen fin respiran
    pulso += 0.08f;
    if (modelo.cargando) repaint (resumo);
    // a versión nova do propio xestor (e o progreso da descarga)
    {
        juce::String t;
        if (modelo.progresoXestor >= 0.0)
            t = tr ("Descargando Barullo Xestor %1", texto (modelo.xestorNovo ? modelo.xestorNovo->version() : std::nullopt))
                + "  " + juce::String (juce::roundToInt (modelo.progresoXestor * 100.0)) + " %";
        else if (modelo.xestorNovo)
            t = tr ("Hai Barullo Xestor %1 · actualizar", texto (modelo.xestorNovo->version()));
        if (t != novaVersion.getButtonText() || t.isNotEmpty() != novaVersion.isVisible())
        {
            novaVersion.setButtonText (t);
            novaVersion.setVisible (t.isNotEmpty());
            novaVersion.setEnabled (modelo.progresoXestor < 0.0);
            repaint();
        }
    }
    if (! modelo.traballos.empty()) lista.repaint();
}

bool Principal::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress ('r', juce::ModifierKeys::commandModifier, 0) || k == juce::KeyPress::F5Key) { modelo.cargar (true); return true; }
    if (k == juce::KeyPress::escapeKey && modal.isVisible()) { modal.pecharSen(); return true; }
    if (k == juce::KeyPress::escapeKey && visor.isVisible()) { visor.pechar(); return true; }
    if (k == juce::KeyPress::escapeKey && ! modelo.dawsAgardados.isEmpty()) { modelo.deixarDeAgardar(); return true; }
    return false;
}

int Principal::altoCompleto (int ancho)
{
    lista.colocar (ancho);
    return corpo.getY() + lista.getHeight() + 40;
}

void Principal::resized()
{
    const int W = getWidth(), H = getHeight(), m = 16;
    cabeceira = { m, 14, W - 2 * m, 64 };
    placa = { cabeceira.getX() + 10, cabeceira.getY() + 8, 196, cabeceira.getHeight() - 16 };
    const int bh = 34, by = cabeceira.getCentreY() - bh / 2;
    const bool estreita = W < 940;
    actualizarTodo.setButtonText ((estreita ? tr ("Actualizar") : tr ("Actualizar todo")) + U ("  ·  ") + juce::String ((int) modelo.porActualizar().size()));
    int x = cabeceira.getRight() - 12;
    xestorBtn.setBounds (x - 36, by, 36, bh);
    x -= 36 + 8;
    if (actualizarTodo.isVisible())
    {
        const int aw = estreita ? 150 : 196;
        actualizarTodo.setBounds (x - aw, by, aw, bh);
        x -= aw + 10;
    }
    refrescar.setBounds (x - 40, by, 40, bh);
    x -= 40 + 14;
    const int fw = estreita ? 52 : 62;
    const int nf = (int) formatoBtn.size();
    x -= nf * fw + (nf - 1) * 4;
    zonaFormatos = { x - 4, by - 4, nf * fw + (nf - 1) * 4 + 8, bh + 8 };
    for (int i = 0; i < nf; ++i) formatoBtn[(size_t) i]->setBounds (x + i * (fw + 4), by, fw, bh);
    resumo = { placa.getRight() + 14, cabeceira.getY() + 9, zonaFormatos.getX() - 14 - (placa.getRight() + 14), cabeceira.getHeight() - 18 };
    corpo = { 0, cabeceira.getBottom() + 8, W, H - (cabeceira.getBottom() + 8) - 34 };
    vista.setBounds (corpo);
    lista.colocar (corpo.getWidth() - (lista.getHeight() > corpo.getHeight() ? vista.getScrollBarThickness() : 0));
    volverProbar.setBounds (W / 2 - 80, corpo.getCentreY() + 40, 160, 36);
    novaVersion.setBounds (W / 2 - 200, H - 29, 400, 24);
    visor.setBounds (getLocalBounds());
    modal.setBounds (getLocalBounds());
}

void Principal::paintCabeceira (juce::Graphics& g)
{
    paintPanel (g, cabeceira.toFloat(), 14.0f);
    // a placa co nome, coma nos plugins
    {
        const auto p = placa.toFloat();
        paintWell (g, p, 9.0f);
        g.setColour (col::crema);
        g.setFont (cond (15.0f).withExtraKerningFactor (0.07f));
        g.drawText ("BARULLO XESTOR", p.reduced (14.0f, 0.0f).withTrimmedBottom (15.0f).withTrimmedTop (3.0f), juce::Justification::centredLeft, false);
        auto sub = p.reduced (14.0f, 0.0f).withTop (p.getBottom() - 19.0f).withHeight (11.0f);
        const float we = marcaEscoita (g, sub, 7.5f, 0.36f, col::crema2);
        g.setColour (col::crema2);
        drawCaps (g, U (" · ") + tr ("Ferramentas"), sub.withTrimmedLeft (we + 4.0f), juce::Justification::centredLeft, 7.5f, 0.36f);
    }
    // o visor do resumo: que hai que facer, nunha liña, e debaixo os números
    if (resumo.getWidth() > 120)
    {
        const auto r = resumo.toFloat();
        paintGlass (g, r, 10.0f);
        const auto n = (int) modelo.porActualizar().size();
        juce::String grande, pequeno;
        juce::Colour luz = col::verde;
        bool aceso = true;
        if (modelo.cargando && ! modelo.temCatalogo())
        {
            grande = tr ("Comprobando…");
            luz = col::laranxa;
            aceso = std::sin (pulso * 2.0f) > -0.2f;
        }
        else if (baleiro())
        {
            grande = modelo.senRede() ? tr ("Sen conexión") : tr ("Ningún plugin");
            luz = col::ouro;
        }
        else if (! modelo.traballos.empty())
        {
            grande = tr ("Traballando…");
            luz = col::laranxa;
        }
        else if (n > 0)
        {
            int novas = 0, vellas = 0;
            for (auto* p : modelo.porActualizar()) (modelo.situacion (*p).estado() == Estado::copiaVella ? vellas : novas)++;
            juce::StringArray partes;
            if (novas > 0) partes.add (novas == 1 ? tr ("1 versión nova") : tr ("%1 versións novas", juce::String (novas)));
            if (vellas > 0) partes.add (vellas == 1 ? tr ("1 copia vella") : tr ("%1 copias vellas", juce::String (vellas)));
            grande = partes.joinIntoString (U (" · "));
            luz = col::laranxa;
        }
        else grande = tr ("Todo ao día");
        int instalados = 0;
        for (auto& p : modelo.produtos()) instalados += modelo.situacion (p).relevantes().empty() ? 0 : 1;
        juce::StringArray fe;
        for (auto f : modelo.axustes.formatos) fe.add (etiqueta (f));
        pequeno = tr ("%1 de %2 instalados", juce::String (instalados), juce::String ((int) modelo.produtos().size())) + U (" · ") + fe.joinIntoString (" + ");
        if (baleiro()) pequeno = modelo.cargando ? tr ("Os repositorios de %1", modelo.axustes.propietario) : tr ("Volve probar co botón de abaixo");
        else if (modelo.cargando && modelo.temCatalogo()) pequeno = tr ("Comprobando en GitHub…");
        else if (modelo.dataCatalogo() != juce::Time()) pequeno << U (" · ") << tr (modelo.senRede() ? "gardado ás %1" : "ás %1", modelo.dataCatalogo().formatted ("%H:%M"));
        paintLed (g, { r.getX() + 18.0f, r.getCentreY() - 7.0f }, aceso, luz);
        float tam = 12.5f;
        const float sitio = r.getWidth() - 44.0f;
        while (tam > 9.0f && anchoTexto (caps (tam, 0.14f), upper (grande)) > sitio) tam -= 0.5f;
        g.setFont (caps (tam, 0.14f));
        glowText (g, upper (grande), r.withTrimmedLeft (32.0f).withTrimmedRight (10.0f).withHeight (r.getHeight() * 0.56f).translated (0.0f, 3.0f), juce::Justification::centredLeft,
                  luz == col::verde ? col::verde : col::ouro);
        g.setColour (col::crema3);
        g.setFont (caps (7.5f, 0.2f));
        g.drawFittedText (upper (pequeno), r.withTrimmedLeft (32.0f).withTrimmedRight (10.0f).withTrimmedTop (r.getHeight() * 0.56f).withTrimmedBottom (6.0f).toNearestInt(),
                          juce::Justification::centredLeft, 1, 0.8f);
    }
    paintWell (g, zonaFormatos.toFloat(), 11.0f);
}

void Principal::paintBaleiro (juce::Graphics& g)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, juce::jmin (520.0f, (float) getWidth() - 60.0f), modelo.cargando ? 150.0f : 190.0f).withCentre (corpo.getCentre().toFloat().translated (0.0f, -20.0f));
    paintGlass (g, r, 14.0f);
    juce::String t, d;
    juce::Colour c = col::ouro;
    if (modelo.cargando)
    {
        t = tr ("Preguntando a GitHub…");
        d = tr ("Buscando os plugins de Escoita: os repositorios con packaging/produto.env e a súa última versión.");
        c = col::laranxa;
    }
    else if (modelo.senRede())
    {
        t = tr ("Sen conexión");
        d = tr ("Non se puido falar con GitHub e aínda non hai nada gardado. Mira a conexión e volve probar.");
    }
    else
    {
        t = tr ("Ningún plugin");
        d = tr ("Non se atopou ningún plugin de Escoita. Se acabas de publicar un, volve probar nun intre.");
    }
    paintLed (g, { r.getCentreX(), r.getY() + 36.0f }, ! modelo.cargando || std::sin (pulso * 2.0f) > -0.2f, c);
    g.setFont (caps (15.0f, 0.16f));
    glowText (g, upper (t), r.withTrimmedTop (52.0f).withHeight (24.0f), juce::Justification::centred, c);
    g.setColour (col::crema2);
    g.setFont (body (13.5f));
    g.drawFittedText (d, r.reduced (36.0f, 0.0f).withTrimmedTop (86.0f).withHeight (44.0f).toNearestInt(), juce::Justification::centredTop, 2, 1.0f);
}

void Principal::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff161e33), 0.0f, 0.0f, juce::Colour (0xff0b101e), 0.0f, (float) getHeight(), false));
    g.fillAll();
    paintCabeceira (g);
    if (baleiro()) paintBaleiro (g);

    // o pé, coma nos plugins
    const auto r = juce::Rectangle<float> (22.0f, (float) getHeight() - 24.0f, (float) getWidth() - 44.0f, 14.0f);
    const bool ancha = getWidth() >= 940;
    const float we = marcaEscoita (g, r, 8.0f, 0.3f, col::crema3);
    g.setColour (col::crema3);
    drawCaps (g, (ancha ? " " + tr ("Laboratorio creativo") + U (" · v") : U (" · v")) + escoita::Actualizacions::currentVersion(), r.withTrimmedLeft (we + 3.0f), juce::Justification::centredLeft, 8.0f, 0.3f);
    if (ancha) drawCaps (g, U ("42°14′N 8°43′W"), r, juce::Justification::centredRight, 8.0f, 0.3f);
    if (modelo.mensaxe.isNotEmpty() && ! novaVersion.isVisible())
    {
        g.setFont (body (11.5f));
        g.setColour (col::crema3);
        g.drawFittedText (modelo.mensaxe.upToFirstOccurrenceOf ("\n", false, false),
                          (ancha ? r.reduced (290.0f, 0.0f) : r.withTrimmedLeft (150.0f)).toNearestInt(),
                          ancha ? juce::Justification::centred : juce::Justification::centredRight, 1, 0.9f);
    }
}
// ==================================================================== teclado e paneis
void Principal::globalFocusChanged (juce::Component* c)
{
    // ao chegar co Tab a unha tecla dunha tarxeta que non se ve, a lista desprázase ata ela
    if (c == nullptr || ! lista.isParentOf (c)) return;
    const auto r = lista.getLocalArea (c, c->getLocalBounds()).expanded (0, 16);
    const auto v = vista.getViewArea();
    if (r.getY() < v.getY()) vista.setViewPosition (0, r.getY());
    else if (r.getBottom() > v.getBottom()) vista.setViewPosition (0, r.getBottom() - v.getHeight());
}

void Principal::escena (const juce::String& e)
{
    const Produto* p = nullptr;
    for (auto& x : modelo.produtos())
        if (x.ultima && (p == nullptr || modelo.situacion (x).estado() == Estado::actualizacion)) p = &x;
    if (e == "notas" && p != nullptr)
    {
        paneis::notas (modelo, this, *p);
        if (p->ultima->notas.isEmpty())
            modal.porTexto (textos::markdownLexible (U ("## Novidades\n- Exemplo: as notas chegan de GitHub (o texto da versión ou NOTAS-VERSION.md).\n- **Máis** cousas.")));
    }
    else if (e == "daw")
        paneis::avisoDaw (modelo, this, { "Ableton Live" }, [] {});
    else if (e == "xestor")
        paneis::xestorNovo (modelo, this);
    else if (e == "atallos")
        paneis::atallos (this);
}
} // namespace vistas

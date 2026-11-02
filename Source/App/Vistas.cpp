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
        case Estado::nonInstalado:  return col::crema2;
        case Estado::senDatos:      return col::crema3;
    }
    return col::crema3;
}

// o texto da pastilla, curto e en minúsculas coma unha frase
juce::String textoPastilla (Estado e)
{
    switch (e)
    {
        case Estado::alDia:         return tr ("Ao día");
        case Estado::actualizacion: return tr ("Actualización");
        case Estado::incompleto:    return tr ("Incompleto");
        case Estado::copiaVella:    return tr ("Copia vella");
        case Estado::nonInstalado:  return tr ("Sen instalar");
        case Estado::soLocal:       return tr ("Só local");
        case Estado::senDatos:      return tr ("Sen datos");
    }
    return {};
}

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

// a tecla da acción: ámbar para instalar e actualizar, contorno de ouro para a copia vella
void estiloAccion (juce::TextButton& b, Estado e)
{
    auto& pr = b.getProperties();
    pr.remove ("primaria");
    pr.remove ("estilo");
    if (e == Estado::nonInstalado || e == Estado::actualizacion || e == Estado::incompleto) pr.set ("primaria", true);
    else if (e == Estado::copiaVella) { pr.set ("estilo", "contorno"); pr.set ("cor", (juce::int64) col::ouro.getARGB()); }
    else pr.set ("estilo", "fantasma");
}

juce::String dataCurta (juce::Time t)
{
    return juce::String (t.getDayOfMonth()) + " " + textos::mesCurto (t.getMonth() + 1) + " " + juce::String (t.getYear());
}

juce::String maiuscula1 (const juce::String& s) { return s.substring (0, 1).toUpperCase() + s.substring (1); }

// «Xuno, Xuntos e Ley»
juce::String lista (const juce::StringArray& a)
{
    if (a.size() <= 1) return a.joinIntoString ("");
    juce::StringArray primeiros (a);
    primeiros.remove (a.size() - 1);
    return tr ("%1 e %2", primeiros.joinIntoString (", "), a[a.size() - 1]);
}

// «18 MB», «640 KB»
juce::String tamano (juce::int64 b)
{
    if (b >= 1024 * 1024) return juce::String (juce::jmax<juce::int64> (1, (b + 512 * 1024) / (1024 * 1024))) + " MB";
    return juce::String (juce::jmax<juce::int64> (1, b / 1024)) + " KB";
}

// os formatos que hai instalados, na orde de sempre: «VST3 · CLAP»
juce::StringArray formatosInstalados (const Situacion& s)
{
    juce::StringArray r;
    for (auto f : todosOsFormatos())
        for (auto& i : s.relevantes())
            if (i.formato == f) { r.add (etiqueta (f)); break; }
    return r;
}

// o que se fai coa tecla principal (e co menú) pasa antes polo aviso do DAW aberto
bool cambiaPlugins (Estado e)
{
    return e == Estado::nonInstalado || e == Estado::actualizacion || e == Estado::incompleto || e == Estado::copiaVella;
}

void facerAccion (Modelo& modelo, juce::Component* desde, const Produto& p)
{
    if (! cambiaPlugins (modelo.situacion (p).estado())) { modelo.accionPrincipal (p); return; }
    auto* mo = &modelo;
    const auto pr = p;
    paneis::protexer (modelo, desde, [mo, pr] { mo->accionPrincipal (pr); });
}

void confirmarDesinstalar (Modelo& modelo, juce::Component* desde, const Produto& produto)
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
    juce::Component::SafePointer<juce::Component> sp (desde);
    auto* mo = &modelo;
    const auto p = produto;
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                      .withTitle (tr ("Desinstalar %1", produto.nome))
                                      .withMessage (msg)
                                      .withButton (tr ("Levar ao Lixo"))
                                      .withButton (tr ("Cancelar"))
                                      .withAssociatedComponent (desde),
                                  [sp, mo, p] (int r)
                                  {
                                      if (sp == nullptr || r != 1) return;
                                      paneis::protexer (*mo, sp.getComponent(), [m2 = mo, p2 = p] { m2->desinstalar (p2); });
                                  });
}

// O menú ⋯ dun plugin (na tarxeta e na ficha): notas, reinstalar, volver atrás, desinstalar, manual...
void menuProduto (Modelo& modelo, juce::Component* boton, const Produto& produto, std::function<void (const Produto&)> abrirFicha)
{
    const auto s = modelo.situacion (produto);
    const auto e = s.estado();
    juce::PopupMenu m;
    m.addSectionHeader (produto.nome);
    const bool hai = ! s.relevantes().empty();
    const bool libre = ! modelo.traballando (produto);
    auto* mo = &modelo;
    const auto p = produto;
    juce::Component::SafePointer<juce::Component> sp (boton);
    auto protexido = [mo, sp] (std::function<void()> f) { return [m2 = mo, s2 = sp, f] { paneis::protexer (*m2, s2.getComponent(), f); }; };
    if (produto.ultima)
        m.addItem (tr ("Notas da versión %1", texto (produto.ultima->version())), [mo, sp, p] { paneis::notas (*mo, sp.getComponent(), p); });
    if (produto.ultima)
        m.addItem (tr ("Reinstalar a %1", texto (produto.ultima->version())), hai && libre, false, protexido ([mo, p] { mo->instalar (p, true); }));
    for (auto& v : modelo.copiasDe (produto))
        m.addItem (tr ("Volver á %1 (gardada, sen rede)", v.texto()), hai && libre, false, protexido ([mo, p, v] { mo->volverA (p, v); }));
    if (! s.vellasDoSistema().empty() && e != Estado::copiaVella)
        m.addItem (tr ("Quitar a copia vella do sistema…"), libre, false, protexido ([mo, p] { mo->quitarCopiasVellas (p); }));
    m.addItem (tr ("Desinstalar…"), hai && libre, false, [mo, sp, p] { if (sp != nullptr) confirmarDesinstalar (*mo, sp.getComponent(), p); });
    m.addSeparator();
    if (produto.manual.isNotEmpty())
        m.addItem (tr ("Abrir o manual"), [mo, p] { mo->abrirManual (p); });
    const auto pres = modelo.cartafolesPresets (produto);
    if (! pres.isEmpty())
    {
        m.addItem (pres.size() == 1 ? tr ("Abrir o cartafol dos presets") : tr ("Abrir os cartafoles dos presets"), [pres] { for (auto& f : pres) f.revealToUser(); });
        m.addItem (tr ("Facer unha copia dos presets"), [mo, p] { mo->copiaPresets (p); });
    }
    if (! produto.capturas.isEmpty() && abrirFicha)
        m.addItem (produto.capturas.size() == 1 ? tr ("Ver a captura") : tr ("Ver as %1 capturas", juce::String (produto.capturas.size())), [abrirFicha, p] { abrirFicha (p); });
    if (hai)
    {
        const auto f = s.relevantes().front().ficheiro;
        m.addItem (tr ("Amosar no cartafol"), [f] { f.revealToUser(); });
    }
    if (produto.repo.isNotEmpty())
        m.addItem (tr ("O repositorio en GitHub"), [u = produto.paxinaRepo()] { juce::URL (u).launchInDefaultBrowser(); });
    if (produto.rutaLocal != juce::File())
        m.addItem (tr ("O repositorio no disco"), [f = produto.rutaLocal] { f.revealToUser(); });
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (boton));
}

// unha liña de texto en anacos de cores distintas (a versión: «0.1.0 → 0.1.1»)
struct Anaco { juce::String t; juce::Colour c; };
void paintAnacos (juce::Graphics& g, const std::vector<Anaco>& as, juce::Rectangle<float> r, const juce::Font& f)
{
    g.setFont (f);
    float x = r.getX();
    for (auto& a : as)
    {
        const float w = anchoTexto (f, a.t);
        g.setColour (a.c);
        g.drawText (a.t, juce::Rectangle<float> (x, r.getY(), juce::jmax (0.0f, juce::jmin (w + 2.0f, r.getRight() - x)), r.getHeight()),
                    juce::Justification::centredLeft, true);
        x += w;
        if (x > r.getRight()) break;
    }
}

std::vector<Anaco> anacosVersion (Modelo& modelo, const Produto& p)
{
    const auto s = modelo.situacion (p);
    const auto e = s.estado();
    const auto ult = p.ultima ? p.ultima->version() : std::nullopt;
    const auto ins = s.versionInstalada();
    const auto luz = acento (p.nome);
    if (e == Estado::soLocal)
        return { { p.versionLocal ? tr ("local") + " " + p.versionLocal->texto() : tr ("Aínda sen publicar"), col::crema3 } };
    if (e == Estado::actualizacion && ins && ult)
        return { { ins->texto(), col::crema2 }, { U (" → "), col::crema3 }, { ult->texto(), luz } };
    if (! ult) return { { U ("—"), col::crema3 } };
    auto fs = formatosInstalados (s);
    std::vector<Anaco> r { { "v" + (ins ? ins->texto() : ult->texto()), col::crema3 } };
    if (! fs.isEmpty()) r.push_back ({ U (" · ") + fs.joinIntoString (U (" · ")), col::crema3 });
    if (p.eSerie() && ! s.relevantes().empty())
    {
        const int con = (int) p.plugins.size() - s.faltan().size();
        r.push_back ({ U (" · ") + juce::String (con) + "/" + juce::String (p.plugins.size()), col::crema3 });
    }
    return r;
}

// o progreso dun traballo: unha barra na cor do plugin co texto «Descargando · 62 %»
void paintProgreso (juce::Graphics& g, Modelo& modelo, const Produto& p, juce::Rectangle<float> barra)
{
    const auto luz = acento (p.nome);
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.fillRoundedRectangle (barra, 10.0f);
    const double v = modelo.traballos.count (p.id()) ? modelo.traballos.at (p.id()) : -1.0;
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addRoundedRectangle (barra, 10.0f);
        g.reduceClipRegion (clip);
        if (v >= 0.0)
        {
            auto feito = barra.withWidth (juce::jmax (12.0f, barra.getWidth() * (float) v));
            g.setGradientFill (juce::ColourGradient (luz.withAlpha (0.30f), feito.getX(), 0.0f, luz.withAlpha (0.70f), feito.getRight(), 0.0f, false));
            g.fillRect (feito);
        }
        else
        {
            // sen saber canto queda: un anaco que vai e vén
            const float w = barra.getWidth() * 0.32f;
            const float t = 0.5f + 0.5f * std::sin ((float) juce::Time::getMillisecondCounter() * 0.0035f);
            auto an = barra.withWidth (w).withX (barra.getX() + (barra.getWidth() - w) * t);
            g.setGradientFill (juce::ColourGradient (luz.withAlpha (0.0f), an.getX(), 0.0f, luz.withAlpha (0.6f), an.getCentreX(), 0.0f, false));
            g.fillRect (an.withWidth (w * 0.5f));
            g.setGradientFill (juce::ColourGradient (luz.withAlpha (0.6f), an.getCentreX(), 0.0f, luz.withAlpha (0.0f), an.getRight(), 0.0f, false));
            g.fillRect (an.withTrimmedLeft (w * 0.5f));
        }
    }
    const auto fase = maiuscula1 (tr (modelo.fases.count (p.id()) ? modelo.fases.at (p.id()) : juce::String ("traballando")));
    g.setColour (col::crema);
    g.setFont (semi (css (12.0f)));
    g.drawText (fase + (v >= 0.0 && v < 1.0 ? U (" · ") + juce::String (juce::roundToInt (v * 100.0)) + " %" : juce::String (U ("…"))),
                barra.reduced (12.0f, 0.0f), juce::Justification::centredLeft, true);
}

// un degradado da cor do plugin co nome grande (cando non hai captura)
void paintSenImaxe (juce::Graphics& g, const Produto& p, juce::Rectangle<float> r, float tamNome)
{
    const auto luz = acento (p.nome);
    g.setGradientFill (juce::ColourGradient (luz.withMultipliedBrightness (0.55f).withMultipliedSaturation (0.8f), r.getX(), r.getY(),
                                             col::panel2, r.getRight(), r.getBottom(), false));
    g.fillRect (r);
    g.setGradientFill (juce::ColourGradient (luz.withAlpha (0.35f), r.getCentreX(), r.getCentreY(), luz.withAlpha (0.0f),
                                             r.getCentreX() + r.getWidth() * 0.55f, r.getCentreY(), true));
    g.fillRect (r);
    // unhas liñas finas en diagonal, coma o veteado dun panel
    g.setColour (juce::Colours::white.withAlpha (0.035f));
    for (float x = r.getX() - r.getHeight(); x < r.getRight(); x += 14.0f)
        g.drawLine (x, r.getBottom(), x + r.getHeight(), r.getY(), 1.0f);
    const auto nome = p.nomeCurto();
    float tam = tamNome;
    while (tam > 14.0f && anchoTexto (cond (tam), nome) > r.getWidth() - 40.0f) tam -= 1.0f;
    g.setFont (cond (tam));
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawText (nome, r.translated (0.0f, 2.0f), juce::Justification::centred, false);
    g.setColour (col::crema.withAlpha (0.92f));
    g.drawText (nome, r, juce::Justification::centred, false);
    g.setColour (col::crema.withAlpha (0.55f));
    drawCaps (g, "Barullo", r.withTrimmedTop (r.getHeight() * 0.5f + tam * 0.45f).withHeight (14.0f), juce::Justification::centredTop, css (8.5f), 0.4f);
}

float suave (float t) { t = juce::jlimit (0.0f, 1.0f, t); return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); }
int altoCaptura (int ancho) { return juce::jlimit (120, 200, juce::roundToInt ((float) ancho * 0.46f)); }
} // namespace

// ==================================================================== tarxeta
Tarxeta::Tarxeta (Modelo& m, std::function<void (const Produto&)> af)
    : modelo (m), abrirFicha (std::move (af))
{
    accion.onClick = [this] { facerAccion (modelo, this, produto); };
    mais.getProperties().set ("icona", "mais");
    mais.setTitle (tr ("Máis"));
    mais.setTooltip (tr ("Notas da versión, volver atrás, manual, presets, reinstalar, desinstalar…"));
    mais.onClick = [this] { menuProduto (modelo, &mais, produto, abrirFicha); };
    for (auto* b : { &accion, &mais }) addAndMakeVisible (b);
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    accion.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    mais.setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void Tarxeta::mostrar (const Produto& p)
{
    produto = p;
    const auto s = modelo.situacion (p);
    const auto e = s.estado();
    const auto luz = acento (p.nome);
    const bool traballa = modelo.traballando (p);
    if (traballaba && ! traballa && modelo.erro.isEmpty()) visto = 1.0f;   // o ✓ ao rematar
    traballaba = traballa;
    for (auto* b : { &accion, &mais }) b->getProperties().set ("led", (juce::int64) luz.getARGB());
    estiloAccion (accion, e);
    accion.setButtonText (textoAccion (e));
    accion.setTooltip (axudaAccion (s));
    accion.setDescription (textoAccion (e) + " " + p.nome);
    accion.setVisible (textoAccion (e).isNotEmpty() && ! traballa);
    mais.setVisible (! accion.isVisible() && ! traballa);
    mais.setDescription (tr ("Máis opcións de %1", p.nome));
    setTitle (p.nome);
    setDescription (p.descricion + ". " + textoPastilla (e));
    setHelpText (tr ("Intro: abrir a ficha"));
    resized();
    repaint();
}

int Tarxeta::alto (int ancho) { return altoCaptura (ancho) + 88; }

float Tarxeta::alzado() const { return -3.0f * sobre + 14.0f * (1.0f - suave (aparicion)); }

juce::Rectangle<float> Tarxeta::caixa() const
{
    return getLocalBounds().toFloat().reduced ((float) marxe).translated (0.0f, alzado());
}

juce::Rectangle<float> Tarxeta::zonaVersion() const
{
    const auto c = caixa();
    const float fy = c.getY() + (float) altoCaptura ((int) c.getWidth()) - 24.0f + 58.0f;
    const float fin = accion.isVisible() ? (float) accion.getX() - 8.0f : mais.isVisible() ? (float) mais.getX() - 8.0f : c.getRight() - 16.0f;
    return { c.getX() + 16.0f, fy, fin - (c.getX() + 16.0f), 36.0f };
}

void Tarxeta::resized()
{
    const auto c = caixa();
    const int fy = juce::roundToInt (c.getY() + (float) altoCaptura ((int) c.getWidth()) - 24.0f + 58.0f);
    const int dereita = juce::roundToInt (c.getRight()) - 16;
    const int aw = juce::jmin (Look::anchoTecla (accion, 30), (int) c.getWidth() / 2);
    accion.setBounds (dereita - aw, fy, aw, 36);
    mais.setBounds (dereita - 32, fy + 2, 32, 32);
}

bool Tarxeta::hitTest (int x, int y)
{
    return getLocalBounds().reduced (marxe).contains (x, y);
}

void Tarxeta::entrar (float a)
{
    aparicion = 0.0f;
    atraso = a;
    setAlpha (0.0f);
    resized();
}

bool Tarxeta::animar (float dt)
{
    bool cambio = false;
    const float obx = (forzarSobre || (isMouseOver (true) && isEnabled())) ? 1.0f : 0.0f;
    if (std::abs (sobre - obx) > 0.001f)
    {
        sobre += (obx - sobre) * juce::jmin (1.0f, dt * 12.0f);
        if (std::abs (sobre - obx) < 0.003f) sobre = obx;
        cambio = true;
    }
    if (aparicion < 1.0f)
    {
        if (atraso > 0.0f) atraso -= dt;
        else aparicion = juce::jmin (1.0f, aparicion + dt / 0.45f);
        setAlpha (suave (aparicion * 1.4f));
        cambio = true;
    }
    if (visto > 0.0f) { visto = juce::jmax (0.0f, visto - dt / 2.4f); cambio = true; }
    if (cambio) resized();
    if (cambio || modelo.traballando (produto)) repaint();
    return cambio;
}

void Tarxeta::paintSenCaptura (juce::Graphics& g, juce::Rectangle<float> r)
{
    paintSenImaxe (g, produto, r, css (26.0f));
}

void Tarxeta::paint (juce::Graphics& g)
{
    const auto c = caixa();
    const auto s = modelo.situacion (produto);
    const auto e = s.estado();
    const auto luz = acento (produto.nome);
    const float rad = 16.0f;

    // a sombra e, co rato enriba, o brillo da cor do plugin
    paintSombra (g, c, rad, juce::Colours::black.withAlpha (0.35f + 0.10f * sobre), 22.0f + 8.0f * sobre, 12.0f + 4.0f * sobre);
    if (sobre > 0.01f) paintSombra (g, c, rad, luz.withAlpha (0.16f * sobre), 26.0f, 0.0f);

    juce::Path forma;
    forma.addRoundedRectangle (c, rad);
    g.setGradientFill (juce::ColourGradient (col::panel1, 0.0f, c.getY(), col::panel2, 0.0f, c.getBottom(), false));
    g.fillPath (forma);

    // a captura a sangue, fundida co panel e cun tinte da súa cor
    const int hc = altoCaptura ((int) c.getWidth());
    const auto zc = c.withHeight ((float) hc);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (forma);
        auto im = modelo.miniatura (produto);
        if (im.isValid())
        {
            g.setColour (col::visor);
            g.fillRect (zc);
            paintCuberta (g, im, zc, 1.0f + 0.05f * sobre, 0.92f);
        }
        else paintSenCaptura (g, zc.withTrimmedBottom (18.0f));
        // primeiro o tinte, e por riba o fundido á cor que ten o panel nesa altura (sen bordo)
        g.setGradientFill (juce::ColourGradient (luz.withAlpha (0.22f), 0.0f, zc.getBottom(), luz.withAlpha (0.0f), 0.0f, zc.getBottom() - zc.getHeight() * 0.6f, false));
        g.fillRect (zc);
        const auto fondo = col::panel1.interpolatedWith (col::panel2, zc.getHeight() / c.getHeight());
        juce::ColourGradient fundido (fondo.withAlpha (0.0f), 0.0f, zc.getY() + zc.getHeight() * 0.35f, fondo, 0.0f, zc.getBottom(), false);
        fundido.addColour (0.7, fondo.withAlpha (0.55f));
        g.setGradientFill (fundido);
        g.fillRect (zc.withTrimmedTop (zc.getHeight() * 0.35f - 1.0f));
    }
    g.setColour (sobre > 0.01f ? luz.withAlpha (0.06f + 0.4f * sobre) : juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (c.reduced (0.5f), rad, 1.0f);
    if (hasKeyboardFocus (false))
    {
        g.setColour (luz.withAlpha (0.95f));
        g.drawRoundedRectangle (c.expanded (1.5f), rad + 1.5f, 2.0f);
    }

    // a pastilla do estado (ou o ✓ ao rematar), arriba á dereita sobre a captura
    const bool traballa = modelo.traballando (produto);
    if (visto > 0.0f)
    {
        const auto t = tr ("Feito");
        const float w = anchoPastilla (t);
        auto r = juce::Rectangle<float> (c.getRight() - 10.0f - w, c.getY() + 10.0f, w, 24.0f);
        juce::Graphics::ScopedSaveState ss (g);
        g.setOpacity (juce::jmin (1.0f, visto * 3.0f));
        paintPastilla (g, r, t, col::verde, false);
        paintVisto (g, r.withWidth (22.0f).translated (5.0f, 0.0f).reduced (4.0f), col::verde, 1.8f);
    }
    else if (! traballa)
    {
        const auto t = textoPastilla (e);
        const float w = anchoPastilla (t);
        paintPastilla (g, { c.getRight() - 10.0f - w, c.getY() + 10.0f, w, 24.0f }, t, corEstado (e));
    }

    // o nome e a descrición
    const float y0 = c.getY() + (float) hc - 24.0f;
    g.setColour (col::crema);
    g.setFont (cond (css (17.0f)));
    g.drawText (produto.nome, juce::Rectangle<float> (c.getX() + 16.0f, y0, c.getWidth() - 32.0f, 24.0f), juce::Justification::centredLeft, true);
    g.setColour (col::crema2);
    g.setFont (body (css (12.5f)));
    g.drawText (produto.descricion.isNotEmpty() ? produto.descricion : juce::String (U ("—")),
                juce::Rectangle<float> (c.getX() + 16.0f, y0 + 26.0f, c.getWidth() - 32.0f, 18.0f), juce::Justification::centredLeft, true);

    // o pé: o progreso, ou a versión (e a tecla)
    if (traballa)
        paintProgreso (g, modelo, produto, { c.getX() + 16.0f, y0 + 58.0f, c.getWidth() - 32.0f, 36.0f });
    else
    {
        auto zv = zonaVersion();
        paintAnacos (g, anacosVersion (modelo, produto), zv, body (css (12.0f)));
        if (e == Estado::actualizacion && produto.ultima && zv.contains (getMouseXYRelative().toFloat()) && isMouseOver (false))
        {
            // o clic abre as notas: subliñado
            const auto as = anacosVersion (modelo, produto);
            float w = 0.0f;
            for (auto& a : as) w += anchoTexto (body (css (12.0f)), a.t);
            g.setColour (luz.withAlpha (0.6f));
            g.fillRect (zv.getX(), zv.getCentreY() + 9.0f, w, 1.0f);
        }
    }
}

void Tarxeta::mouseEnter (const juce::MouseEvent&) { rato = true; }
void Tarxeta::mouseExit (const juce::MouseEvent&) { rato = false; repaint(); }
void Tarxeta::mouseMove (const juce::MouseEvent&) { if (modelo.situacion (produto).estado() == Estado::actualizacion) repaint (zonaVersion().toNearestInt().expanded (2)); }

void Tarxeta::mouseUp (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { menuProduto (modelo, this, produto, abrirFicha); return; }
    if (! e.mouseWasClicked() || ! caixa().contains (e.position)) return;
    if (zonaVersion().contains (e.position) && produto.ultima && modelo.situacion (produto).estado() == Estado::actualizacion
        && ! modelo.traballando (produto))
    {
        paneis::notas (modelo, this, produto);
        return;
    }
    if (abrirFicha) abrirFicha (produto);
}

bool Tarxeta::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::returnKey || k == juce::KeyPress::spaceKey) { if (abrirFicha) abrirFicha (produto); return true; }
    if (mover)
    {
        if (k == juce::KeyPress::leftKey)  { mover (this, -1, 0); return true; }
        if (k == juce::KeyPress::rightKey) { mover (this, 1, 0); return true; }
        if (k == juce::KeyPress::upKey)    { mover (this, 0, -1); return true; }
        if (k == juce::KeyPress::downKey)  { mover (this, 0, 1); return true; }
    }
    return false;
}

// ==================================================================== a banda das actualizacións
Destacado::Destacado (Modelo& m) : modelo (m)
{
    todo.getProperties().set ("primaria", true);
    todo.getProperties().set ("fontSize", 14.0);
    todo.setTooltip (tr ("Actualizar todo o que ten versión nova (e quitar as copias vellas do sistema). O que se substitúe vai ao Lixo."));
    todo.onClick = [this] { paneis::protexer (modelo, this, [mo = &modelo] { mo->actualizarTodo(); }); };
    vella.getProperties().set ("estilo", "aviso");
    vella.setTooltip (tr ("No teu cartafol xa está a última, pero hai unha copia vella no cartafol do sistema que o DAW tamén ve. Lévaa ao Lixo (o sistema pedirá o contrasinal)."));
    vella.onClick = [this]
    {
        auto* mo = &modelo;
        std::vector<Produto> ps;
        for (auto* p : modelo.porActualizar())
            if (modelo.situacion (*p).estado() == Estado::copiaVella) ps.push_back (*p);
        paneis::protexer (modelo, this, [mo, ps] { for (auto& p : ps) mo->quitarCopiasVellas (p); });
    };
    addAndMakeVisible (todo);
    addChildComponent (vella);
}

bool Destacado::hai() const { return ! modelo.porActualizar().empty(); }

void Destacado::actualizar()
{
    juce::StringArray novas, vellas, nomesVellas;
    juce::int64 bytes = 0;
    minis.clear();
    std::vector<juce::Image> minisVellas;
    for (auto* p : modelo.porActualizar())
    {
        const bool v = modelo.situacion (*p).estado() == Estado::copiaVella;
        (v ? vellas : novas).add (p->nomeCurto());
        if (v) nomesVellas.add (p->nome);
        if (! v && p->ultima)
            if (auto* a = p->ultima->arquivo()) bytes += a->tamano;
        auto im = modelo.miniatura (*p);
        if (im.isValid()) (v ? minisVellas : minis).push_back (im);
    }
    if (novas.isEmpty()) minis = minisVellas;
    if (minis.size() > 3) minis.resize (3);
    if (! novas.isEmpty())
    {
        titulo = novas.size() == 1 ? tr ("1 actualización dispoñible") : tr ("%1 actualizacións dispoñibles", juce::String (novas.size()));
        subtitulo = lista (novas);
        if (bytes > 0) subtitulo << U (" · ") << tr ("%1 en total", tamano (bytes));
        subtitulo << U (" · ") << tr ("pecha o DAW antes de actualizar");
        todo.setButtonText (tr ("Actualizar todo"));
    }
    else
    {
        titulo = vellas.size() == 1 ? tr ("Unha copia vella no sistema") : tr ("%1 copias vellas no sistema", juce::String (vellas.size()));
        subtitulo = lista (vellas) + U (" · ") + tr ("van ao Lixo; o sistema pedirá o contrasinal");
        todo.setButtonText (vellas.size() == 1 ? tr ("Quitar a vella") : tr ("Quitar as vellas"));
    }
    vella.setVisible (! novas.isEmpty() && ! vellas.isEmpty());
    vella.setButtonText (vellas.size() == 1 ? tr ("%1 ten unha copia vella no sistema", nomesVellas[0])
                                            : tr ("%1 plugins teñen unha copia vella no sistema", juce::String (vellas.size())));
    // (o que xa está traballando sáltase: «instalar» non fai nada con el)
    setTitle (titulo);
    resized();
    repaint();
}

int Destacado::alto (int ancho) const
{
    const bool e = ancho < 1000;
    return (e ? 116 : 100) + (e && vella.isVisible() ? 48 : 0);
}

void Destacado::resized()
{
    const int tw = Look::anchoTecla (todo, 46);
    todo.setBounds (getWidth() - 26 - tw, 50 - 22, tw, 44);
    const int vw = juce::jmin (Look::anchoTecla (vella, 30), getWidth() - 52);
    if (estreito()) vella.setBounds (26, 116 - 8, vw, 36);
    else vella.setBounds (todo.getX() - 18 - vw, 50 - 18, vw, 36);
}

void Destacado::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    const float rad = 18.0f;
    g.setColour (col::panel2);
    g.fillRoundedRectangle (r, rad);
    g.setGradientFill (juce::ColourGradient (col::laranxa.withAlpha (0.18f), r.getX(), r.getY() + r.getHeight() * 0.3f,
                                             col::laranxa.withAlpha (0.02f), r.getX() + r.getWidth() * 0.6f, r.getY() + r.getHeight() * 0.7f, false));
    g.fillRoundedRectangle (r, rad);
    g.setColour (col::laranxa.withAlpha (0.22f));
    g.drawRoundedRectangle (r, rad, 1.0f);

    // as miniaturas, unhas por riba das outras
    float x = 26.0f;
    const float cy = 50.0f;
    const bool conMinis = ! minis.empty() && getWidth() >= 900;
    if (conMinis)
    {
        for (size_t i = 0; i < minis.size(); ++i)
        {
            auto m = juce::Rectangle<float> (x + (float) i * 64.0f, cy - 28.0f, 86.0f, 56.0f);
            paintSombra (g, m, 8.0f, juce::Colours::black.withAlpha (0.5f), 12.0f, 6.0f);
            juce::Path p;
            p.addRoundedRectangle (m, 8.0f);
            {
                juce::Graphics::ScopedSaveState ss (g);
                g.reduceClipRegion (p);
                g.setColour (col::visor);
                g.fillRect (m);
                paintCuberta (g, minis[i], m);
            }
            g.setColour (col::panel2);
            g.drawRoundedRectangle (m.expanded (1.0f), 9.0f, 2.0f);
        }
        x += 86.0f + (float) (minis.size() - 1) * 64.0f + 24.0f;
    }
    const float fin = (float) (vella.isVisible() && ! estreito() ? vella.getX() : todo.getX()) - 20.0f;
    g.setColour (col::crema);
    g.setFont (cond (css (22.0f)));
    g.drawFittedText (titulo, juce::Rectangle<float> (x, cy - 24.0f, fin - x, 30.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.85f);
    g.setColour (col::crema2);
    g.setFont (body (css (13.0f)));
    if (estreito())
        g.drawFittedText (subtitulo, juce::Rectangle<float> (x, cy + 10.0f, fin - x, 40.0f).toNearestInt(), juce::Justification::topLeft, 2, 1.0f);
    else
        g.drawFittedText (subtitulo, juce::Rectangle<float> (x, cy + 8.0f, fin - x, 20.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.9f);
}

// ==================================================================== órfos
FilaOrfo::FilaOrfo (Modelo& m, const Instalado& i) : modelo (m), inst (i)
{
    quitar.setButtonText (tr ("Ao Lixo"));
    quitar.getProperties().set ("estilo", "fantasma");
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

void FilaOrfo::resized() { quitar.setBounds (getWidth() - 14 - 100, (getHeight() - 32) / 2, 100, 32); }

void FilaOrfo::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setGradientFill (juce::ColourGradient (acento (inst.plugin).withAlpha (0.08f), r.getX(), 0.0f, juce::Colours::transparentBlack, r.getX() + r.getWidth() * 0.5f, 0.0f, false));
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (r, 12.0f, 1.0f);
    paintLed (g, { r.getX() + 18.0f, r.getCentreY() }, true, acento (inst.plugin));
    g.setColour (col::crema);
    g.setFont (cond (css (14.0f)));
    g.drawText (inst.plugin, r.withTrimmedLeft (34.0f).withWidth (220.0f), juce::Justification::centredLeft, true);
    g.setColour (col::crema3);
    g.setFont (body (css (12.0f)));
    g.drawText (etiqueta (inst.formato) + U (" · ") + (inst.textoVersion.isNotEmpty() ? inst.textoVersion : juce::String ("?")) + U (" · ")
                    + inst.ficheiro.getParentDirectory().getFullPathName().replace (juce::File::getSpecialLocation (juce::File::userHomeDirectory).getFullPathName(), "~"),
                r.withTrimmedLeft (264.0f).withTrimmedRight (124.0f), juce::Justification::centredLeft, true);
}

// ==================================================================== lista
Lista::Lista (Modelo& m, std::function<void (const Produto&)> af) : modelo (m), abrirFicha (std::move (af)), destacado (m)
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
    addChildComponent (destacado);
}

void Lista::actualizar()
{
    const auto& ps = modelo.produtos();
    // reutilízanse as tarxetas (así non se perde o foco nin a animación)
    std::vector<std::unique_ptr<Tarxeta>> novas;
    for (auto& p : ps)
    {
        std::unique_ptr<Tarxeta> t;
        for (auto& v : tarxetas)
            if (v != nullptr && v->getProduto().id() == p.id()) { t = std::move (v); break; }
        if (t == nullptr)
        {
            t = std::make_unique<Tarxeta> (modelo, abrirFicha);
            t->mover = [this] (Tarxeta* d, int dx, int dy) { moverFoco (d, dx, dy); };
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
    destacado.setVisible (destacado.hai());
    if (destacado.hai()) destacado.actualizar();
    colocar (getWidth());
    repaint();
}

Tarxeta* Lista::tarxeta (const juce::String& nome)
{
    for (auto& t : tarxetas)
        if (t->getProduto().nome == nome) return t.get();
    return nullptr;
}

int Lista::visibles() const
{
    int n = 0;
    for (auto& t : tarxetas) n += t->isVisible() ? 1 : 0;
    return n;
}

void Lista::colocar (int ancho)
{
    if (ancho <= 0) return;
    const int m = ancho >= 1000 ? 40 : 20, gap = 20, M = Tarxeta::marxe;
    const int cols = juce::jlimit (1, 4, (ancho - 2 * m + gap) / (290 + gap));
    const int cw = (ancho - 2 * m - (cols - 1) * gap) / cols;
    const int ch = Tarxeta::alto (cw);
    int y = 6;
    titulos.clear();

    // o aviso de arriba: un erro (que se pecha), agardando polo DAW, «volve escanear» ou «sen conexión»
    const auto aviso = textoFaixa();
    const bool hai = aviso.isNotEmpty();
    pecharFaixa.setVisible (hai && ! (modelo.erro.isEmpty() && modelo.dawsAgardados.isEmpty() && modelo.lembranza.isEmpty()));
    if (hai)
    {
        juce::AttributedString as;
        as.append (aviso, body (css (13.0f)));
        juce::TextLayout tl;
        tl.createLayout (as, (float) (ancho - 2 * m - 48 - 52));
        faixa = { m, y, ancho - 2 * m, juce::jmax (48, juce::roundToInt (tl.getHeight()) + 28) };
        pecharFaixa.setBounds (faixa.getRight() - 42, faixa.getY() + 9, 30, 30);
        y = faixa.getBottom() + 18;
    }
    else faixa = {};

    if (destacado.isVisible())
    {
        destacado.setBounds (m, y, ancho - 2 * m, destacado.alto (ancho - 2 * m));
        y = destacado.getBottom() + 26;
    }

    // as seccións: instrumentos, efectos, paquetes (e os que aínda non están publicados, ao final)
    auto tipoDe = [this] (const Produto& p) { return modelo.situacion (p).estado() == Estado::soLocal ? TipoProduto::desconecido : p.tipo; };
    for (auto tp : { TipoProduto::instrumento, TipoProduto::efecto, TipoProduto::paquete, TipoProduto::desconecido })
    {
        std::vector<Tarxeta*> estas;
        for (auto& t : tarxetas)
        {
            const auto& p = t->getProduto();
            if (tipoDe (p) != tp) continue;
            const bool ve = casaPestana (p, pestana) && casaBusca (p, busca);
            t->setVisible (ve);
            if (ve) estas.push_back (t.get());
        }
        if (estas.empty()) continue;
        titulos.push_back ({ tp == TipoProduto::desconecido ? tr ("Só neste ordenador (sen publicar)") : tr (titulo (tp)), { m, y, ancho - 2 * m, 22 }, (int) estas.size() });
        y += 22 + 14;
        for (size_t i = 0; i < estas.size(); ++i)
        {
            const int c = (int) i % cols;
            if (i > 0 && c == 0) y += ch + gap;
            estas[i]->setBounds (m + c * (cw + gap) - M, y - M, cw + 2 * M, ch + 2 * M);
        }
        y += ch + 30;
    }
    const bool orfosVisibles = ! orfos.empty() && pestana == Pestana::todo && busca.trim().isEmpty();
    if (orfosVisibles)
    {
        titulos.push_back ({ tr ("Outros de Escoita neste ordenador"), { m, y, ancho - 2 * m, 22 }, (int) orfos.size() });
        y += 22 + 14;
        for (auto& o : orfos) { o->setVisible (true); o->setBounds (m, y, ancho - 2 * m, 52); y += 60; }
        y += 20;
    }
    else for (auto& o : orfos) o->setVisible (false);

    baleiro = {};
    if (visibles() == 0 && ! tarxetas.empty())
    {
        baleiro = { m, y + 20, ancho - 2 * m, 80 };
        y = baleiro.getBottom() + 20;
    }
    setSize (ancho, y);
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
        const auto r = t.r.toFloat();
        g.setColour (col::crema);
        g.setFont (cond (css (15.0f)));
        g.drawText (t.texto, r, juce::Justification::centredLeft, false);
        const float w = anchoTexto (cond (css (15.0f)), t.texto);
        g.setColour (col::crema3);
        drawCaps (g, juce::String (t.n), r.withTrimmedLeft (w + 12.0f).withWidth (40.0f).translated (0.0f, 1.5f), juce::Justification::centredLeft, css (10.0f), 0.22f);
    }
    if (! faixa.isEmpty())
    {
        auto r = faixa.toFloat().reduced (0.5f);
        const bool erro = modelo.erro.isNotEmpty();
        const bool agarda = ! erro && ! modelo.dawsAgardados.isEmpty();
        const bool lembra = ! erro && ! agarda && modelo.lembranza.isNotEmpty();
        const auto c = erro ? col::laranxa : lembra ? col::verde : agarda ? col::azul : col::ouro;
        g.setColour (col::panel2.withAlpha (0.85f));
        g.fillRoundedRectangle (r, 14.0f);
        g.setGradientFill (juce::ColourGradient (c.withAlpha (0.10f), r.getX(), 0.0f, c.withAlpha (0.0f), r.getX() + r.getWidth() * 0.5f, 0.0f, false));
        g.fillRoundedRectangle (r, 14.0f);
        g.setColour (c.withAlpha (0.22f));
        g.drawRoundedRectangle (r, 14.0f, 1.0f);
        paintLed (g, { r.getX() + 22.0f, r.getY() + 24.0f }, true, c);
        g.setColour (erro || agarda ? col::crema : col::crema2);
        g.setFont (body (css (13.0f)));
        g.drawFittedText (textoFaixa(), r.withTrimmedLeft (42.0f).withTrimmedRight (pecharFaixa.isVisible() ? 54.0f : 18.0f).reduced (0.0f, 14.0f).toNearestInt(),
                          juce::Justification::centredLeft, 5, 1.0f);
    }
    if (! baleiro.isEmpty())
    {
        g.setColour (col::crema2);
        g.setFont (cond (css (16.0f)));
        g.drawText (busca.trim().isNotEmpty() ? tr ("Nada casa con «%1»", busca.trim()) : tr ("Nada nesta pestana"),
                    baleiro.toFloat().withHeight (30.0f), juce::Justification::centred, true);
        g.setColour (col::crema3);
        g.setFont (body (css (13.0f)));
        g.drawText (tr ("Proba con outra palabra ou mira en «Todo»."), baleiro.toFloat().withTrimmedTop (34.0f).withHeight (20.0f), juce::Justification::centred, true);
    }
}

bool Lista::animar (float dt)
{
    bool algo = false;
    for (auto& t : tarxetas)
        if (t->isVisible()) algo = t->animar (dt) || algo;
    return algo;
}

void Lista::entrada()
{
    // de arriba abaixo e de esquerda a dereita, un pouco despois cada unha
    std::vector<Tarxeta*> vs;
    for (auto& t : tarxetas) if (t->isVisible()) vs.push_back (t.get());
    std::sort (vs.begin(), vs.end(), [] (Tarxeta* a, Tarxeta* b) { return a->getY() != b->getY() ? a->getY() < b->getY() : a->getX() < b->getX(); });
    for (size_t i = 0; i < vs.size(); ++i) vs[i]->entrar (0.05f + 0.045f * (float) i);
}

void Lista::moverFoco (Tarxeta* desde, int dx, int dy)
{
    std::vector<Tarxeta*> vs;
    for (auto& t : tarxetas) if (t->isVisible()) vs.push_back (t.get());
    std::sort (vs.begin(), vs.end(), [] (Tarxeta* a, Tarxeta* b) { return a->getY() != b->getY() ? a->getY() < b->getY() : a->getX() < b->getX(); });
    const auto i = std::find (vs.begin(), vs.end(), desde);
    if (i == vs.end()) return;
    Tarxeta* destino = nullptr;
    if (dx != 0)
    {
        const auto j = (int) (i - vs.begin()) + dx;
        if (j >= 0 && j < (int) vs.size()) destino = vs[(size_t) j];
    }
    else
    {
        // a fila de arriba (ou de abaixo) máis próxima, e nela a tarxeta máis cerca en horizontal
        int mellorY = -1;
        for (auto* t : vs)
        {
            const int d = (t->getY() - desde->getY()) * dy;
            if (d > 0 && (mellorY < 0 || d < mellorY)) mellorY = d;
        }
        if (mellorY > 0)
        {
            int mellorX = std::numeric_limits<int>::max();
            for (auto* t : vs)
                if ((t->getY() - desde->getY()) * dy == mellorY && std::abs (t->getX() - desde->getX()) < mellorX)
                {
                    mellorX = std::abs (t->getX() - desde->getX());
                    destino = t;
                }
        }
    }
    if (destino != nullptr) destino->grabKeyboardFocus();
}

// ==================================================================== a ficha
Ficha::Ficha (Modelo& m) : modelo (m)
{
    pecharBtn.getProperties().set ("icona", "pechar");
    anterior.getProperties().set ("icona", "anterior");
    seguinte.getProperties().set ("icona", "seguinte");
    pecharBtn.setTitle (tr ("Pechar"));
    anterior.setTitle (tr ("Anterior"));
    seguinte.setTitle (tr ("Seguinte"));
    pecharBtn.onClick = [this] { pechar(); };
    anterior.onClick = [this] { ir (indice - 1); };
    seguinte.onClick = [this] { ir (indice + 1); };
    accion.getProperties().set ("fontSize", 14.0);
    accion.onClick = [this] { facerAccion (modelo, this, produto); };
    manual.setButtonText (tr ("Manual"));
    manual.getProperties().set ("estilo", "fantasma");
    manual.getProperties().set ("fontSize", 14.0);
    manual.onClick = [this] { modelo.abrirManual (produto); };
    presets.setButtonText (tr ("Presets"));
    presets.getProperties().set ("estilo", "fantasma");
    presets.getProperties().set ("fontSize", 14.0);
    presets.setTooltip (tr ("Abrir o cartafol dos presets"));
    presets.onClick = [this] { for (auto& f : modelo.cartafolesPresets (produto)) f.revealToUser(); };
    mais.setButtonText (U ("⋯"));
    mais.getProperties().set ("estilo", "fantasma");
    mais.getProperties().set ("fontSize", 16.0);
    mais.setTitle (tr ("Máis"));
    mais.setTooltip (tr ("Notas da versión, volver atrás, manual, presets, reinstalar, desinstalar…"));
    mais.onClick = [this] { menuProduto (modelo, &mais, produto, {}); };
    for (auto* b : { &pecharBtn, &accion, &manual, &presets, &mais }) addAndMakeVisible (b);
    for (auto* b : { &anterior, &seguinte }) addChildComponent (b);

    notas.setMultiLine (true, true);
    notas.setReadOnly (true);
    notas.setCaretVisible (false);
    notas.setScrollbarsShown (true);
    notas.setPopupMenuEnabled (true);
    notas.setWantsKeyboardFocus (false);
    notas.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    notas.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    notas.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    notas.setColour (juce::TextEditor::textColourId, col::crema2);
    notas.setColour (juce::TextEditor::highlightColourId, col::laranxa.withAlpha (0.35f));
    notas.setColour (juce::TextEditor::highlightedTextColourId, col::crema);
    notas.setFont (body (css (13.0f)));
    notas.setLineSpacing (1.25f);
    notas.setIndents (16, 12);
    notas.setTitle (tr ("Novidades"));
    addAndMakeVisible (notas);

    setWantsKeyboardFocus (true);
    setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);
    setVisible (false);
}

juce::Image Ficha::imaxe (int i)
{
    if (auto it = imaxes.find (i); it != imaxes.end() && it->second.isValid()) return it->second;
    if (i < 0 || i >= produto.capturas.size()) return {};
    const auto url = produto.capturas[i];
    auto im = modelo.capturaSeHai (url);
    if (! im.isValid() && modelo.escenaActual.isNotEmpty()) im = modelo.captura (url);   // (a captura: sen fíos)
    if (im.isValid()) { imaxes[i] = im; return im; }
    if (imaxes.count (i) == 0)
    {
        imaxes[i] = {};   // pedida
        juce::Component::SafePointer<Ficha> sp (this);
        const auto id = produto.id();
        modelo.pedirCaptura (url, [sp, i, id] (juce::Image novo)
        {
            if (sp == nullptr || sp->produto.id() != id) return;
            sp->imaxes[i] = novo;
            sp->repaint();
        });
    }
    return i == 0 ? modelo.miniatura (produto) : juce::Image();
}

void Ficha::abrir (const Produto& p, int i)
{
    const bool outro = p.id() != produto.id() || ! isVisible();
    produto = p;
    if (outro)
    {
        imaxes.clear();
        indice = juce::jlimit (0, juce::jmax (0, p.capturas.size() - 1), i);
        const auto v = p.ultima ? texto (p.ultima->version()) : (p.versionLocal ? p.versionLocal->texto() : juce::String());
        tituloNotas = v.isNotEmpty() ? tr ("Novidades na %1", v) : tr ("Novidades");
        notas.setText (tr ("Cargando as notas…"), false);
        juce::Component::SafePointer<Ficha> sp (this);
        const auto id = p.id();
        modelo.pedirNotas (p, [sp, id] (const juce::String& n)
        {
            if (sp == nullptr || sp->produto.id() != id) return;
            sp->porNotas (n);
        });
    }
    setVisible (true);
    toFront (false);
    refrescar();
    if (isShowing())
    {
        if (accion.isVisible()) accion.grabKeyboardFocus();
        else grabKeyboardFocus();
    }
}

void Ficha::porNotas (const juce::String& n)
{
    notas.setText (n.isNotEmpty() ? textos::markdownLexible (n) : tr ("Esta versión non ten notas."), false);
    notas.moveCaretToTop (false);
    colocar();
}

void Ficha::pechar()
{
    setVisible (false);
    if (auto* p = getParentComponent()) p->grabKeyboardFocus();
}

void Ficha::ir (int i)
{
    if (produto.capturas.size() < 2) return;
    indice = (i + produto.capturas.size()) % produto.capturas.size();
    repaint();
}

void Ficha::refrescar()
{
    if (! isVisible()) return;
    for (auto& p : modelo.produtos())
        if (p.id() == produto.id()) { produto = p; break; }
    const auto s = modelo.situacion (produto);
    const auto e = s.estado();
    const auto luz = acento (produto.nome);
    const auto v = produto.ultima ? texto (produto.ultima->version()) : juce::String();
    for (auto* b : { &pecharBtn, &anterior, &seguinte, &accion, &manual, &presets, &mais }) b->getProperties().set ("led", (juce::int64) luz.getARGB());

    // a acción principal (co número da versión)
    estiloAccion (accion, e);
    juce::String ta = textoAccion (e);
    if (e == Estado::actualizacion) ta = tr ("Actualizar á %1", v);
    else if (e == Estado::nonInstalado) ta = tr ("Instalar a %1", v);
    accion.setButtonText (ta);
    accion.setTooltip (axudaAccion (s));
    const bool traballa = modelo.traballando (produto);
    accion.setVisible (ta.isNotEmpty() && ! traballa);
    manual.setVisible (produto.manual.isNotEmpty());
    presets.setVisible (! modelo.cartafolesPresets (produto).isEmpty());
    anterior.setVisible (produto.capturas.size() > 1);
    seguinte.setVisible (produto.capturas.size() > 1);

    // os chips: o instalado (formato e versión) e os sistemas da versión publicada
    chips.clear();
    for (auto f : todosOsFormatos())
    {
        juce::StringArray vs;
        bool sistema = false;
        for (auto& i : s.relevantes())
            if (i.formato == f) { vs.addIfNotAlreadyThere (i.textoVersion.isNotEmpty() ? i.textoVersion : juce::String ("?")); sistema = sistema || i.ubicacion.eSistema(); }
        if (! vs.isEmpty()) chips.add (etiqueta (f) + " " + vs.joinIntoString ("/") + (sistema ? " (" + tr ("sistema") + ")" : juce::String()));
    }
    if (chips.isEmpty() && e != Estado::soLocal) chips.add (tr ("Sen instalar"));
    if (produto.ultima)
    {
        juce::StringArray sos;
        if (produto.ultima->arquivo (SO::mac) != nullptr) sos.add ("Mac");
        if (produto.ultima->arquivo (SO::win) != nullptr) sos.add ("Windows");
        if (produto.ultima->arquivo (SO::lin) != nullptr) sos.add ("Linux");
        if (! sos.isEmpty()) chips.add (sos.joinIntoString (U (" · ")));
    }

    // as versións: a publicada, a instalada e as gardadas (para volver a elas)
    filas.clear();
    const auto ult = produto.ultima ? produto.ultima->version() : std::nullopt;
    const auto ins = s.versionInstalada();
    if (ult)
        filas.push_back ({ ult->texto() + U (" · ") + tr ("actual") + (ins && *ins == *ult ? U (" · ") + tr ("instalada") : juce::String()),
                           produto.ultima->publicado != juce::Time() ? dataCurta (produto.ultima->publicado) : juce::String(), true, {} });
    if (ins && (! ult || *ins != *ult))
        filas.push_back ({ ins->texto() + U (" · ") + tr ("instalada"), {}, ! ult, {} });
    for (auto& c : modelo.copiasDe (produto))
        if (! ult || c != *ult) filas.push_back ({ c.texto(), {}, false, c });
    volver.clear();
    const bool libre = ! traballa && ! s.relevantes().empty();
    for (auto& f : filas)
    {
        if (! f.volverA) continue;
        auto b = std::make_unique<juce::TextButton> (tr ("Volver a esta"));
        b->getProperties().set ("estilo", "ligazon");
        b->getProperties().set ("led", (juce::int64) luz.getARGB());
        b->setTooltip (tr ("Volver á %1 (gardada, sen rede)", f.volverA->texto()));
        b->setEnabled (libre);
        auto* mo = &modelo;
        const auto p = produto;
        const auto ver = *f.volverA;
        juce::Component::SafePointer<Ficha> sp (this);
        b->onClick = [mo, p, ver, sp] { paneis::protexer (*mo, sp.getComponent(), [m2 = mo, p2 = p, v2 = ver] { m2->volverA (p2, v2); }); };
        addAndMakeVisible (*b);
        volver.push_back (std::move (b));
    }
    setTitle (produto.nome);
    setDescription (produto.descricion);
    colocar();
    repaint();
}

void Ficha::colocar()
{
    if (getWidth() <= 0) return;
    const int W = getWidth(), H = getHeight();
    const int ix = juce::jlimit (16, 70, W / 20), iy = juce::jlimit (16, 44, H / 20);
    caixa = getLocalBounds().reduced (ix, iy);
    const bool estreita = W < 980;
    const int we = juce::roundToInt ((float) caixa.getWidth() * (estreita ? 0.46f : 1.45f / 2.45f));
    esquerda = caixa.withWidth (we);
    dereita = caixa.withTrimmedLeft (we);
    const int pad = estreita ? 24 : 34;
    auto r = dereita.reduced (pad);
    pecharBtn.setBounds (r.getRight() - 36, r.getY() - 6, 36, 36);
    int y = r.getY() + 16 + 6;   // a etiqueta da categoría
    yNome = y;
    y += 42;
    // a descrición (ata tres liñas)
    {
        juce::AttributedString as;
        as.append (produto.descricion, body (css (14.0f)));
        juce::TextLayout tl;
        tl.createLayout (as, (float) r.getWidth());
        altoDesc = juce::jmin (3 * 22, juce::roundToInt (tl.getHeight()) + 2);
    }
    yDesc = y;
    y += altoDesc + 18;
    // os chips (en filas, se non collen)
    {
        int x = r.getX(), yc = y;
        const auto f = semi (css (11.0f));
        for (auto& c : chips)
        {
            const int w = juce::roundToInt (anchoTexto (f, c)) + 22;
            if (x + w > r.getRight() && x > r.getX()) { x = r.getX(); yc += 34; }
            x += w + 8;
        }
        zonaChips = { r.getX(), y, r.getWidth(), chips.isEmpty() ? 0 : yc - y + 28 };
        y = zonaChips.getBottom() + (chips.isEmpty() ? 0 : 18);
    }
    // as teclas
    {
        int x = r.getX(), yb = y;
        for (auto* b : { &accion, &manual, &presets, &mais })
        {
            if (! b->isVisible()) continue;
            const int w = b == &mais ? 54 : juce::jmax (b == &accion ? 150 : 100, Look::anchoTecla (*b, 46));
            if (x + w > r.getRight() && x > r.getX()) { x = r.getX(); yb += 54; }
            b->setBounds (x, yb, w, 44);
            x += w + 10;
        }
        if (modelo.traballando (produto) && ! accion.isVisible())
        {
            // o progreso vai antes das outras teclas
            const int wp = juce::jmin (240, r.getWidth());
            for (auto* b : { &manual, &presets, &mais }) if (b->isVisible()) b->setTopLeftPosition (b->getX() + wp + 10, b->getY());
            accion.setBounds (r.getX(), yb, wp, 44);
        }
        y = yb + 44 + 26;
    }
    // abaixo: as versións (e os plugins dunha serie); no medio, as notas
    const int altoFila = 36;
    const int hv = filas.empty() ? 0 : 24 + (int) filas.size() * (altoFila + 2);
    const int filasSerie = produto.eSerie() ? (produto.plugins.size() + 2) / 3 : 0;
    const int hs = filasSerie > 0 ? 24 + filasSerie * 26 + 8 : 0;
    const int abaixo = hv + (hs > 0 ? hs + 20 : 0);
    juce::AttributedString as;
    as.append (notas.getText(), body (css (13.0f)));
    juce::TextLayout tl;
    tl.createLayout (as, (float) r.getWidth() - 40.0f);
    const int hTexto = juce::roundToInt (tl.getHeight() * 1.25f) + 28;
    const int dispo = r.getBottom() - y - 24 - (abaixo > 0 ? abaixo + 24 : 0);
    const int hn = juce::jmax (70, juce::jmin (hTexto, dispo));
    zonaNotas = { r.getX(), y + 24, r.getWidth(), hn };
    notas.setBounds (zonaNotas.reduced (1));
    y = zonaNotas.getBottom() + 26;
    zonaVersions = { r.getX(), y, r.getWidth(), hv };
    {
        size_t k = 0;
        for (size_t i = 0; i < filas.size(); ++i)
        {
            if (! filas[i].volverA || k >= volver.size()) continue;
            auto& b = *volver[k++];
            const int w = Look::anchoTecla (b, 24);
            b.setBounds (zonaVersions.getRight() - w - 2, zonaVersions.getY() + 24 + (int) i * (altoFila + 2) + 2, w, altoFila - 4);
        }
    }
    y = zonaVersions.getBottom() + (hv > 0 ? 20 : 0);
    zonaSerie = hs > 0 ? juce::Rectangle<int> (r.getX(), y, r.getWidth(), hs) : juce::Rectangle<int>();

    // a esquerda: a captura e as miniaturas
    const int ladoBtn = produto.capturas.size() > 1 ? 52 : 34;
    anterior.setBounds (esquerda.getX() + 10, esquerda.getCentreY() - 18 - 40, 36, 36);
    seguinte.setBounds (esquerda.getRight() - 46, esquerda.getCentreY() - 18 - 40, 36, 36);
    juce::ignoreUnused (ladoBtn);
}

juce::Rectangle<float> Ficha::zonaImaxe() const
{
    const int lado = produto.capturas.size() > 1 ? 54 : 34;
    auto z = esquerda.reduced (lado, 34).toFloat();
    if (produto.capturas.size() > 1) z.removeFromBottom (64.0f + 18.0f);
    auto im = const_cast<Ficha*> (this)->imaxe (indice);
    if (! im.isValid()) return z.withSizeKeepingCentre (z.getWidth(), juce::jmin (z.getHeight(), z.getWidth() * 0.62f));
    return juce::RectanglePlacement (juce::RectanglePlacement::centred).appliedTo (juce::Rectangle<float> ((float) im.getWidth(), (float) im.getHeight()), z);
}

std::vector<juce::Rectangle<float>> Ficha::zonasMiniaturas() const
{
    std::vector<juce::Rectangle<float>> r;
    const int n = produto.capturas.size();
    if (n < 2) return r;
    const auto zi = zonaImaxe();
    const float ancho = (float) esquerda.getWidth() - 2.0f * 54.0f;
    const float w = juce::jmin (110.0f, (ancho - (float) (n - 1) * 10.0f) / (float) n);
    const float h = w * 64.0f / 110.0f;
    float x = (float) esquerda.getCentreX() - ((float) n * w + (float) (n - 1) * 10.0f) * 0.5f;
    const float y = zi.getBottom() + 18.0f;
    for (int i = 0; i < n; ++i) { r.push_back ({ x, y, w, h }); x += w + 10.0f; }
    return r;
}

void Ficha::resized() { colocar(); }

void Ficha::paint (juce::Graphics& g)
{
    // o fondo escurecido
    g.fillAll (juce::Colour (0xb8050810));
    const auto luz = acento (produto.nome);
    const auto c = caixa.toFloat();
    const float rad = 22.0f;
    paintSombra (g, c, rad, juce::Colours::black.withAlpha (0.6f), 40.0f, 30.0f);
    juce::Path forma;
    forma.addRoundedRectangle (c, rad);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (forma);
        g.setColour (col::panel2);
        g.fillRect (c);
        // a esquerda: a captura sobre un halo da cor do plugin
        const auto e = esquerda.toFloat();
        g.setColour (juce::Colour (0xff080c18));
        g.fillRect (e);
        {
            juce::Graphics::ScopedSaveState s2 (g);
            const auto ctr = juce::Point<float> (e.getCentreX(), e.getY() + e.getHeight() * 0.4f);
            g.addTransform (juce::AffineTransform::scale (1.0f, 400.0f / 600.0f, ctr.x, ctr.y));
            juce::ColourGradient halo (luz.withAlpha (0.30f), ctr.x, ctr.y, luz.withAlpha (0.0f), ctr.x + 600.0f, ctr.y, true);
            halo.addColour (0.35, luz.withAlpha (0.16f));
            halo.addColour (0.7, luz.withAlpha (0.0f));
            g.setGradientFill (halo);
            g.fillRect (e.expanded (0.0f, e.getHeight()));
        }
        const auto zi = zonaImaxe();
        auto im = imaxe (indice);
        paintSombra (g, zi, 12.0f, juce::Colours::black.withAlpha (0.6f), 34.0f, 26.0f);
        {
            juce::Path p;
            p.addRoundedRectangle (zi, 12.0f);
            juce::Graphics::ScopedSaveState s3 (g);
            g.reduceClipRegion (p);
            if (im.isValid())
            {
                g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
                g.setOpacity (1.0f);   // (drawImage usa o alfa da última cor)
                g.drawImage (im, zi, juce::RectanglePlacement::stretchToFit);
            }
            else paintSenImaxe (g, produto, zi, css (40.0f));
        }
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawRoundedRectangle (zi, 12.0f, 1.0f);
        // as miniaturas
        const auto zm = zonasMiniaturas();
        for (size_t i = 0; i < zm.size(); ++i)
        {
            const auto& m = zm[i];
            juce::Path p;
            p.addRoundedRectangle (m, 7.0f);
            {
                juce::Graphics::ScopedSaveState s4 (g);
                g.reduceClipRegion (p);
                g.setColour (col::visor);
                g.fillRect (m);
                auto mi = imaxe ((int) i);
                if (mi.isValid()) paintCuberta (g, mi, m, 1.0f, (int) i == indice ? 1.0f : 0.55f);
            }
            if ((int) i == indice)
            {
                g.setColour (luz);
                g.drawRoundedRectangle (m.expanded (1.0f), 8.0f, 2.0f);
            }
        }
        // a dereita: o fondo
        const auto d = dereita.toFloat();
        g.setGradientFill (juce::ColourGradient (col::panel1, 0.0f, d.getY(), col::panel2, 0.0f, d.getBottom(), false));
        g.fillRect (d);
    }
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (c.reduced (0.5f), rad, 1.0f);

    // a dereita: categoría, nome e descrición
    const int pad = getWidth() < 980 ? 24 : 34;
    auto r = dereita.reduced (pad).toFloat();
    const auto categoria = produto.tipo == TipoProduto::instrumento ? tr ("Instrumento")
                         : produto.tipo == TipoProduto::efecto ? tr ("Efecto")
                         : produto.tipo == TipoProduto::paquete ? tr ("Paquete") : juce::String ("Plugin");
    g.setColour (luz);
    drawCaps (g, categoria, r.withHeight (16.0f), juce::Justification::centredLeft, css (10.0f), 0.22f);
    g.setColour (col::crema);
    g.setFont (cond (css (30.0f)));
    g.drawFittedText (produto.nome, juce::Rectangle<float> (r.getX(), (float) yNome, r.getWidth() - 46.0f, 40.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    g.setColour (col::crema2);
    g.setFont (body (css (14.0f)));
    g.drawFittedText (produto.descricion, juce::Rectangle<float> (r.getX(), (float) yDesc, r.getWidth(), (float) altoDesc).toNearestInt(), juce::Justification::topLeft, 3, 1.0f);

    // os chips
    {
        float x = (float) zonaChips.getX(), y = (float) zonaChips.getY();
        const auto f = semi (css (11.0f));
        for (auto& ch : chips)
        {
            const float w = anchoTexto (f, ch) + 22.0f;
            if (x + w > (float) zonaChips.getRight() && x > (float) zonaChips.getX()) { x = (float) zonaChips.getX(); y += 34.0f; }
            auto cr = juce::Rectangle<float> (x, y, w, 28.0f);
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.fillRoundedRectangle (cr, 14.0f);
            g.setColour (juce::Colours::white.withAlpha (0.07f));
            g.drawRoundedRectangle (cr.reduced (0.5f), 14.0f, 1.0f);
            g.setColour (col::crema2);
            g.setFont (f);
            g.drawText (ch, cr, juce::Justification::centred, false);
            x += w + 8.0f;
        }
    }
    // o progreso, se está traballando
    if (modelo.traballando (produto) && ! accion.isVisible())
        paintProgreso (g, modelo, produto, accion.getBounds().toFloat());

    // as notas
    g.setColour (col::crema3);
    drawCaps (g, tituloNotas, zonaNotas.toFloat().withY ((float) zonaNotas.getY() - 24.0f).withHeight (14.0f), juce::Justification::centredLeft, css (10.0f), 0.22f);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (zonaNotas.toFloat(), 12.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (zonaNotas.toFloat().reduced (0.5f), 12.0f, 1.0f);

    // as versións
    if (! filas.empty())
    {
        g.setColour (col::crema3);
        drawCaps (g, tr ("Versións"), zonaVersions.toFloat().withHeight (14.0f), juce::Justification::centredLeft, css (10.0f), 0.22f);
        for (size_t i = 0; i < filas.size(); ++i)
        {
            auto fr = juce::Rectangle<float> ((float) zonaVersions.getX(), (float) zonaVersions.getY() + 24.0f + (float) i * 38.0f, (float) zonaVersions.getWidth(), 36.0f);
            if (i % 2 == 0) { g.setColour (juce::Colours::white.withAlpha (0.025f)); g.fillRoundedRectangle (fr, 8.0f); }
            g.setColour (filas[i].forte ? col::crema : col::crema2);
            g.setFont (body (css (13.0f)));
            g.drawText (filas[i].esquerda, fr.reduced (12.0f, 0.0f), juce::Justification::centredLeft, true);
            if (filas[i].dereita.isNotEmpty())
            {
                g.setColour (col::crema2);
                g.drawText (filas[i].dereita, fr.reduced (12.0f, 0.0f), juce::Justification::centredRight, true);
            }
        }
    }
    // os plugins dunha serie, un a un
    if (! zonaSerie.isEmpty())
    {
        const auto s = modelo.situacion (produto);
        g.setColour (col::crema3);
        drawCaps (g, tr ("Os plugins da serie"), zonaSerie.toFloat().withHeight (14.0f), juce::Justification::centredLeft, css (10.0f), 0.22f);
        const float cw = (float) zonaSerie.getWidth() / 3.0f;
        const auto rel = s.relevantes();
        const auto ult = produto.ultima ? produto.ultima->version() : std::nullopt;
        for (int i = 0; i < produto.plugins.size(); ++i)
        {
            auto cr = juce::Rectangle<float> ((float) zonaSerie.getX() + cw * (float) (i % 3), (float) zonaSerie.getY() + 24.0f + (float) (i / 3) * 26.0f, cw - 10.0f, 26.0f);
            std::optional<Version> v;
            bool ten = false;
            for (auto& co : rel) if (co.plugin == produto.plugins[i]) { ten = true; if (co.version && (! v || *co.version < *v)) v = co.version; }
            const bool vello = ten && ult && (! v || *v < *ult);
            paintLed (g, { cr.getX() + 5.0f, cr.getCentreY() }, ten, vello ? col::laranxa : acento (produto.plugins[i]));
            g.setColour (ten ? col::crema2 : col::crema3);
            g.setFont (body (css (12.0f)));
            g.drawText (produto.plugins[i].startsWith ("Barullo ") ? produto.plugins[i].substring (8) : produto.plugins[i], cr.withTrimmedLeft (16.0f), juce::Justification::centredLeft, true);
        }
    }
}

void Ficha::mouseMove (const juce::MouseEvent& e)
{
    bool sobre = false;
    for (auto& m : zonasMiniaturas()) sobre = sobre || m.contains (e.position);
    setMouseCursor (sobre ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void Ficha::mouseUp (const juce::MouseEvent& e)
{
    if (! caixa.contains (e.getPosition())) { pechar(); return; }
    const auto zm = zonasMiniaturas();
    for (size_t i = 0; i < zm.size(); ++i)
        if (zm[i].contains (e.position)) { indice = (int) i; repaint(); return; }
    if (zonaImaxe().contains (e.position) && produto.capturas.size() > 1) ir (indice + 1);
}

bool Ficha::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { pechar(); return true; }
    if (k == juce::KeyPress::leftKey) { ir (indice - 1); return true; }
    if (k == juce::KeyPress::rightKey) { ir (indice + 1); return true; }
    return false;
}

// ==================================================================== a xanela
Principal::Principal (Modelo& m)
    : modelo (m),
      lista (m, [this] (const Produto& p) { abrirFicha (p); }),
      ficha (m)
{
    setLookAndFeel (&look);
    setTitle ("Barullo Xestor");
    // as pestanas: todo, instrumentos, efectos, paquetes (co reconto)
    int orde = 1;
    for (auto t : { Pestana::todo, Pestana::instrumentos, Pestana::efectos, Pestana::paquetes })
    {
        auto b = std::make_unique<juce::TextButton> (t == Pestana::todo ? tr ("Todo") : t == Pestana::instrumentos ? tr ("Instrumentos")
                                                     : t == Pestana::efectos ? tr ("Efectos") : tr ("Paquetes"));
        b->getProperties().set ("estilo", "pestana");
        b->setToggleState (t == Pestana::todo, juce::dontSendNotification);
        b->setExplicitFocusOrder (orde++);
        b->onClick = [this, t]
        {
            lista.pestana = t;
            for (size_t i = 0; i < pestanaBtn.size(); ++i) pestanaBtn[i]->setToggleState ((int) i == (int) t, juce::dontSendNotification);
            lista.colocar (lista.getWidth());
            vista.setViewPosition (0, 0);
            repaint();
        };
        addAndMakeVisible (*b);
        pestanaBtn.push_back (std::move (b));
    }
    // o buscador
    buscador.getProperties().set ("buscador", true);
    buscador.setTextToShowWhenEmpty (tr ("Buscar"), col::crema3);
    buscador.setFont (body (css (13.0f)));
    buscador.setIndents (34, 0);
    buscador.setJustification (juce::Justification::centredLeft);
    buscador.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    buscador.setColour (juce::TextEditor::textColourId, col::crema);
    buscador.setColour (juce::TextEditor::highlightColourId, col::laranxa.withAlpha (0.35f));
    buscador.setColour (juce::TextEditor::highlightedTextColourId, col::crema);
    buscador.setTitle (tr ("Buscar"));
    buscador.setDescription (tr ("Buscar polo nome ou a descrición (Cmd/Ctrl + F)"));
    buscador.setExplicitFocusOrder (orde++);
    buscador.onTextChange = [this]
    {
        lista.busca = buscador.getText();
        lista.colocar (lista.getWidth());
        vista.setViewPosition (0, 0);
        recontos();
    };
    buscador.onEscapeKey = [this] { buscador.clear(); lista.busca.clear(); lista.colocar (lista.getWidth()); recontos(); grabKeyboardFocus(); };
    buscador.onReturnKey = [this]
    {
        // Intro: á primeira tarxeta que casa
        for (auto* c : lista.getChildren())
            if (auto* t = dynamic_cast<Tarxeta*> (c); t != nullptr && t->isVisible()) { t->grabKeyboardFocus(); return; }
    };
    addAndMakeVisible (buscador);

    refrescar.getProperties().set ("icona", "refrescar");
    refrescar.setTitle (tr ("Comprobar agora"));
    refrescar.setTooltip (tr ("Preguntar agora a GitHub polas últimas versións (Cmd/Ctrl + R)."));
    refrescar.onClick = [this] { modelo.cargar (true); };
    refrescar.setExplicitFocusOrder (orde++);
    axustes.getProperties().set ("icona", "axustes");
    axustes.setTitle (tr ("Barullo Xestor: axustes"));
    axustes.setTooltip (tr ("Idioma, comprobar cada día, copia dos presets, copiar o informe, atallos do teclado…"));
    axustes.onClick = [this] { paneis::menuXestor (modelo, &axustes); };
    axustes.setExplicitFocusOrder (orde++);
    addAndMakeVisible (refrescar);
    addAndMakeVisible (axustes);
    volverProbar.getProperties().set ("primaria", true);
    volverProbar.setButtonText (tr ("Volver probar"));
    volverProbar.onClick = [this] { modelo.cargar (true); };
    addChildComponent (volverProbar);

    // o pé: os formatos (chips que se acenden) e «Hai Barullo Xestor X»
    for (auto f : formatosDe (soActual()))
    {
        auto b = std::make_unique<juce::TextButton> (etiqueta (f));
        b->getProperties().set ("estilo", "chip");
        b->setTooltip (tr ("Mirar e instalar %1. Pódense escoller varios; por defecto, só VST3.", etiqueta (f)));
        b->onClick = [this, f] { modelo.cambiarFormato (f); };
        addAndMakeVisible (*b);
        formatoBtn.push_back (std::move (b));
        formatos.push_back (f);
    }
    novaVersion.getProperties().set ("estilo", "aviso");
    novaVersion.getProperties().set ("fontSize", 12.0);
    novaVersion.onClick = [this] { paneis::xestorNovo (modelo, this); };
    addChildComponent (novaVersion);

    vista.setViewedComponent (&lista, false);
    vista.setScrollBarsShown (true, false);
    vista.setScrollOnDragMode (juce::Viewport::ScrollOnDragMode::never);
    vista.getVerticalScrollBar().setAutoHide (true);
    vista.setSingleStepSizes (0, 40);
    addAndMakeVisible (vista);
    addChildComponent (ficha);
    addChildComponent (modal);
    setWantsKeyboardFocus (true);
    // co teclado: Tab percorre a cabeceira, as tarxetas (e as súas teclas) e o pé, na orde en que se ven
    setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);
    juce::Desktop::getInstance().addFocusChangeListener (this);
    modelo.addChangeListener (this);
    setSize (anchoInicial, altoInicial);
    actualizar();
    ultimoTic = juce::Time::getMillisecondCounter();
    startTimerHz (60);
}

Principal::~Principal()
{
    juce::Desktop::getInstance().removeFocusChangeListener (this);
    modelo.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void Principal::changeListenerCallback (juce::ChangeBroadcaster*) { actualizar(); }

bool Principal::baleiro() const { return modelo.produtos().empty(); }

void Principal::recontos()
{
    const auto r = contar (modelo.produtos(), lista.busca);
    for (size_t i = 0; i < pestanaBtn.size(); ++i)
        pestanaBtn[i]->getProperties().set ("reconto", juce::String (r.de ((Pestana) i)));
    resized();
    repaint();
}

void Principal::actualizar()
{
    for (size_t i = 0; i < formatos.size(); ++i)
    {
        const bool on = std::find (modelo.axustes.formatos.begin(), modelo.axustes.formatos.end(), formatos[i]) != modelo.axustes.formatos.end();
        formatoBtn[i]->setToggleState (on, juce::dontSendNotification);
    }
    refrescar.setEnabled (! modelo.cargando);
    volverProbar.setVisible (baleiro() && ! modelo.cargando);
    vista.setVisible (! baleiro());
    for (auto& b : pestanaBtn) b->setVisible (! baleiro());
    buscador.setVisible (! baleiro());
    lista.actualizar();
    ficha.refrescar();
    recontos();
    if (! xaEntrou && ! baleiro() && isShowing()) { xaEntrou = true; lista.entrada(); }
}

void Principal::visibilityChanged()
{
    if (! xaEntrou && ! baleiro() && isShowing()) { xaEntrou = true; lista.entrada(); }
}

void Principal::timerCallback()
{
    const auto agora = juce::Time::getMillisecondCounter();
    const float dt = juce::jlimit (0.0f, 0.1f, (float) (agora - ultimoTic) * 0.001f);
    ultimoTic = agora;
    // o LED do pé respira mentres se comproba ou se traballa
    pulso += dt * 2.4f;
    if (modelo.cargando || ! modelo.traballos.empty()) repaint (pe);
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
            resized();
            repaint();
        }
    }
    if (isShowing()) lista.animar (dt);
    if (ficha.isVisible() && modelo.traballando (ficha.getProduto())) ficha.repaint();
}

bool Principal::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress ('r', juce::ModifierKeys::commandModifier, 0) || k == juce::KeyPress::F5Key) { modelo.cargar (true); return true; }
    if (k == juce::KeyPress ('f', juce::ModifierKeys::commandModifier, 0) && buscador.isVisible()) { buscador.grabKeyboardFocus(); buscador.selectAll(); return true; }
    if (k == juce::KeyPress::escapeKey && modal.isVisible()) { modal.pecharSen(); return true; }
    if (k == juce::KeyPress::escapeKey && ficha.isVisible()) { ficha.pechar(); return true; }
    if (k == juce::KeyPress::escapeKey && buscador.getText().isNotEmpty()) { buscador.clear(); buscador.onTextChange(); return true; }
    if (k == juce::KeyPress::escapeKey && ! modelo.dawsAgardados.isEmpty()) { modelo.deixarDeAgardar(); return true; }
    return false;
}

int Principal::altoCompleto (int ancho)
{
    lista.colocar (ancho);
    return corpo.getY() + lista.getHeight() + pe.getHeight() + 10;
}

void Principal::resized()
{
    const int W = getWidth(), H = getHeight();
    const int m = W >= 1000 ? 40 : 20;
    const int fila = 22;
    // a cabeceira: a icona e o nome; as pestanas; o buscador e as dúas iconas
    const int anchoLogo = 46 + juce::roundToInt (juce::jmax (anchoTexto (cond (css (17.0f)).withExtraKerningFactor (0.04f), "Barullo Xestor"),
                                                              anchoTexto (caps (css (8.5f), 0.22f), "ESCOITA " + upper (U ("· ") + tr ("Ferramentas")))));
    logo = { m, fila, anchoLogo, 36 };
    const int gap = W >= 1000 ? 28 : 12;
    int x = W - m;
    axustes.setBounds (x - 36, fila, 36, 36);
    x -= 36 + (W >= 1000 ? 14 : 10);
    refrescar.setBounds (x - 36, fila, 36, 36);
    x -= 36 + gap;
    int xp = dobreFila() ? m : logo.getRight() + 24 + 28;
    const int yp = dobreFila() ? fila + 36 + 14 : fila;
    for (auto& b : pestanaBtn)
    {
        const int w = Look::anchoTecla (*b, 28);
        b->setBounds (xp, yp + 1, w, 34);
        xp += w + 4;
    }
    const int fb = dobreFila() ? W - m : x;
    const int wb = juce::jlimit (120, 220, fb - xp - 16);
    zonaBusca = { fb - wb, yp, wb, 36 };
    buscador.setBounds (zonaBusca);
    corpo = { 0, yp + 36 + 22, W, 0 };

    // o pé
    pe = { 0, H - 36, W, 36 };
    const auto version = "Xestor " + juce::String (JUCE_APPLICATION_VERSION_STRING);
    int xf = W - m - juce::roundToInt (anchoTexto (body (css (12.0f)), version)) - 18;
    for (int i = (int) formatoBtn.size() - 1; i >= 0; --i)
    {
        auto& b = *formatoBtn[(size_t) i];
        const int w = Look::anchoTecla (b, 18);
        b.setBounds (xf - w, pe.getY() + 7, w, 22);
        xf -= w + 6;
    }
    if (novaVersion.isVisible())
    {
        const int w = juce::jmin (Look::anchoTecla (novaVersion, 30), xf - 220);
        novaVersion.setBounds (xf - 12 - w, pe.getY() + 5, w, 26);
    }
    corpo.setHeight (pe.getY() - corpo.getY());
    vista.setBounds (corpo);
    lista.colocar (corpo.getWidth() - (lista.getHeight() > corpo.getHeight() ? vista.getScrollBarThickness() : 0));
    volverProbar.setBounds (W / 2 - 90, corpo.getCentreY() + 60, 180, 40);
    ficha.setBounds (getLocalBounds());
    modal.setBounds (getLocalBounds());
}

void Principal::paintCabeceira (juce::Graphics& g)
{
    // a icona e o nome
    const auto l = logo.toFloat();
    const auto ic = iconaApp();
    const auto ri = juce::Rectangle<float> (l.getX(), l.getCentreY() - 17.0f, 34.0f, 34.0f);
    if (ic.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (ic, ri, juce::RectanglePlacement::stretchToFit);
    }
    g.setColour (col::crema);
    g.setFont (cond (css (17.0f)).withExtraKerningFactor (0.04f));
    g.drawText ("Barullo Xestor", juce::Rectangle<float> (ri.getRight() + 12.0f, l.getY() - 2.0f, 260.0f, 24.0f), juce::Justification::centredLeft, false);
    g.setColour (col::crema3);
    drawCaps (g, "Escoita " + juce::String (U ("· ")) + tr ("Ferramentas"), juce::Rectangle<float> (ri.getRight() + 13.0f, l.getY() + 22.0f, 260.0f, 12.0f),
              juce::Justification::centredLeft, css (8.5f), 0.22f);
    // o buscador: o fondo e a lupa (o texto debúxao o editor)
    if (buscador.isVisible())
    {
        const auto b = zonaBusca.toFloat().reduced (0.5f);
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (b, 10.0f);
        paintLupa (g, juce::Rectangle<float> (b.getX() + 8.0f, b.getY(), 22.0f, b.getHeight()), col::crema3);
    }
}

void Principal::paintPe (juce::Graphics& g)
{
    const auto r = pe.toFloat();
    g.setColour (juce::Colour (0x99070b16));
    g.fillRect (r);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRect (r.withHeight (1.0f));
    const int m = getWidth() >= 1000 ? 40 : 20;
    // o LED: verde ao día, laranxa (respirando) comprobando ou traballando, ouro sen rede
    juce::Colour c = col::verde;
    bool aceso = true;
    if (modelo.cargando || ! modelo.traballos.empty()) { c = col::laranxa; aceso = std::sin (pulso * 2.0f) > -0.3f; }
    else if (modelo.erro.isNotEmpty()) c = col::laranxa;
    else if (modelo.senRede()) c = col::ouro;
    const auto led = juce::Point<float> ((float) m + 4.0f, r.getCentreY());
    if (aceso)
    {
        g.setColour (c.withAlpha (0.3f)); g.fillEllipse (led.x - 5.0f, led.y - 5.0f, 10.0f, 10.0f);
        g.setColour (c); g.fillEllipse (led.x - 3.0f, led.y - 3.0f, 6.0f, 6.0f);
    }
    else { g.setColour (c.withAlpha (0.35f)); g.fillEllipse (led.x - 3.0f, led.y - 3.0f, 6.0f, 6.0f); }
    const float fin = (float) (novaVersion.isVisible() ? novaVersion.getX() : formatoBtn.empty() ? getWidth() - m : formatoBtn.front()->getX()) - 18.0f;
    g.setColour (col::crema3);
    g.setFont (body (css (12.0f)));
    auto msx = modelo.cargando ? tr ("Comprobando en GitHub…") : modelo.mensaxe.upToFirstOccurrenceOf ("\n", false, false);
    g.drawText (msx, juce::Rectangle<float> (led.x + 16.0f, r.getY(), fin - led.x - 16.0f, r.getHeight()), juce::Justification::centredLeft, true);
    g.drawText ("Xestor " + juce::String (JUCE_APPLICATION_VERSION_STRING), r.withTrimmedRight ((float) m), juce::Justification::centredRight, false);
}

void Principal::paintBaleiro (juce::Graphics& g)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, juce::jmin (520.0f, (float) getWidth() - 60.0f), 170.0f).withCentre (corpo.getCentre().toFloat().translated (0.0f, -30.0f));
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
    // o mando da familia, grande e tenue, e debaixo o texto
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.setOpacity (modelo.cargando ? 0.55f + 0.25f * std::sin (pulso * 2.0f) : 0.6f);
        paintMandoFamilia (g, juce::Rectangle<float> (r.getCentreX() - 70.0f, r.getY() - 90.0f, 140.0f, 140.0f), false);
    }
    g.setColour (col::crema);
    g.setFont (cond (css (20.0f)));
    g.drawText (t, r.withTrimmedTop (64.0f).withHeight (30.0f), juce::Justification::centred, false);
    g.setColour (col::crema2);
    g.setFont (body (css (13.5f)));
    g.drawFittedText (d, r.reduced (20.0f, 0.0f).withTrimmedTop (100.0f).withHeight (44.0f).toNearestInt(), juce::Justification::centredTop, 2, 1.0f);
    juce::ignoreUnused (c);
}

void Principal::paint (juce::Graphics& g)
{
    paintFondo (g, getLocalBounds().toFloat());
    paintCabeceira (g);
    if (baleiro()) paintBaleiro (g);
    paintPe (g);
}

// ==================================================================== teclado e paneis
void Principal::globalFocusChanged (juce::Component* c)
{
    // ao chegar co Tab (ou coas frechas) a unha tarxeta que non se ve, a lista desprázase ata ela
    if (c == nullptr || ! lista.isParentOf (c)) return;
    const auto r = lista.getLocalArea (c, c->getLocalBounds()).expanded (0, 8);
    const auto v = vista.getViewArea();
    if (r.getY() < v.getY()) vista.setViewPosition (0, juce::jmax (0, r.getY()));
    else if (r.getBottom() > v.getBottom()) vista.setViewPosition (0, r.getBottom() - v.getHeight());
}

void Principal::escena (const juce::String& e)
{
    const Produto* p = nullptr;
    for (auto& x : modelo.produtos())
        if (x.ultima && (p == nullptr || modelo.situacion (x).estado() == Estado::actualizacion)) p = &x;
    if (e == "demo")
    {
        // coma na maqueta: o rato enriba de Xuno
        if (auto* t = lista.tarxeta ("Barullo Xuno")) t->porSobre (true);
    }
    else if ((e == "ficha" || e == "captura" || e == "aberta") && p != nullptr)
    {
        const Produto* f = p;
        for (auto& x : modelo.produtos())
        {
            if (e == "ficha" && x.nome == "Barullo Xuno") f = &x;
            if (e == "captura" && x.capturas.size() > 1 && f == p) f = &x;
            if (e == "aberta" && x.eSerie()) f = &x;
        }
        ficha.abrir (*f);
        ficha.porNotas (modelo.notasXa (*f));
    }
    else if (e == "notas" && p != nullptr)
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

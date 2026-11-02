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
} // namespace

// ==================================================================== a interface en perspectiva
namespace
{
const juce::Colour fondoNoite { 0xff070a14 };

// Unha sombra (ou un brillo) baixo un camiño, en píxeles físicos
struct Sombra { juce::Colour cor; float radio; juce::Point<float> desprazamento; };

// A interface dun plugin en perspectiva, coas súas sombras e un fío de luz no bordo, nunha imaxe
// (en píxeles físicos). «caixa» devolve onde vai, en puntos e relativa ao centro.
juce::Image montarCapa (const juce::Image& src, float w, float h, const Perspectiva& p, float escala, float radio,
                        const std::vector<Sombra>& sombras, float bordo, juce::Rectangle<float>& caixa)
{
    juce::Rectangle<float> ci;
    auto im = deformar (src, w, h, p, escala, radio, ci);
    if (! im.isValid()) return {};
    float marxe = 0.0f;
    for (auto& s : sombras) marxe = juce::jmax (marxe, s.radio * 1.2f + juce::jmax (std::abs (s.desprazamento.x), std::abs (s.desprazamento.y)));
    caixa = ci.expanded (marxe);
    caixa = juce::Rectangle<float> (std::floor (caixa.getX() * escala) / escala, std::floor (caixa.getY() * escala) / escala,
                                    std::ceil (caixa.getWidth() * escala) / escala, std::ceil (caixa.getHeight() * escala) / escala);
    juce::Image out (juce::Image::ARGB, juce::jmax (1, juce::roundToInt (caixa.getWidth() * escala)), juce::jmax (1, juce::roundToInt (caixa.getHeight() * escala)), true);
    juce::Graphics g (out);
    const auto q = esquinas (w, h, p);
    juce::Path quad;
    auto fis = [&] (juce::Point<float> pt) { return (pt - caixa.getPosition()) * escala; };
    quad.startNewSubPath (fis (q[0]));
    for (int i = 1; i < 4; ++i) quad.lineTo (fis (q[(size_t) i]));
    quad.closeSubPath();
    for (auto& s : sombras)
        juce::DropShadow (s.cor, juce::jmax (1, juce::roundToInt (s.radio * escala)),
                          { juce::roundToInt (s.desprazamento.x * escala), juce::roundToInt (s.desprazamento.y * escala) }).drawForPath (g, quad);
    g.setOpacity (1.0f);
    g.drawImageAt (im, juce::roundToInt ((ci.getX() - caixa.getX()) * escala), juce::roundToInt ((ci.getY() - caixa.getY()) * escala));
    if (bordo > 0.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (bordo));
        g.strokePath (quad, juce::PathStrokeType (escala));
    }
    return out;
}

// unha «captura» para os que non a teñen: o degradado e o nome grande
juce::Image capturaFinxida (const Produto& p, int w, int h)
{
    juce::Image im (juce::Image::ARGB, w, h, true);
    juce::Graphics g (im);
    paintSenImaxe (g, p, { 0.0f, 0.0f, (float) w, (float) h }, (float) h * 0.3f);
    return im;
}

float escalaDe (juce::Graphics& g) { return juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor()); }

void paintLedEstado (juce::Graphics& g, juce::Point<float> c, juce::Colour cor, float r = 5.0f)
{
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (cor.withAlpha (0.10f * (float) (4 - i)));
        g.fillEllipse (c.x - r - (float) i * 2.5f, c.y - r - (float) i * 2.5f, 2.0f * (r + (float) i * 2.5f), 2.0f * (r + (float) i * 2.5f));
    }
    g.setColour (cor);
    g.fillEllipse (c.x - r, c.y - r, 2.0f * r, 2.0f * r);
}

// «v0.2.0 · ao día» ou «0.1.0 → 0.1.1» (a nova na cor do plugin)
std::vector<Anaco> anacosCapa (Modelo& modelo, const Produto& p)
{
    const auto s = modelo.situacion (p);
    const auto e = s.estado();
    const auto ult = p.ultima ? p.ultima->version() : std::nullopt;
    const auto ins = s.versionInstalada();
    if (e == Estado::actualizacion && ins && ult)
        return { { ins->texto(), col::crema2 }, { U (" → "), col::crema3 }, { ult->texto(), acento (p.nome) } };
    if (e == Estado::soLocal)
        return { { p.versionLocal ? tr ("local") + " " + p.versionLocal->texto() : tr ("Aínda sen publicar"), col::crema3 } };
    std::vector<Anaco> r;
    const auto vv = e == Estado::copiaVella ? ult : (ins ? ins : ult);   // (a copia vella é a do sistema)
    if (vv) r.push_back ({ "v" + vv->texto() + U (" · "), col::crema3 });
    auto est = textoPastilla (e).toLowerCase();
    if (e == Estado::incompleto)
        est = juce::String ((int) p.plugins.size() - s.faltan().size()) + "/" + juce::String (p.plugins.size());
    r.push_back ({ est, e == Estado::copiaVella ? col::ouro : col::crema3 });
    return r;
}
} // namespace

// ==================================================================== a capa dun produto
Tarxeta::Tarxeta (Modelo& m, std::function<void (const Produto&)> af)
    : modelo (m), abrirFicha (std::move (af))
{
    accion.getProperties().set ("fontSize", 12.0);
    accion.onClick = [this] { facerAccion (modelo, this, produto); };
    mais.getProperties().set ("icona", "mais");
    mais.getProperties().set ("redonda", true);
    mais.setTitle (tr ("Máis"));
    mais.setTooltip (tr ("Notas da versión, volver atrás, manual, presets, reinstalar, desinstalar…"));
    mais.onClick = [this] { menuProduto (modelo, &mais, produto, abrirFicha); };
    addAndMakeVisible (accion);
    addChildComponent (mais);
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    accion.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    mais.setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void Tarxeta::mostrar (const Produto& p)
{
    if (p.id() != produto.id()) anchoCache = 0;
    produto = p;
    const auto s = modelo.situacion (p);
    const auto e = s.estado();
    const auto luz = acento (p.nome);
    const bool traballa = modelo.traballando (p);
    if (traballaba && ! traballa && modelo.erro.isEmpty()) visto = 1.0f;   // o ✓ ao rematar
    traballaba = traballa;
    for (auto* b : { &accion, &mais }) b->getProperties().set ("led", (juce::int64) luz.getARGB());
    estiloAccion (accion, e);
    if (e == Estado::soLocal || e == Estado::senDatos) { accion.getProperties().remove ("estilo"); accion.getProperties().set ("estilo", "escura"); }
    accion.setButtonText (textoAccion (e));
    accion.setTooltip (axudaAccion (s));
    accion.setDescription (textoAccion (e) + " " + p.nome);
    accion.setVisible (textoAccion (e).isNotEmpty() && ! traballa);
    mais.setDescription (tr ("Máis opcións de %1", p.nome));
    setTitle (p.nome);
    setDescription (p.descricion + ". " + textoPastilla (e));
    setHelpText (tr ("Intro: abrir a ficha"));
    resized();
    repaint();
}

int Tarxeta::alto (int ancho) { return ancho + 58; }

float Tarxeta::alzado() const { return -4.0f * sobre + 16.0f * (1.0f - suave (aparicion)); }

juce::Rectangle<float> Tarxeta::capa() const
{
    const float a = (float) (getWidth() - 2 * marxe);
    return juce::Rectangle<float> ((float) marxe, (float) marxe + alzado(), a, a);
}

void Tarxeta::resized()
{
    const auto c = capa().toNearestInt();
    const int aw = Look::anchoTecla (accion, 26);
    accion.setBounds (c.getRight() - 12 - aw, c.getBottom() - 12 - 30, aw, 30);
    mais.setBounds (c.getX() + 12, c.getBottom() - 12 - 30, 30, 30);
    const bool ve = (sobre > 0.4f || hasKeyboardFocus (true)) && ! modelo.traballando (produto);
    if (mais.isVisible() != ve) mais.setVisible (ve);
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
        sobre += (obx - sobre) * juce::jmin (1.0f, dt * 10.0f);
        if (std::abs (sobre - obx) < 0.003f) sobre = obx;
        cambio = true;
    }
    if (aparicion < 1.0f)
    {
        if (atraso > 0.0f) atraso -= dt;
        else aparicion = juce::jmin (1.0f, aparicion + dt / 0.5f);
        setAlpha (suave (aparicion * 1.4f));
        cambio = true;
    }
    if (visto > 0.0f) { visto = juce::jmax (0.0f, visto - dt / 2.4f); cambio = true; }
    if (cambio) resized();
    if (cambio || modelo.traballando (produto)) repaint();
    return cambio;
}

void Tarxeta::paint (juce::Graphics& g)
{
    const auto c = capa();
    const float k = c.getWidth() / 250.0f;   // as medidas da maqueta van para 250 px
    const auto s = modelo.situacion (produto);
    const auto e = s.estado();
    const auto luz = acento (produto.nome);
    const float rad = 18.0f;
    const bool traballa = modelo.traballando (produto);

    // a sombra e, co rato enriba, o brillo da cor
    paintSombra (g, c, rad, juce::Colours::black.withAlpha (0.5f), 26.0f, 16.0f);
    if (sobre > 0.01f) paintSombra (g, c, rad, luz.withAlpha (0.28f * sobre), 30.0f, 4.0f);

    juce::Path forma;
    forma.addRoundedRectangle (c, rad);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (forma);
        // o fondo: un degradado tinguido e un brillo da cor abaixo
        g.setGradientFill (juce::ColourGradient (luz.interpolatedWith (juce::Colour (0xff10142a), 0.72f), c.getX() + c.getWidth() * 0.2f, c.getY(),
                                                 juce::Colour (0xff0b0f1f), c.getRight() - c.getWidth() * 0.15f, c.getBottom(), false));
        g.fillRect (c);
        {
            juce::Graphics::ScopedSaveState s2 (g);
            const auto ctr = juce::Point<float> (c.getCentreX(), c.getY() + c.getHeight() * 1.1f);
            const float rx = 260.0f * k * (1.0f + 0.12f * sobre), ry = 200.0f * k * (1.0f + 0.12f * sobre);
            g.addTransform (juce::AffineTransform::scale (1.0f, ry / rx, ctr.x, ctr.y));
            juce::ColourGradient gl (luz.withAlpha (0.95f), ctr.x, ctr.y, luz.withAlpha (0.0f), ctr.x + rx, ctr.y, true);
            gl.addColour (0.35, luz.withAlpha (0.5f));
            gl.addColour (0.7, luz.withAlpha (0.0f));
            g.setGradientFill (gl);
            g.fillRect (juce::Rectangle<float> (ctr.x - rx, ctr.y - rx, rx * 2.0f, rx * 2.0f));
        }
        // a interface inclinada (gardada ao seu tamaño e escala)
        const float esc = escalaDe (g);
        auto mini = modelo.miniatura (produto);
        const bool conImaxe = mini.isValid();
        const float iw = 250.0f * k;
        if (anchoCache != (int) c.getWidth() || ! juce::approximatelyEqual (escalaCache, esc) || tinhaImaxe != conImaxe || ! deformada.isValid())
        {
            const auto fonte = conImaxe ? mini : capturaFinxida (produto, 500, 320);
            const float ih = iw * (float) fonte.getHeight() / (float) fonte.getWidth();
            deformada = montarCapa (fonte, iw, ih, { -18.0f, 6.0f, 0.0f, 700.0f * k }, esc, 8.0f * k,
                                    { { juce::Colours::black.withAlpha (0.67f), 40.0f * k, { -20.0f * k, 30.0f * k } } }, 0.09f, caixaDeformada);
            anchoCache = (int) c.getWidth();
            escalaCache = esc;
            tinhaImaxe = conImaxe;
        }
        if (deformada.isValid())
        {
            const float ih = conImaxe ? iw * (float) mini.getHeight() / (float) mini.getWidth() : iw * 0.64f;
            const auto centro = juce::Point<float> (c.getX() + 30.0f * k + iw * 0.5f, c.getY() + 44.0f * k + ih * 0.5f);
            // co rato: un chisco máis cerca e cara á esquerda, coma se se endereitase
            const float z = 1.0f + 0.035f * sobre;
            const auto t = juce::AffineTransform::scale (1.0f / esc)
                               .translated (centro.x + caixaDeformada.getX(), centro.y + caixaDeformada.getY())
                               .scaled (z, z, centro.x, centro.y)
                               .translated (-6.0f * k * sobre, -3.0f * k * sobre);
            g.setOpacity (1.0f);
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImageTransformed (deformada, t);
        }
        // abaixo, para que o nome e as teclas se lean
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.0f), 0.0f, c.getBottom() - 70.0f * k,
                                                 juce::Colours::black.withAlpha (accion.isVisible() || traballa ? 0.35f : 0.0f), 0.0f, c.getBottom(), false));
        g.fillRect (c.withTop (c.getBottom() - 70.0f * k));
        // o nome curto arriba á esquerda (cun degradado para que se lea sobre a interface)
        g.setGradientFill (juce::ColourGradient (luz.interpolatedWith (juce::Colour (0xff10142a), 0.72f).withAlpha (0.7f), 0.0f, c.getY(),
                                                 juce::Colours::transparentBlack, 0.0f, c.getY() + 46.0f * k, false));
        g.fillRect (c.withHeight (46.0f * k));
        g.setColour (col::crema.withAlpha (0.82f));
        g.setFont (cond (css (13.0f)).withExtraKerningFactor (0.2f));
        g.drawText (upper (produto.nomeCurto()), juce::Rectangle<float> (c.getX() + 18.0f, c.getY() + 13.0f, c.getWidth() - 56.0f, 22.0f),
                    juce::Justification::centredLeft, true);
    }
    g.setColour (sobre > 0.01f ? luz.withAlpha (0.08f + 0.4f * sobre) : juce::Colours::white.withAlpha (0.063f));
    g.drawRoundedRectangle (c.reduced (0.5f), rad, 1.0f);
    if (hasKeyboardFocus (false))
    {
        g.setColour (luz.withAlpha (0.95f));
        g.drawRoundedRectangle (c.expanded (2.0f), rad + 2.0f, 2.0f);
    }

    // o LED do estado (ou o ✓ ao rematar)
    if (visto > 0.0f)
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.setOpacity (juce::jmin (1.0f, visto * 3.0f));
        const auto r = juce::Rectangle<float> (c.getRight() - 40.0f, c.getY() + 8.0f, 30.0f, 30.0f);
        g.setColour (col::verde.withAlpha (0.2f));
        g.fillEllipse (r);
        paintVisto (g, r.reduced (7.0f), col::verde, 2.0f);
    }
    else paintLedEstado (g, { c.getRight() - 17.0f, c.getY() + 17.0f }, corEstado (e));

    // o progreso, abaixo na capa
    if (traballa)
    {
        const auto barra = juce::Rectangle<float> (c.getX() + 12.0f, c.getBottom() - 42.0f, c.getWidth() - 24.0f, 30.0f);
        g.setColour (juce::Colour (0xe60b0f1f));
        g.fillRoundedRectangle (barra, 10.0f);
        paintProgreso (g, modelo, produto, barra);
    }

    // debaixo: o nome de verdade e a versión
    g.setColour (col::crema);
    g.setFont (cond (css (15.0f)));
    g.drawText (produto.nome, juce::Rectangle<float> (c.getX(), c.getBottom() + 12.0f, c.getWidth(), 22.0f), juce::Justification::centredLeft, true);
    paintAnacos (g, anacosCapa (modelo, produto), { c.getX(), c.getBottom() + 36.0f, c.getWidth(), 18.0f }, body (css (12.5f)));
}

void Tarxeta::mouseUp (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { menuProduto (modelo, this, produto, abrirFicha); return; }
    if (! e.mouseWasClicked()) return;
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

// ==================================================================== o heroe
Heroe::Heroe (Modelo& m, std::function<void (const Produto&)> af) : modelo (m), abrirFicha (std::move (af))
{
    principal.getProperties().set ("primaria", true);
    principal.getProperties().set ("fontSize", 14.0);
    principal.getProperties().set ("grande", true);
    for (auto* b : { &novidades, &ficha })
    {
        b->getProperties().set ("estilo", "escura");
        b->getProperties().set ("fontSize", 14.0);
    }
    novidades.setButtonText (tr ("Ver novidades"));
    ficha.setButtonText (tr ("Abrir ficha"));
    principal.onClick = [this]
    {
        if (auto* p = produto (actual))
        {
            const auto e = modelo.situacion (*p).estado();
            if (cambiaPlugins (e)) facerAccion (modelo, this, *p);
            else if (abrirFicha) abrirFicha (*p);
        }
    };
    novidades.onClick = [this] { if (auto* p = produto (actual)) paneis::notas (modelo, this, *p); };
    ficha.onClick = [this] { if (auto* p = produto (actual)) if (abrirFicha) abrirFicha (*p); };
    for (auto* b : { &principal, &novidades, &ficha }) addAndMakeVisible (b);
    setWantsKeyboardFocus (true);
    setTitle (tr ("Destacados"));
    setHelpText (tr ("← →: o destacado anterior ou o seguinte"));
}

int Heroe::alto (int ancho) { return ancho >= 1000 ? 500 : 426; }

const Produto* Heroe::produto (int i) const
{
    if (i < 0 || i >= slides.size()) return nullptr;
    for (auto& p : modelo.produtos())
        if (p.id() == slides[i]) return &p;
    return nullptr;
}

void Heroe::actualizar()
{
    // primeiro os que se actualizan, despois os que non están instalados e despois os últimos publicados
    juce::StringArray novos;
    std::vector<const Produto*> resto;
    for (auto& p : modelo.produtos())
    {
        const auto e = modelo.situacion (p).estado();
        if (e == Estado::actualizacion || e == Estado::incompleto) novos.add (p.id());
    }
    for (auto& p : modelo.produtos())
        if (modelo.situacion (p).estado() == Estado::nonInstalado) novos.add (p.id());
    for (auto& p : modelo.produtos())
        if (p.ultima && ! novos.contains (p.id())) resto.push_back (&p);
    std::stable_sort (resto.begin(), resto.end(), [] (const Produto* a, const Produto* b) { return a->ultima->publicado > b->ultima->publicado; });
    for (auto* p : resto)
    {
        if (novos.size() >= 3) break;
        novos.add (p->id());
    }
    while (novos.size() > 5) novos.remove (novos.size() - 1);
    if (novos != slides)
    {
        const auto antes = slides[actual];
        slides = novos;
        actual = juce::jmax (0, slides.indexOf (antes));
        anterior = -1;
        mestura = 1.0f;
    }
    textos();
}

void Heroe::textos()
{
    auto* p = produto (actual);
    if (p == nullptr) return;
    const auto s = modelo.situacion (*p);
    const auto e = s.estado();
    const auto luz = acento (p->nome);
    juce::String tam;
    if (p->ultima)
        if (auto* a = p->ultima->arquivo(); a != nullptr && a->tamano > 0) tam = tamano (a->tamano);
    juce::String t;
    if (e == Estado::actualizacion || e == Estado::incompleto) t = tam.isNotEmpty() ? tr ("Actualizar · %1", tam) : tr ("Actualizar");
    else if (e == Estado::nonInstalado) t = tam.isNotEmpty() ? tr ("Instalar · %1", tam) : tr ("Instalar");
    else if (e == Estado::copiaVella) t = tr ("Quitar a vella");
    else t = tr ("Abrir ficha");
    if (modelo.traballando (*p))
    {
        const double v = modelo.traballos.at (p->id());
        t = maiuscula1 (tr (modelo.fases.count (p->id()) ? modelo.fases.at (p->id()) : juce::String ("traballando")))
            + (v >= 0.0 && v < 1.0 ? U (" · ") + juce::String (juce::roundToInt (v * 100.0)) + " %" : juce::String (U ("…")));
    }
    principal.setButtonText (t);
    principal.setTooltip (cambiaPlugins (e) ? axudaAccion (s) : juce::String());
    principal.setEnabled (! modelo.traballando (*p));
    novidades.setVisible (p->ultima.has_value());
    ficha.setVisible (cambiaPlugins (e));
    for (auto* b : { &principal, &novidades, &ficha }) b->getProperties().set ("led", (juce::int64) luz.getARGB());
    setDescription (p->nome + ". " + p->descricion);
    resized();
    repaint();
}

void Heroe::ir (int i)
{
    if (slides.size() < 2) return;
    i = (i + slides.size()) % slides.size();
    if (i == actual) return;
    anterior = actual;
    actual = i;
    mestura = 0.0f;
    tempo = 0.0f;
    repaint();
}

bool Heroe::animar (float dt)
{
    bool cambio = false;
    if (! isMouseOver (true) && ! hasKeyboardFocus (true)) tempo += dt;
    if (tempo > 7.0f && slides.size() > 1) ir (actual + 1);
    if (mestura < 1.0f)
    {
        const bool antes = mestura < 0.5f;
        mestura = juce::jmin (1.0f, mestura + dt / 0.7f);
        if (antes && mestura >= 0.5f) textos();
        const float a = mestura < 0.5f ? 1.0f - 2.0f * mestura : 2.0f * mestura - 1.0f;
        for (auto* b : { &principal, &novidades, &ficha }) b->setAlpha (a);
        cambio = true;
    }
    if (auto* p = produto (actual); p != nullptr && modelo.traballando (*p)) { textos(); cambio = true; }
    if (cambio) repaint();
    return cambio;
}

juce::Image Heroe::arte (int i, float escala)
{
    auto* p = produto (i);
    if (p == nullptr) return {};
    auto& a = artes[p->id()];
    const int W = getWidth(), H = getHeight();
    if (a.im.isValid() && a.w == W && a.h == H && juce::approximatelyEqual (a.escala, escala)) return a.im;
    a.w = W; a.h = H; a.escala = escala;
    const auto luz = acento (p->nome);
    juce::Image im (juce::Image::ARGB, juce::jmax (1, juce::roundToInt ((float) W * escala)), juce::jmax (1, juce::roundToInt ((float) H * escala)), true);
    {
        juce::Graphics g (im);
        g.addTransform (juce::AffineTransform::scale (escala));
        const auto r = juce::Rectangle<float> ((float) W, (float) H);
        g.setColour (juce::Colour (0xff0a0d1c));
        g.fillRect (r);
        auto brillo = [&] (juce::Point<float> c, float rx, float ry, juce::Colour cor)
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.addTransform (juce::AffineTransform::scale (1.0f, ry / rx, c.x, c.y));
            juce::ColourGradient gr (cor, c.x, c.y, cor.withAlpha (0.0f), c.x + rx, c.y, true);
            gr.addColour (0.35, cor.withMultipliedAlpha (0.6f));
            gr.addColour (0.7, cor.withAlpha (0.0f));
            g.setGradientFill (gr);
            g.fillRect (juce::Rectangle<float> (c.x - rx, c.y - rx, rx * 2.0f, rx * 2.0f));
        };
        brillo ({ r.getWidth() * 0.72f, r.getHeight() * 0.55f }, 700.0f, 420.0f, luz.withAlpha (0.33f));
        brillo ({ r.getWidth() * 0.9f, 0.0f }, 900.0f, 500.0f, juce::Colour (0xff3a2c6a).interpolatedWith (luz.withMultipliedBrightness (0.45f), 0.4f).withAlpha (0.4f));

        // a interface en perspectiva, á dereita (ou detrás, estreita)
        juce::Image fonte;
        if (! p->capturas.isEmpty())
        {
            const auto url = p->capturas[0];
            fonte = modelo.capturaSeHai (url);
            if (! fonte.isValid() && modelo.escenaActual.isNotEmpty()) fonte = modelo.captura (url);   // (a captura: sen fíos)
            if (! fonte.isValid() && ! pedidas.contains (url))
            {
                // a enteira, nun fío; mentres, a miniatura
                pedidas.add (url);
                juce::Component::SafePointer<Heroe> sp (this);
                const auto id = p->id();
                modelo.pedirCaptura (url, [sp, id] (juce::Image)
                {
                    if (sp == nullptr) return;
                    sp->artes.erase (id);
                    sp->repaint();
                });
            }
        }
        if (! fonte.isValid()) fonte = modelo.miniatura (*p);
        if (! fonte.isValid()) fonte = capturaFinxida (*p, 900, 540);
        const bool estr = W < 1000;
        const float sw = estr ? (float) W * 0.95f : juce::jlimit (640.0f, 1100.0f, 900.0f * (float) W / 1440.0f);
        const float sh = sw * (float) fonte.getHeight() / (float) fonte.getWidth();
        const float sx = estr ? (float) W - sw * 0.72f : (float) W + 60.0f * (float) W / 1440.0f - sw;
        const float sy = estr ? 96.0f : 70.0f;
        juce::Rectangle<float> caixa;
        auto capa = montarCapa (fonte, sw, sh, { -24.0f, 8.0f, 1.0f, 1600.0f }, escala, 14.0f,
                                { { juce::Colours::black.withAlpha (0.8f), 120.0f, { 0.0f, 60.0f } }, { luz.withAlpha (0.27f), 120.0f, {} } }, 0.08f, caixa);
        if (capa.isValid())
        {
            g.setOpacity (1.0f);
            g.drawImage (capa, caixa.translated (sx + sw * 0.5f, sy + sh * 0.5f), juce::RectanglePlacement::stretchToFit);
        }
        // o fundido: á esquerda (onde vai o texto) e abaixo, ata o fondo da xanela
        juce::ColourGradient esq (fondoNoite, 0.0f, 0.0f, fondoNoite.withAlpha (0.0f), r.getWidth() * (estr ? 1.05f : 0.6f), 0.0f, false);
        esq.addColour (estr ? 0.5 : 0.28 / 0.6, fondoNoite.withAlpha (estr ? 0.88f : 0.93f));
        g.setGradientFill (esq);
        g.fillRect (r);
        g.setGradientFill (juce::ColourGradient (fondoNoite, 0.0f, r.getBottom(), fondoNoite.withAlpha (0.0f), 0.0f, r.getBottom() - r.getHeight() * (estr ? 0.45f : 0.35f), false));
        g.fillRect (r);
    }
    a.im = im;
    return im;
}

juce::Rectangle<float> Heroe::zonaPuntos (int i) const
{
    float x = estreito() ? 20.0f : 44.0f;
    for (int j = 0; j < i; ++j) x += (j == actual ? 40.0f : 22.0f) + 8.0f;
    return { x, (float) getHeight() - (estreito() ? 30.0f : 48.0f), i == actual ? 40.0f : 22.0f, 4.0f };
}

void Heroe::resized()
{
    const bool e = estreito();
    const int x0 = e ? 20 : 44;
    // o nome pode ser máis pequeno se non colle; as teclas, debaixo da descrición
    const int yb = e ? 312 : 349;
    int x = x0;
    for (auto* b : { &principal, &novidades, &ficha })
    {
        if (! b->isVisible()) continue;
        const int w = Look::anchoTecla (*b, 48);
        if (x + w > getWidth() - x0) { b->setBounds (0, 0, 0, 0); continue; }
        b->setBounds (x, yb, w, 50);
        x += w + 12;
    }
}

void Heroe::paint (juce::Graphics& g)
{
    const float esc = escalaDe (g);
    g.setOpacity (1.0f);
    if (anterior >= 0 && mestura < 1.0f)
    {
        auto a = arte (anterior, esc);
        if (a.isValid()) g.drawImage (a, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    }
    auto im = arte (actual, esc);
    if (im.isValid())
    {
        g.setOpacity (anterior >= 0 ? suave (mestura) : 1.0f);
        g.drawImage (im, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
        g.setOpacity (1.0f);
    }
    // o texto do que se ve (na transición, o de antes desaparece e aparece o novo)
    const auto* p = produto (mestura < 0.5f && anterior >= 0 ? anterior : actual);
    if (p == nullptr) return;
    const float alfa = anterior >= 0 && mestura < 1.0f ? (mestura < 0.5f ? 1.0f - 2.0f * mestura : 2.0f * mestura - 1.0f) : 1.0f;
    const auto luz = acento (p->nome);
    const auto e = modelo.situacion (*p).estado();
    const bool es = estreito();
    const float x0 = es ? 20.0f : 44.0f, ancho = juce::jmin (es ? (float) getWidth() - 40.0f : 640.0f, (float) getWidth() - 2.0f * x0);
    juce::Graphics::ScopedSaveState ss (g);
    g.setOpacity (alfa);
    // a etiqueta: «● Novidade · 0.1.1»
    juce::String et;
    const auto v = p->ultima ? texto (p->ultima->version()) : juce::String();
    if (e == Estado::actualizacion || e == Estado::incompleto) et = tr ("Novidade") + U (" · ") + v;
    else if (e == Estado::nonInstalado) et = tr ("Sen instalar") + U (" · ") + v;
    else if (e == Estado::copiaVella) et = tr ("Copia vella");
    else et = tr ("Ao día") + (v.isNotEmpty() ? U (" · ") + v : juce::String());
    et = U ("● ") + upper (et);
    const auto fe = semi (css (11.0f)).withExtraKerningFactor (0.18f);
    const float yb = es ? 128.0f : 150.0f;
    auto badge = juce::Rectangle<float> (x0, yb, anchoTexto (fe, et) * 1.04f + 26.0f, 32.0f);
    g.setColour (luz.withAlpha (0.10f));
    g.fillRoundedRectangle (badge, 16.0f);
    g.setColour (luz.withAlpha (0.27f));
    g.drawRoundedRectangle (badge.reduced (0.5f), 16.0f, 1.0f);
    g.setColour (luz);
    g.setFont (fe);
    g.drawText (et, badge.withTrimmedLeft (13.0f), juce::Justification::centredLeft, false);
    // o nome, enorme
    float tam = css (es ? 46.0f : 64.0f);
    while (tam > css (30.0f) && anchoTexto (cond (tam), p->nome) > (float) getWidth() - x0 - 20.0f) tam -= 2.0f;
    g.setColour (col::crema);
    g.setFont (cond (tam).withExtraKerningFactor (-0.01f));
    g.drawText (p->nome, juce::Rectangle<float> (x0 - 3.0f, badge.getBottom() + 14.0f, (float) getWidth(), tam * 1.05f), juce::Justification::centredLeft, false);
    // a descrición (ata dúas liñas)
    g.setColour (col::crema2);
    g.setFont (body (css (16.0f)));
    const float yd = badge.getBottom() + 14.0f + tam * 1.05f + 8.0f;
    g.drawFittedText (p->descricion, juce::Rectangle<float> (x0, yd, ancho, 58.0f).toNearestInt(), juce::Justification::topLeft, 2, 1.0f);
    // os puntos (liñas): o actual, máis longo e laranxa
    for (int i = 0; i < slides.size() && slides.size() > 1; ++i)
    {
        g.setColour (i == actual ? col::laranxa : juce::Colours::white.withAlpha (0.14f));
        g.fillRoundedRectangle (zonaPuntos (i), 2.0f);
    }
    if (hasKeyboardFocus (false))
    {
        g.setColour (luz.withAlpha (0.8f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (3.0f), 12.0f, 1.6f);
    }
}

void Heroe::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasClicked()) return;
    for (int i = 0; i < slides.size(); ++i)
        if (zonaPuntos (i).expanded (4.0f, 10.0f).contains (e.position)) { ir (i); return; }
    if (e.position.x > (float) getWidth() * 0.55f)
        if (auto* p = produto (actual)) if (abrirFicha) abrirFicha (*p);
}

bool Heroe::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::leftKey) { ir (actual - 1); return true; }
    if (k == juce::KeyPress::rightKey) { ir (actual + 1); return true; }
    if (k == juce::KeyPress::returnKey) { if (auto* p = produto (actual)) if (abrirFicha) abrirFicha (*p); return true; }
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
Lista::Lista (Modelo& m, std::function<void (const Produto&)> af) : modelo (m), abrirFicha (std::move (af)), heroe (m, abrirFicha), destacado (m)
{
    addChildComponent (heroe);
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
    if (destacado.hai()) destacado.actualizar();
    heroe.actualizar();
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
    const int m = ancho >= 1000 ? 44 : 20, gap = 22, M = Tarxeta::marxe;
    const int cols = juce::jlimit (1, 6, (ancho - 2 * m + gap) / (220 + gap));
    const int cw = (ancho - 2 * m - (cols - 1) * gap) / cols;
    const int ch = Tarxeta::alto (cw);
    titulos.clear();
    const auto& ps = modelo.produtos();
    auto casa = [this] (const Produto& p)
    {
        if (! casaBusca (p, busca)) return false;
        switch (seccion)
        {
            case 1: return casaPestana (p, Pestana::instrumentos);
            case 2: return casaPestana (p, Pestana::efectos);
            case 3: return casaPestana (p, Pestana::paquetes);
            case 4: return pideActualizar (modelo.situacion (p).estado()) || modelo.traballando (p);
            default: return true;
        }
    };
    int instalados = 0, porActualizar = 0;
    for (auto& p : ps)
        if (casa (p))
        {
            instalados += modelo.situacion (p).relevantes().empty() ? 0 : 1;
            porActualizar += pideActualizar (modelo.situacion (p).estado()) ? 1 : 0;
        }

    int y = 0;
    // o inicio: o heroe; nas pestanas (e buscando), un título grande debaixo da barra
    heroe.setVisible (inicio() && heroe.hai());
    if (heroe.isVisible())
    {
        heroe.setBounds (0, 0, ancho, Heroe::alto (ancho));
        y = heroe.getBottom() + 2;
    }
    else y = ancho >= 980 ? 96 : 128;

    // o aviso: un erro (que se pecha), agardando polo DAW, «volve escanear» ou «sen conexión»
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
        y = faixa.getBottom() + 26;
    }
    else faixa = {};

    // o título: «A túa colección» (ou a pestana, ou a busca) e o resumo en maiúsculas
    {
        juce::String t, r;
        if (busca.trim().isNotEmpty()) { t = U ("«") + busca.trim() + U ("»"); }
        else if (seccion == 0) t = tr ("A túa colección");
        else if (seccion == 4) t = tr ("Actualizacións");
        else t = tr (titulo (seccion == 1 ? TipoProduto::instrumento : seccion == 2 ? TipoProduto::efecto : TipoProduto::paquete));
        r = tr ("%1 instalados", juce::String (instalados)) + U (" · ")
            + (porActualizar > 0 ? tr ("%1 por actualizar", juce::String (porActualizar)) : tr ("todo ao día"));
        const bool grande = ! inicio();
        titulos.push_back ({ t, { m, y, ancho - 2 * m, grande ? 44 : 28 }, -1, grande, r });
        y += (grande ? 44 : 28) + 18;
    }
    destacado.setVisible (seccion == 4 && busca.trim().isEmpty() && destacado.hai());
    if (destacado.isVisible())
    {
        destacado.setBounds (m, y, ancho - 2 * m, destacado.alto (ancho - 2 * m));
        y = destacado.getBottom() + 26;
    }

    // as capas: no inicio e buscando, por seccións; nas pestanas, todas xuntas
    auto tipoDe = [this] (const Produto& p) { return modelo.situacion (p).estado() == Estado::soLocal ? TipoProduto::desconecido : p.tipo; };
    const bool porSeccions = seccion == 0;
    for (auto tp : { TipoProduto::instrumento, TipoProduto::efecto, TipoProduto::paquete, TipoProduto::desconecido })
    {
        std::vector<Tarxeta*> estas;
        for (auto& t : tarxetas)
        {
            const auto& p = t->getProduto();
            if (tipoDe (p) != tp) continue;
            const bool ve = casa (p);
            t->setVisible (ve);
            if (ve) estas.push_back (t.get());
        }
        if (estas.empty()) continue;
        if (porSeccions)
        {
            titulos.push_back ({ tp == TipoProduto::desconecido ? tr ("Só neste ordenador (sen publicar)") : tr (titulo (tp)), { m, y, ancho - 2 * m, 22 }, (int) estas.size(), false, {} });
            y += 22 + 12;
        }
        for (size_t i = 0; i < estas.size(); ++i)
        {
            const int c = (int) i % cols;
            if (i > 0 && c == 0) y += ch + gap;
            estas[i]->setBounds (m + c * (cw + gap) - M, y - M, cw + 2 * M, ch + 2 * M);
        }
        if (porSeccions) y += ch + 34;
        else
        {
            // (todas seguidas: a seguinte sección continúa a reixa)
            y += ch + gap;
        }
    }
    if (! porSeccions)
    {
        // reordenar en reixa continua as visibles de todas as seccións
        std::vector<Tarxeta*> vs;
        for (auto& t : tarxetas) if (t->isVisible()) vs.push_back (t.get());
        int yy = titulos.empty() ? y : (destacado.isVisible() ? destacado.getBottom() + 26 : titulos.back().r.getBottom() + 18);
        for (size_t i = 0; i < vs.size(); ++i)
        {
            const int c = (int) i % cols;
            if (i > 0 && c == 0) yy += ch + gap;
            vs[i]->setBounds (m + c * (cw + gap) - M, yy - M, cw + 2 * M, ch + 2 * M);
        }
        y = vs.empty() ? yy : yy + ch + 34;
    }
    const bool orfosVisibles = ! orfos.empty() && seccion == 0 && busca.trim().isEmpty();
    if (orfosVisibles)
    {
        titulos.push_back ({ tr ("Outros de Escoita neste ordenador"), { m, y, ancho - 2 * m, 22 }, (int) orfos.size(), false, {} });
        y += 22 + 14;
        for (auto& o : orfos) { o->setVisible (true); o->setBounds (m, y, ancho - 2 * m, 52); y += 60; }
        y += 20;
    }
    else for (auto& o : orfos) o->setVisible (false);

    baleiro = {};
    if (visibles() == 0 && ! tarxetas.empty())
    {
        baleiro = { m, y + 10, ancho - 2 * m, 80 };
        y = baleiro.getBottom() + 20;
    }
    setSize (ancho, y + 20);
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
        if (t.n < 0)
        {
            // o título da páxina: grande, co resumo en maiúsculas pequenas ao lado
            const auto f = cond (css (t.grande ? 32.0f : 20.0f));
            g.setColour (col::crema);
            g.setFont (f);
            g.drawText (t.texto, r, juce::Justification::centredLeft, true);
            const float w = anchoTexto (f, t.texto);
            g.setColour (col::crema3);
            drawCaps (g, t.resumo, r.withTrimmedLeft (w + 18.0f).translated (0.0f, t.grande ? 4.0f : 2.0f), juce::Justification::centredLeft, css (10.0f), 0.24f);
            continue;
        }
        g.setColour (col::crema2);
        g.setFont (cond (css (14.0f)));
        g.drawText (t.texto, r, juce::Justification::centredLeft, false);
        const float w = anchoTexto (cond (css (14.0f)), t.texto);
        g.setColour (col::crema3);
        drawCaps (g, juce::String (t.n), r.withTrimmedLeft (w + 10.0f).withWidth (40.0f).translated (0.0f, 1.5f), juce::Justification::centredLeft, css (10.0f), 0.22f);
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
    bool algo = animarHeroe (dt);
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
    manual.getProperties().set ("estilo", "escura");
    manual.getProperties().set ("fontSize", 14.0);
    manual.onClick = [this] { modelo.abrirManual (produto); };
    presets.setButtonText (tr ("Presets"));
    presets.getProperties().set ("estilo", "escura");
    presets.getProperties().set ("fontSize", 14.0);
    presets.setTooltip (tr ("Abrir o cartafol dos presets"));
    presets.onClick = [this] { for (auto& f : modelo.cartafolesPresets (produto)) f.revealToUser(); };
    mais.setButtonText (U ("⋯"));
    mais.getProperties().set ("estilo", "escura");
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
    notas.setFont (body (css (14.0f)));
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
    y += 56;
    // a descrición (ata tres liñas)
    {
        juce::AttributedString as;
        as.append (produto.descricion, body (css (16.0f)));
        juce::TextLayout tl;
        tl.createLayout (as, (float) r.getWidth());
        altoDesc = juce::jmin (3 * 26, juce::roundToInt (tl.getHeight()) + 2);
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
    as.append (notas.getText(), body (css (14.0f)));
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
        g.setColour (juce::Colour (0xff070a14));
        g.fillRect (e);
        {
            juce::Graphics::ScopedSaveState s2 (g);
            const auto ctr = juce::Point<float> (e.getCentreX(), e.getY() + e.getHeight() * 0.4f);
            g.addTransform (juce::AffineTransform::scale (1.0f, 400.0f / 600.0f, ctr.x, ctr.y));
            juce::ColourGradient halo (luz.withAlpha (0.42f), ctr.x, ctr.y, luz.withAlpha (0.0f), ctr.x + 600.0f, ctr.y, true);
            halo.addColour (0.35, luz.withAlpha (0.22f));
            halo.addColour (0.7, luz.withAlpha (0.0f));
            g.setGradientFill (halo);
            g.fillRect (e.expanded (0.0f, e.getHeight()));
        }
        // a captura en perspectiva sobre o halo (gardada para este índice, tamaño e escala)
        const auto zi = zonaImaxe();
        const float esc = escalaDe (g);
        auto im = imaxe (indice);
        const bool enteira = indice < produto.capturas.size() && modelo.capturaSeHai (produto.capturas[indice]).isValid();
        if (! deformada.isValid() || indiceDeformada != indice || ! juce::approximatelyEqual (escalaDeformada, esc) || zonaDeformada != zi || (enteira && ! deformadaBoa))
        {
            const auto fonte = im.isValid() ? im : capturaFinxida (produto, 900, 560);
            deformada = montarCapa (fonte, zi.getWidth() * 0.92f, zi.getWidth() * 0.92f * (float) fonte.getHeight() / (float) fonte.getWidth(),
                                    { -16.0f, 6.0f, 0.5f, 1800.0f }, esc, 12.0f,
                                    { { juce::Colours::black.withAlpha (0.75f), 70.0f, { 0.0f, 40.0f } }, { luz.withAlpha (0.3f), 90.0f, {} } }, 0.08f, caixaDeformada);
            indiceDeformada = indice;
            escalaDeformada = esc;
            zonaDeformada = zi;
            deformadaBoa = enteira;
        }
        if (deformada.isValid())
        {
            g.setOpacity (1.0f);
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImage (deformada, caixaDeformada.translated (zi.getCentreX(), zi.getCentreY()), juce::RectanglePlacement::stretchToFit);
        }
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
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff121831), 0.0f, d.getY(), juce::Colour (0xff0a0d1c), 0.0f, d.getBottom(), false));
        g.fillRect (d);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRect (d.withWidth (1.0f));
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
    g.setFont (cond (css (38.0f)));
    g.drawFittedText (produto.nome, juce::Rectangle<float> (r.getX(), (float) yNome, r.getWidth() - 46.0f, 50.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    g.setColour (col::crema2);
    g.setFont (body (css (16.0f)));
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
    // a lista vai detrás de todo (o heroe pasa por baixo da barra)
    vista.setViewedComponent (&lista, false);
    vista.setScrollBarsShown (true, false);
    vista.setScrollOnDragMode (juce::Viewport::ScrollOnDragMode::never);
    vista.getVerticalScrollBar().setAutoHide (true);
    vista.setSingleStepSizes (0, 40);
    addAndMakeVisible (vista);
    addAndMakeVisible (barra);

    // as pestanas: inicio, instrumentos, efectos, paquetes e (se as hai) actualizacións
    int orde = 1;
    for (int i = 0; i < 5; ++i)
    {
        auto b = std::make_unique<juce::TextButton> (i == 0 ? tr ("Inicio") : i == 1 ? tr ("Instrumentos") : i == 2 ? tr ("Efectos")
                                                     : i == 3 ? tr ("Paquetes") : tr ("Actualizacións"));
        b->getProperties().set ("estilo", "pestana");
        b->setToggleState (i == 0, juce::dontSendNotification);
        b->setExplicitFocusOrder (orde++);
        b->onClick = [this, i]
        {
            lista.seccion = i;
            for (size_t j = 0; j < pestanaBtn.size(); ++j) pestanaBtn[j]->setToggleState ((int) j == i, juce::dontSendNotification);
            lista.colocar (lista.getWidth());
            vista.setViewPosition (0, 0);
            repaint();
        };
        addAndMakeVisible (*b);
        pestanaBtn.push_back (std::move (b));
    }
    // o buscador: ábrese coa lupa
    buscador.getProperties().set ("buscador", true);
    buscador.setTextToShowWhenEmpty (tr ("Buscar"), col::crema3);
    buscador.setFont (body (css (13.0f)));
    buscador.setIndents (16, 0);
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
    buscador.onEscapeKey = [this] { buscador.clear(); lista.busca.clear(); lista.colocar (lista.getWidth()); recontos(); abrirBusca (false); grabKeyboardFocus(); };
    buscador.onReturnKey = [this]
    {
        // Intro: á primeira capa que casa
        for (auto* c : lista.getChildren())
            if (auto* t = dynamic_cast<Tarxeta*> (c); t != nullptr && t->isVisible()) { t->grabKeyboardFocus(); return; }
    };
    buscador.onFocusLost = [this] { if (buscador.getText().trim().isEmpty()) abrirBusca (false); };
    addChildComponent (buscador);

    for (auto* b : { &lupa, &refrescar, &axustes }) b->getProperties().set ("redonda", true);
    lupa.getProperties().set ("icona", "lupa");
    lupa.setTitle (tr ("Buscar"));
    lupa.setTooltip (tr ("Buscar polo nome ou a descrición (Cmd/Ctrl + F)"));
    lupa.onClick = [this] { abrirBusca (! buscando || buscador.getText().isEmpty() ? ! buscando : true); };
    lupa.setExplicitFocusOrder (orde++);
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
    for (auto* b : { &lupa, &refrescar, &axustes }) addAndMakeVisible (b);
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

    addChildComponent (ficha);
    addChildComponent (modal);
    setWantsKeyboardFocus (true);
    // co teclado: Tab percorre a barra, o heroe, as capas (e as súas teclas) e o pé, na orde en que se ven
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

void Principal::abrirBusca (bool a)
{
    if (a == buscando) return;
    buscando = a;
    buscador.setVisible (a);
    resized();
    if (a) buscador.grabKeyboardFocus();
    repaint();
}

void Principal::recontos()
{
    const int n = (int) modelo.porActualizar().size();
    auto& act = *pestanaBtn.back();
    act.setButtonText (tr ("Actualizacións") + U (" · ") + juce::String (n));
    act.setVisible (n > 0 && ! baleiro());
    if (n == 0 && lista.seccion == 4) pestanaBtn.front()->onClick();
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
    lupa.setVisible (! baleiro());
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
    // a barra escurece ao baixar
    {
        const int y = vista.getViewPositionY();
        if ((y > 8) != (ultimoY > 8)) barra.repaint();
        ultimoY = y;
    }
    if (isShowing() && ! ficha.isVisible()) lista.animar (dt);
    if (ficha.isVisible() && modelo.traballando (ficha.getProduto())) ficha.repaint();
}

bool Principal::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress ('r', juce::ModifierKeys::commandModifier, 0) || k == juce::KeyPress::F5Key) { modelo.cargar (true); return true; }
    if (k == juce::KeyPress ('f', juce::ModifierKeys::commandModifier, 0) && lupa.isVisible()) { abrirBusca (true); buscador.grabKeyboardFocus(); buscador.selectAll(); return true; }
    if (k == juce::KeyPress::escapeKey && modal.isVisible()) { modal.pecharSen(); return true; }
    if (k == juce::KeyPress::escapeKey && ficha.isVisible()) { ficha.pechar(); return true; }
    if (k == juce::KeyPress::escapeKey && buscando) { buscador.clear(); buscador.onTextChange(); abrirBusca (false); return true; }
    if (k == juce::KeyPress::escapeKey && ! modelo.dawsAgardados.isEmpty()) { modelo.deixarDeAgardar(); return true; }
    return false;
}

int Principal::altoCompleto (int ancho)
{
    lista.colocar (ancho);
    return lista.getHeight() + pe.getHeight();
}

void Principal::resized()
{
    const int W = getWidth(), H = getHeight();
    const int m = W >= 1000 ? 44 : 20;
    const bool dobre = dobreFila();
    // a barra: a icona e o nome; as pestanas; a lupa, comprobar e os axustes (redondos)
    const int fila = 18;
    logo = { m, fila, 34 + 30 + juce::roundToInt (anchoTexto (cond (css (16.0f)), "Barullo Xestor")), 36 };
    int x = W - m;
    for (auto* b : { &axustes, &refrescar, &lupa })
    {
        b->setBounds (x - 36, fila, 36, 36);
        x -= 36 + (W >= 1000 ? 30 : 14);
    }
    if (buscando)
    {
        const int wb = juce::jlimit (140, 260, x + (W >= 1000 ? 30 : 14) - 12 - (dobre ? logo.getRight() + 16 : W / 2));
        zonaBusca = { lupa.getX() - 10 - wb, fila, wb, 36 };
        buscador.setBounds (zonaBusca);
        x = zonaBusca.getX() - 20;
    }
    else zonaBusca = {};
    int xp = dobre ? m : logo.getRight() + 30;
    const int yp = dobre ? fila + 36 + 10 : fila;
    for (auto& b : pestanaBtn)
    {
        if (! b->isVisible()) continue;
        const int w = Look::anchoTecla (*b, 20);
        b->setBounds (xp, yp + 2, w, 34);
        xp += w + 6;
    }
    barra.setBounds (0, 0, W, dobre ? 112 : 76);

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
    corpo = { 0, 0, W, pe.getY() };
    vista.setBounds (corpo);
    lista.colocar (corpo.getWidth() - (lista.getHeight() > corpo.getHeight() ? vista.getScrollBarThickness() : 0));
    volverProbar.setBounds (W / 2 - 90, corpo.getCentreY() + 60, 180, 40);
    ficha.setBounds (getLocalBounds());
    modal.setBounds (getLocalBounds());
}

void Principal::Barra::paint (juce::Graphics& g)
{
    // un degradado escuro (máis opaco ao baixar), a icona e o nome
    const bool baixou = p.vista.getViewPositionY() > 8 || ! p.lista.inicio();
    const auto r = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (fondoNoite.withAlpha (baixou ? 0.96f : 0.8f), 0.0f, 0.0f,
                                             fondoNoite.withAlpha (baixou ? 0.85f : 0.0f), 0.0f, r.getBottom(), false));
    g.fillRect (r);
    if (baixou)
    {
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillRect (r.withTop (r.getBottom() - 1.0f));
    }
    const auto l = p.logo.toFloat();
    const auto ic = iconaApp();
    const auto ri = juce::Rectangle<float> (l.getX(), l.getCentreY() - 17.0f, 34.0f, 34.0f);
    if (ic.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.setOpacity (1.0f);
        g.drawImage (ic, ri, juce::RectanglePlacement::stretchToFit);
    }
    g.setColour (col::crema);
    g.setFont (cond (css (16.0f)));
    g.drawText ("Barullo Xestor", juce::Rectangle<float> (ri.getRight() + 30.0f, l.getY(), 260.0f, l.getHeight()), juce::Justification::centredLeft, false);
    // o buscador aberto: o fondo (o texto debúxao o editor)
    if (p.buscando)
    {
        const auto b = p.zonaBusca.toFloat().reduced (0.5f);
        g.setColour (juce::Colour (0xff0c1020).withAlpha (0.9f));
        g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
    }
}

void Principal::paintPe (juce::Graphics& g)
{
    const auto r = pe.toFloat();
    g.setColour (fondoNoite.withAlpha (0.92f));
    g.fillRect (r);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillRect (r.withHeight (1.0f));
    const int m = getWidth() >= 1000 ? 44 : 20;
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
    if (modelo.cargando)
    {
        t = tr ("Preguntando a GitHub…");
        d = tr ("Buscando os plugins de Escoita: os repositorios con packaging/produto.env e a súa última versión.");
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
}

void Principal::paint (juce::Graphics& g)
{
    g.fillAll (fondoNoite);
    if (baleiro()) paintBaleiro (g);
    paintPe (g);
}

// ==================================================================== teclado e paneis
void Principal::globalFocusChanged (juce::Component* c)
{
    // ao chegar co Tab (ou coas frechas) a unha capa que non se ve, a lista desprázase ata ela (sen quedar baixo a barra)
    if (c == nullptr || ! lista.isParentOf (c)) return;
    const auto r = lista.getLocalArea (c, c->getLocalBounds()).expanded (0, 8);
    const auto v = vista.getViewArea().withTrimmedTop (barra.getHeight());
    if (r.getY() < v.getY()) vista.setViewPosition (0, juce::jmax (0, r.getY() - barra.getHeight()));
    else if (r.getBottom() > v.getBottom()) vista.setViewPosition (0, r.getBottom() - vista.getViewHeight());
}

void Principal::escena (const juce::String& e)
{
    const Produto* p = nullptr;
    for (auto& x : modelo.produtos())
        if (x.ultima && (p == nullptr || modelo.situacion (x).estado() == Estado::actualizacion)) p = &x;
    if (e == "efectos" || e == "actualizacions")
        pestanaBtn[e == "efectos" ? 2 : 4]->onClick();
    else if (e == "busca")
    {
        abrirBusca (true);
        buscador.setText ("verb", true);
    }
    else if (e == "sobre")
    {
        if (auto* t = lista.tarxeta ("Barullo Xuntos")) t->porSobre (true);
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

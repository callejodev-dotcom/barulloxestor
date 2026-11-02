#include "Paneis.h"

using namespace aspecto;
using namespace xestor;

namespace paneis
{
// ==================================================================== o panel
Modal::Modal()
{
    corpo.setMultiLine (true, true);
    corpo.setReadOnly (true);
    corpo.setCaretVisible (false);
    corpo.setScrollbarsShown (true);
    corpo.setPopupMenuEnabled (true);   // (para copiar)
    corpo.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    corpo.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    corpo.setColour (juce::TextEditor::focusedOutlineColourId, col::laranxa.withAlpha (0.5f));
    corpo.setColour (juce::TextEditor::textColourId, col::crema2);
    corpo.setColour (juce::TextEditor::highlightColourId, col::laranxa.withAlpha (0.35f));
    corpo.setColour (juce::TextEditor::highlightedTextColourId, col::crema);
    corpo.setFont (body (14.0f));
    corpo.setIndents (14, 10);
    corpo.setTitle (tr ("Texto"));
    addAndMakeVisible (corpo);
    pecharBtn.getProperties().set ("icona", "pechar");
    pecharBtn.setTitle (tr ("Pechar"));
    pecharBtn.onClick = [this] { pecharSen(); };
    addAndMakeVisible (pecharBtn);
    setWantsKeyboardFocus (true);
    // Tab móvese só entre o que hai no panel mentres está aberto
    setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);
    setVisible (false);
}

void Modal::abrir (const juce::String& t, const juce::String& s, const juce::String& txt, juce::Colour l,
                   std::vector<Tecla> ts, bool lg)
{
    titulo = t;
    subtitulo = s;
    luz = l;
    longo = lg;
    teclas = std::move (ts);
    botons.clear();
    for (size_t i = 0; i < teclas.size(); ++i)
    {
        auto b = std::make_unique<juce::TextButton> (teclas[i].texto);
        if (teclas[i].primaria) b->getProperties().set ("primaria", true);
        else b->getProperties().set ("noLed", true);
        b->getProperties().set ("led", (juce::int64) luz.getARGB());
        b->onClick = [this, i]
        {
            auto f = teclas[i].accion;
            pecharSen();
            // despois: a acción pode abrir outro panel (e borrar esta tecla mentres se preme)
            if (f) juce::MessageManager::callAsync (f);
        };
        addAndMakeVisible (*b);
        botons.push_back (std::move (b));
    }
    setTitle (titulo);
    setDescription (subtitulo);
    setVisible (true);
    toFront (false);
    porTexto (txt);
    // o foco, na tecla principal (ou na última)
    juce::TextButton* foco = nullptr;
    for (size_t i = 0; i < botons.size(); ++i) if (teclas[i].primaria) foco = botons[i].get();
    if (foco == nullptr && ! botons.empty()) foco = botons.back().get();
    if (foco != nullptr && isShowing()) foco->grabKeyboardFocus();
    else if (isShowing()) grabKeyboardFocus();
}

void Modal::porTexto (const juce::String& t)
{
    corpo.setText (t, false);
    corpo.moveCaretToTop (false);
    colocar();
    repaint();
}

void Modal::pecharSen()
{
    setVisible (false);
    if (auto* p = getParentComponent()) p->grabKeyboardFocus();
}

void Modal::colocar()
{
    if (getWidth() <= 0) return;
    const int ancho = juce::jmin (640, getWidth() - 60);
    const int dentro = ancho - 2 * 22 - 28;
    // o alto do texto (cos tamaños do TextEditor)
    juce::AttributedString as;
    as.append (corpo.getText(), body (14.0f));
    juce::TextLayout tl;
    tl.createLayout (as, (float) dentro);
    const int altoTexto = juce::roundToInt (tl.getHeight()) + 24;
    const int maxCorpo = getHeight() - 60 - 74 - 70;
    // axustado ao texto; se é longo (as notas), cun mínimo e desprazándose se non colle
    const int minimo = longo ? 160 : 60;
    const int hCorpo = juce::jlimit (minimo, juce::jmax (minimo, maxCorpo), altoTexto);
    const int alto = 74 + juce::jmin (hCorpo, juce::jmax (60, maxCorpo)) + 70;
    caixa = juce::Rectangle<int> (ancho, alto).withCentre (getLocalBounds().getCentre());
    corpo.setBounds (caixa.getX() + 22, caixa.getY() + 70, caixa.getWidth() - 44, caixa.getHeight() - 70 - 66);
    pecharBtn.setBounds (caixa.getRight() - 50, caixa.getY() + 14, 36, 32);
    int x = caixa.getRight() - 22;
    for (int i = (int) botons.size() - 1; i >= 0; --i)
    {
        auto& b = *botons[(size_t) i];
        const int w = juce::jlimit (110, 260, (int) anchoTexto (caps (9.5f, 0.15f), upper (b.getButtonText())) + (teclas[(size_t) i].primaria ? 52 : 36));
        b.setBounds (x - w, caixa.getBottom() - 52, w, 34);
        x -= w + 10;
    }
}

void Modal::resized() { colocar(); }

void Modal::paint (juce::Graphics& g)
{
    g.fillAll (col::noite.withAlpha (0.82f));
    const auto r = caixa.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 6.0f).expanded (2.0f), 16.0f);
    paintPanel (g, r, 14.0f);
    // a cabeceira: o LED, o título e debaixo o subtítulo
    auto cab = juce::Rectangle<float> (r.getX() + 24.0f, r.getY() + 18.0f, r.getWidth() - 90.0f, 22.0f);
    paintLed (g, { cab.getX() + 4.0f, cab.getCentreY() }, true, luz);
    g.setColour (col::crema);
    g.setFont (cond (16.0f).withExtraKerningFactor (0.04f));
    g.drawFittedText (titulo, cab.withTrimmedLeft (18.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    if (subtitulo.isNotEmpty())
    {
        g.setColour (col::crema3);
        drawCaps (g, subtitulo, cab.translated (18.0f, 24.0f).withHeight (12.0f), juce::Justification::centredLeft, 8.0f, 0.2f);
    }
    paintWell (g, corpo.getBounds().toFloat().expanded (2.0f), 10.0f);
}

bool Modal::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { pecharSen(); return true; }
    if (k == juce::KeyPress::returnKey)
    {
        for (size_t i = 0; i < botons.size(); ++i)
            if (teclas[i].primaria) { botons[i]->triggerClick(); return true; }
        pecharSen();
        return true;
    }
    return false;
}

void Modal::mouseUp (const juce::MouseEvent& e)
{
    if (! caixa.contains (e.getPosition())) pecharSen();
}

Modal* atopar (juce::Component* desde)
{
    if (desde == nullptr) return nullptr;
    std::function<Modal* (juce::Component*)> buscar = [&] (juce::Component* c) -> Modal*
    {
        if (auto* m = dynamic_cast<Modal*> (c)) return m;
        for (auto* f : c->getChildren())
            if (auto* m = buscar (f)) return m;
        return nullptr;
    };
    return buscar (desde->getTopLevelComponent());
}

// ==================================================================== o DAW aberto
void avisoDaw (Modelo& modelo, juce::Component* desde, const juce::StringArray& daws, std::function<void()> accion)
{
    auto* m = atopar (desde);
    if (m == nullptr) { accion(); return; }
    const auto nomes = daws.joinIntoString (", ");
    auto* mo = &modelo;
    m->abrir (tr (daws.size() == 1 ? "%1 está aberto" : "%1 están abertos", nomes),
              tr ("Antes de cambiar os plugins"),
              tr ("Se o DAW ten cargado algún destes plugins, pode seguir usando a versión vella (ou non ver a nova) "
                  "ata que o reinicies ou volvas escanear os plugins. O mellor é pechalo antes.\n\n"
                  "«Agardar» faino só cando o peches; «Continuar igual» faino agora."),
              col::ouro,
              { { tr ("Cancelar"), {} },
                { tr ("Continuar igual"), accion },
                { tr ("Agardar a que se peche"), [mo, daws, accion] { mo->agardarDaw (daws, accion); }, true } });
}

void protexer (Modelo& modelo, juce::Component* desde, std::function<void()> accion)
{
    const auto daws = sistema::dawsAbertos();
    if (daws.isEmpty()) { accion(); return; }
    avisoDaw (modelo, desde, daws, std::move (accion));
}

// ==================================================================== as notas da versión
void notas (Modelo& modelo, juce::Component* desde, const Produto& p)
{
    auto* m = atopar (desde);
    if (m == nullptr) return;
    const auto s = modelo.situacion (p);
    const auto e = s.estado();
    const auto v = p.ultima ? texto (p.ultima->version()) : (p.versionLocal ? p.versionLocal->texto() : juce::String());
    juce::String sub = tr ("Notas da versión %1", v);
    if (p.ultima && p.ultima->publicado != juce::Time())
        sub << "  ·  " << juce::String (p.ultima->publicado.getDayOfMonth()) << " " << textos::mesCurto (p.ultima->publicado.getMonth() + 1)
            << " " << juce::String (p.ultima->publicado.getYear());
    std::vector<Modal::Tecla> teclas;
    if (p.ultima && p.ultima->paxina.isNotEmpty())
        teclas.push_back ({ tr ("Ver en GitHub"), [u = p.ultima->paxina] { juce::URL (u).launchInDefaultBrowser(); } });
    teclas.push_back ({ tr ("Pechar"), {} });
    if (e == Estado::actualizacion || e == Estado::incompleto || e == Estado::nonInstalado)
    {
        auto* mo = &modelo;
        juce::Component::SafePointer<juce::Component> sp (desde);
        teclas.push_back ({ e == Estado::nonInstalado ? tr ("Instalar a %1", v) : tr ("Actualizar á %1", v),
                            [mo, sp, p] { protexer (*mo, sp.getComponent(), [m2 = mo, p2 = p] { m2->instalar (p2); }); }, true });
    }
    m->abrir (p.nome, sub, tr ("Cargando as notas…"), acento (p.nome), std::move (teclas), true);
    juce::Component::SafePointer<Modal> sm (m);
    modelo.pedirNotas (p, [sm] (const juce::String& n)
    {
        if (sm == nullptr || ! sm->isVisible()) return;
        sm->porTexto (n.isNotEmpty() ? textos::markdownLexible (n) : tr ("Esta versión non ten notas."));
    });
}

// ==================================================================== o propio xestor
void xestorNovo (Modelo& modelo, juce::Component* desde)
{
    auto* m = atopar (desde);
    if (m == nullptr || ! modelo.xestorNovo) return;
    const auto l = *modelo.xestorNovo;
    const auto v = texto (l.version());
    auto* mo = &modelo;
    juce::String explica = soActual() == SO::win
        ? tr ("Descárgase o instalador (comprobando a suma), péchase o xestor e instálase a nova en silencio, sen admin.")
        : tr ("Descárgase (comprobando a suma), a de agora vai ao Lixo, a nova ponse no seu sitio e ábrese.");
    const auto n = l.notas.isNotEmpty() ? textos::markdownLexible (l.notas) : juce::String();
    std::vector<Modal::Tecla> teclas;
    if (l.paxina.isNotEmpty()) teclas.push_back ({ tr ("Ver en GitHub"), [u = l.paxina] { juce::URL (u).launchInDefaultBrowser(); } });
    teclas.push_back ({ tr ("Agora non"), {} });
    teclas.push_back ({ tr ("Actualizar e reiniciar"), [mo] { mo->actualizarXestor(); }, true });
    m->abrir (tr ("Hai Barullo Xestor %1", v), tr ("Tes a %1", juce::String (JUCE_APPLICATION_VERSION_STRING)),
              explica + (n.isNotEmpty() ? "\n\n" + n : juce::String()), col::laranxa, std::move (teclas), n.isNotEmpty());
}

// ==================================================================== atallos
void atallos (juce::Component* desde)
{
    auto* m = atopar (desde);
    if (m == nullptr) return;
    const auto cmd = soActual() == SO::mac ? juce::String ("Cmd") : juce::String ("Ctrl");
    m->abrir (tr ("Atallos do teclado"), "Barullo Xestor",
              tr ("Tab / Maiús + Tab: ir dunha tecla á seguinte (a lista desprázase soa)\n"
                  "Intro: premer a tecla que ten o foco (o aro de cor)\n"
                  "Esc: pechar o panel, o menú ou o visor (e deixar de agardar polo DAW)\n"
                  "%1 + R ou F5: comprobar agora en GitHub\n"
                  "← →: pasar as capturas no visor\n"
                  "Nos menús: as frechas e Intro", cmd),
              col::azul, { { tr ("Pechar"), {}, true } });
}

// ==================================================================== o menú ••• do xestor
void menuXestor (Modelo& modelo, juce::Component* boton)
{
    auto* mo = &modelo;
    juce::Component::SafePointer<juce::Component> sp (boton);
    juce::PopupMenu m;
    m.addSectionHeader ("Barullo Xestor " + juce::String (JUCE_APPLICATION_VERSION_STRING));

    juce::PopupMenu idiomas;
    const auto escollido = idiomaDeClave (modelo.axustes.idioma);
    idiomas.addItem (tr ("Automático (o do sistema: %1)", nome (idiomaDoSistema())), true, ! escollido, [mo] { mo->cambiarIdioma (std::nullopt); });
    idiomas.addSeparator();
    for (auto i : { Idioma::gl, Idioma::es, Idioma::en })
        idiomas.addItem (nome (i), true, escollido == i, [mo, i] { mo->cambiarIdioma (i); });
    m.addSubMenu (tr ("Idioma"), idiomas);

    m.addItem (soActual() == SO::mac ? tr ("Comprobar cada día (icona na barra de menús)") : tr ("Comprobar cada día (icona na bandeja do sistema)"),
               true, modelo.axustes.bandeja, [mo] { mo->cambiarBandeja (! mo->axustes.bandeja); });
    m.addItem (tr ("Copia dos presets antes de actualizar"), true, modelo.axustes.copiaPresets,
               [mo] { mo->cambiarCopiaPresets (! mo->axustes.copiaPresets); });
    m.addSeparator();
    if (modelo.xestorNovo)
        m.addItem (tr ("Actualizar a Barullo Xestor %1…", texto (modelo.xestorNovo->version())), modelo.progresoXestor < 0.0,
                   false, [mo, sp] { xestorNovo (*mo, sp.getComponent()); });
    else
        m.addItem (tr ("Buscar unha versión nova do xestor"), ! modelo.cargando, false, [mo] { mo->cargar (true); });
    m.addItem (tr ("Copiar informe (para pedir axuda)"), [mo] { mo->copiarInforme(); });
    const auto cp = presets::cartafolCopias();
    if (cp.isDirectory()) m.addItem (tr ("Abrir as copias dos presets"), [cp] { cp.revealToUser(); });
    m.addItem (tr ("Atallos do teclado"), [sp] { atallos (sp.getComponent()); });
    m.addItem (tr ("Barullo Xestor en GitHub"), [] { juce::URL ("https://github.com/" + autoactualizacion::repo()).launchInDefaultBrowser(); });
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (boton));
}
} // namespace paneis

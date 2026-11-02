#include "Bandeja.h"
#include "Aspecto.h"

using namespace xestor;

Bandeja::Bandeja (Modelo& m, std::function<void()> abrir, std::function<void()> s)
    : modelo (m), abrirXanela (std::move (abrir)), sair (std::move (s))
{
    modelo.addChangeListener (this);
    actualizarIcona();
    startTimer (10 * 1000);   // a primeira vez, aos poucos segundos; despois cada media hora
}

Bandeja::~Bandeja()
{
    modelo.removeChangeListener (this);
}

juce::Image Bandeja::icona (bool aviso, bool monocromo, int tam)
{
    // o mando da familia (coma a icona da app): en cor coa placa (Windows, Linux) ou en negro
    // para o «template» da barra de menús do Mac; cun punto se hai algo novo
    juce::Image im (juce::Image::ARGB, tam, tam, true);
    juce::Graphics g (im);
    const float t = (float) tam;
    // o punto do aviso, abaixo á dereita, cun oco arredor (o mando non o toca)
    const float r = t * 0.17f;
    const auto punto = juce::Rectangle<float> (t - 2.0f * r, t - 2.0f * r, 2.0f * r, 2.0f * r);
    {
        juce::Graphics::ScopedSaveState ss (g);
        if (aviso)
        {
            juce::Path fora;
            fora.setUsingNonZeroWinding (false);
            fora.addRectangle (0.0f, 0.0f, t, t);
            fora.addEllipse (punto.expanded (t * 0.06f));
            g.reduceClipRegion (fora);
        }
        // sen placa, o mando enche o cadrado (o arco vai de 237 a 787 en 1024)
        const auto zona = monocromo ? juce::Rectangle<float> (0.0f, 0.0f, t, t).expanded (t * 0.34f) : juce::Rectangle<float> (0.0f, 0.0f, t, t).expanded (t * 0.1f);
        aspecto::paintMandoFamilia (g, zona, ! monocromo, monocromo);
    }
    if (aviso)
    {
        g.setColour (monocromo ? juce::Colours::black : aspecto::col::laranxa);
        g.fillEllipse (punto);
    }
    return im;
}

void Bandeja::actualizarIcona()
{
    const auto chaves = modelo.chavesPorActualizar();
    const int aviso = chaves.isEmpty() ? 0 : 1;
    if (aviso != avisoActual)
    {
        avisoActual = aviso;
        setIconImage (icona (aviso != 0, false, 64), icona (aviso != 0, true, 36));
    }
    setIconTooltip (chaves.isEmpty() ? tr ("Barullo Xestor: todo ao día")
                                     : tr ("Barullo Xestor: %1", chaves.joinIntoString (", ")));
}

void Bandeja::notificar (const juce::String& titulo, const juce::String& texto)
{
    if (soActual() == SO::win) { showInfoBubble (titulo, texto); return; }
    // osascript ou notify-send: nun fío aparte (tardan un pouco)
    juce::Thread::launch ([titulo, texto] { sistema::notificar (titulo, texto); });
}

void Bandeja::timerCallback()
{
    startTimer (30 * 60 * 1000);
    if (comprobacion::toca (modelo.axustes.ultimaComprobacion) && ! modelo.cargando && modelo.traballos.empty())
    {
        comprobando = true;
        modelo.cargar (true);
    }
}

void Bandeja::changeListenerCallback (juce::ChangeBroadcaster*)
{
    actualizarIcona();
    if (! comprobando || modelo.cargando) return;
    comprobando = false;
    auto& ax = modelo.axustes;
    ax.ultimaComprobacion = juce::Time::currentTimeMillis();
    const auto agora = modelo.chavesPorActualizar();
    const auto novas = comprobacion::novidades (agora, ax.avisadas);
    ax.avisadas = agora;   // (o que xa non está, esquécese: se volve, avísase de novo)
    ax.gardar();
    if (! novas.isEmpty())
        notificar (novas.size() == 1 ? tr ("Hai unha versión nova") : tr ("Hai %1 versións novas", juce::String (novas.size())),
                   novas.joinIntoString (", "));
}

void Bandeja::mouseDown (const juce::MouseEvent&)
{
    juce::PopupMenu m;
    const auto chaves = modelo.chavesPorActualizar();
    m.addSectionHeader (chaves.isEmpty() ? tr ("Todo ao día") : tr ("Versións novas: %1", chaves.joinIntoString (", ")));
    m.addItem (tr ("Abrir Barullo Xestor"), [this] { abrirXanela(); });
    m.addItem (tr ("Comprobar agora"), ! modelo.cargando, false, [this] { comprobando = true; modelo.cargar (true); });
    m.addSeparator();
    m.addItem (tr ("Non comprobar en segundo plano"), [this] { modelo.cambiarBandeja (false); });
    m.addItem (tr ("Saír"), [this] { sair(); });
   #if JUCE_MAC
    showDropdownMenu (m);
   #else
    m.showMenuAsync (juce::PopupMenu::Options());
   #endif
}

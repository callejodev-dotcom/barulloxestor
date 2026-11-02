// Barullo Xestor: a app. Con opcións «--algo», modo de liña de ordes (sen xanela); se non, a xanela.
#include <juce_gui_basics/juce_gui_basics.h>
#include "CLI.h"
#include "Vistas.h"
#include "Bandeja.h"
#include <cstdio>

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <shellapi.h>
 #if defined (_MSC_VER)
  #pragma comment (lib, "shell32.lib")
 #endif
#endif

namespace
{
// ================================================================ a xanela
class Xanela : public juce::DocumentWindow
{
public:
    explicit Xanela (Modelo& m)
        : juce::DocumentWindow ("Barullo Xestor", aspecto::col::noite, juce::DocumentWindow::allButtons), modelo (m)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (new vistas::Principal (m), true);
        setResizable (true, false);
        setResizeLimits (760, 480, 4000, 3000);
        centreWithSize (vistas::Principal::anchoInicial, vistas::Principal::altoInicial);
        setVisible (true);
    }
    // co idioma novo: a xanela feita de novo (todos os textos)
    void refacer()
    {
        const auto r = getBounds();
        setContentOwned (new vistas::Principal (modelo), false);
        setBounds (r);
    }
    // se a icona da bandeja está, pechar a xanela só a agocha (a app queda alí)
    std::function<bool()> quedarNaBandeja;
    void closeButtonPressed() override
    {
        if (quedarNaBandeja && quedarNaBandeja())
        {
            setVisible (false);
           #if JUCE_MAC
            juce::Process::setDockIconVisible (false);
           #endif
            return;
        }
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
    void amosar()
    {
       #if JUCE_MAC
        juce::Process::setDockIconVisible (true);
       #endif
        setVisible (true);
        setMinimised (false);
        toFront (true);
       #if JUCE_MAC
        juce::Process::makeForegroundProcess();
       #endif
    }
private:
    Modelo& modelo;
};

class App : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Barullo Xestor"; }
    const juce::String getApplicationVersion() override { return ESCOITA_VERSION; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise (const juce::String&) override
    {
        modelo = std::make_unique<Modelo>();
        xanela = std::make_unique<Xanela> (*modelo);
        xanela->quedarNaBandeja = [this] { return bandeja != nullptr; };
        modelo->sair = [] { quit(); };
        modelo->alCambiarIdioma = [this]
        {
            // despois do menú (que aínda está a pechar)
            juce::MessageManager::callAsync ([this] { if (xanela != nullptr) xanela->refacer(); });
        };
        modelo->alCambiarBandeja = [this] { porBandeja(); };
        porBandeja();
        modelo->cargar();
    }
    void shutdown() override
    {
        bandeja = nullptr;
        xanela = nullptr;
        modelo = nullptr;
    }
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted (const juce::String&) override { if (xanela != nullptr) xanela->amosar(); }

private:
    void porBandeja()
    {
        if (modelo->axustes.bandeja && bandeja == nullptr)
            bandeja = std::make_unique<Bandeja> (*modelo, [this] { xanela->amosar(); }, [] { quit(); });
        else if (! modelo->axustes.bandeja && bandeja != nullptr)
        {
            bandeja = nullptr;
            if (! xanela->isVisible()) xanela->amosar();   // sen icona, a xanela non pode quedar agochada
        }
    }

    std::unique_ptr<Modelo> modelo;
    std::unique_ptr<Xanela> xanela;
    std::unique_ptr<Bandeja> bandeja;
};

juce::JUCEApplicationBase* crearApp() { return new App(); }

// ================================================================ captura (sen abrir a xanela)
int captura (const xestor::cli::Opcions& o)
{
    juce::ScopedJuceInitialiser_GUI gui;
    Modelo m;
    if (auto i = xestor::idiomaDeClave (o.idioma)) xestor::porIdioma (*i);   // só para esta captura
    if (o.formatos) m.axustes.formatos = *o.formatos;   // só para esta captura: non se garda
    if (o.escena != "baleiro") m.cargarAgora (o.refrescar);
    m.escena (o.escena);
    vistas::Principal p (m);
    p.setSize (o.ancho, o.alto);
    if (o.completa) p.setSize (o.ancho, juce::jmax (o.alto, p.altoCompleto (o.ancho)));
    if (o.escena == "captura")
        for (auto& pr : m.produtos())
            if (! pr.capturas.isEmpty()) { p.abrirVisor (pr, 0); break; }
    p.resized();
    p.escena (o.escena);   // os paneis (notas, DAW, xestor, atallos)
    auto im = p.createComponentSnapshot (p.getLocalBounds(), true, o.escala);
    o.captura.deleteFile();
    juce::FileOutputStream out (o.captura);
    if (! out.openedOk() || ! juce::PNGImageFormat().writeImageToStream (im, out))
    {
        std::fprintf (stderr, "Non se puido gardar %s\n", o.captura.getFullPathName().toRawUTF8());
        return 1;
    }
    std::printf ("Captura: %s (%dx%d)\n", o.captura.getFullPathName().toRawUTF8(), im.getWidth(), im.getHeight());
    for (auto& f : xestor::textos::senTraducion())
        std::fprintf (stderr, "sen traducir: %s\n", f.toRawUTF8());
    return 0;
}

// ================================================================ a icona (1024 px)
// Un mando da familia (o disco mecanizado e o anel de LED laranxa) nunha placa azul mariño.
int icona (const xestor::cli::Opcions& o)
{
    juce::ScopedJuceInitialiser_GUI gui;
    using namespace aspecto;
    const int N = 1024;
    juce::Image im (juce::Image::ARGB, N, N, true);
    juce::Graphics g (im);
    const auto placa = juce::Rectangle<float> (100.0f, 100.0f, 824.0f, 824.0f);
    const float rad = 185.0f;
    for (int i = 10; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.035f));
        g.fillRoundedRectangle (placa.translated (0.0f, 2.0f * (float) i).expanded ((float) i * 1.5f), rad + (float) i);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff222c4c), 0.0f, placa.getY(), juce::Colour (0xff0d1326), 0.0f, placa.getBottom(), false));
    g.fillRoundedRectangle (placa, rad);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (placa.reduced (1.5f), rad, 3.0f);

    const auto c = placa.getCentre().translated (0.0f, 6.0f);
    const float ring = 300.0f;
    const float a0 = juce::MathConstants<float>::pi * 1.25f, a1 = juce::MathConstants<float>::pi * 2.75f;
    const float ang = a0 + 0.72f * (a1 - a0);
    juce::Path pista; pista.addCentredArc (c.x, c.y, ring, ring, 0.0f, a0, a1, true);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (pista, juce::PathStrokeType (34.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path val; val.addCentredArc (c.x, c.y, ring, ring, 0.0f, a0, ang, true);
    for (auto [w, a] : { std::pair<float, float> { 90.0f, 0.10f }, { 58.0f, 0.22f }, { 34.0f, 1.0f } })
    {
        g.setColour (col::laranxa.withAlpha (a));
        g.strokePath (val, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    const float kr = 222.0f;
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.x - kr, c.y - kr + 22.0f, kr * 2.0f, kr * 2.0f);
    juce::ColourGradient disco (juce::Colour (0xff56638a), c.x - kr * 0.3f, c.y - kr * 0.45f, juce::Colour (0xff0e1323), c.x + kr * 0.55f, c.y + kr * 1.05f, true);
    disco.addColour (0.55, juce::Colour (0xff232b45));
    g.setGradientFill (disco);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 4.0f);
    const juce::Point<float> punta (c.x + kr * 0.80f * std::sin (ang), c.y - kr * 0.80f * std::cos (ang));
    g.setColour (col::crema);
    g.drawLine ({ c, punta }, 22.0f);
    g.fillEllipse (c.x - 11.0f, c.y - 11.0f, 22.0f, 22.0f);
    // o punto laranxa de «escoita.»
    g.setColour (col::laranxa);
    g.fillEllipse (placa.getRight() - 150.0f, placa.getBottom() - 150.0f, 44.0f, 44.0f);

    if (o.icona.getFileNameWithoutExtension().endsWith ("-bandeja"))
        im = Bandeja::icona (true, false, 256);   // (para revisar a da bandeja)
    o.icona.deleteFile();
    juce::FileOutputStream out (o.icona);
    if (! out.openedOk() || ! juce::PNGImageFormat().writeImageToStream (im, out)) return 1;
    std::printf ("Icona: %s\n", o.icona.getFullPathName().toRawUTF8());
    return 0;
}

int linhaDeOrdes (const juce::StringArray& args)
{
    xestor::cli::Contexto ctx;
    ctx.captura = captura;
    ctx.icona = icona;
    return xestor::cli::executar (args, ctx);
}
} // namespace

// ================================================================ entrada
#if JUCE_WINDOWS
int __stdcall WinMain (struct HINSTANCE__*, struct HINSTANCE__*, char*, int)
{
    juce::StringArray args;
    int n = 0;
    if (auto** v = CommandLineToArgvW (GetCommandLineW(), &n))
    {
        for (int i = 1; i < n; ++i) args.add (juce::String (v[i]));
        LocalFree (v);
    }
    if (xestor::cli::eCLI (args))
    {
        // unha app de Windows non ten consola: úsase a de quen a chamou (se a hai)
        if (AttachConsole (ATTACH_PARENT_PROCESS))
        {
            FILE* f = nullptr;
            freopen_s (&f, "CONOUT$", "w", stdout);
            freopen_s (&f, "CONOUT$", "w", stderr);
            SetConsoleOutputCP (CP_UTF8);   // os tiles e as comiñas «»
        }
        return linhaDeOrdes (args);
    }
    juce::JUCEApplicationBase::createInstance = &crearApp;
    return juce::JUCEApplicationBase::main();
}
#else
int main (int argc, char* argv[])
{
    juce::StringArray args;
    for (int i = 1; i < argc; ++i) args.add (juce::String::fromUTF8 (argv[i]));
    if (xestor::cli::eCLI (args)) return linhaDeOrdes (args);
    juce::JUCEApplicationBase::createInstance = &crearApp;
    return juce::JUCEApplicationBase::main (argc, (const char**) argv);
}
#endif

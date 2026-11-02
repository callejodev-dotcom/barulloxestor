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
    if (o.escena == "cargador-fotogramas")
    {
        // oito fotogramas do cargador nunha tira (para ver o movemento)
        const int n = 8, lado = 220;
        juce::Image tira (juce::Image::ARGB, (int) ((float) (n * lado) * o.escala), (int) ((float) lado * o.escala), true);
        {
            juce::Graphics g (tira);
            g.addTransform (juce::AffineTransform::scale (o.escala));
            g.fillAll (juce::Colour (0xff070a14));
            for (int i = 0; i < n; ++i)
                aspecto::paintCargador (g, juce::Rectangle<float> ((float) (i * lado), 0.0f, (float) lado, (float) lado).reduced (30.0f),
                                        3.2 * (double) i / (double) n);
        }
        o.captura.deleteFile();
        juce::FileOutputStream out (o.captura);
        if (! out.openedOk() || ! juce::PNGImageFormat().writeImageToStream (tira, out)) return 1;
        std::printf ("Captura: %s\n", o.captura.getFullPathName().toRawUTF8());
        return 0;
    }
    if (o.escena != "baleiro" && o.escena != "cargando") m.cargarAgora (o.refrescar);
    m.escena (o.escena);
    vistas::Principal p (m);
    p.setSize (o.ancho, o.alto);
    if (o.completa) p.setSize (o.ancho, juce::jmax (o.alto, p.altoCompleto (o.ancho)));
    p.resized();
    p.escena (o.escena);   // os paneis (notas, DAW, xestor, atallos) e a ficha
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
// O mando da familia: o arco de segmentos coas oito cores, o disco e a agulla, nunha placa azul
// mariño (a de Resources/Icona.png; isto debúxaa de novo, para comprobala e para a bandeja).
int icona (const xestor::cli::Opcions& o)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const int N = 1024;
    juce::Image im (juce::Image::ARGB, N, N, true);
    {
        juce::Graphics g (im);
        aspecto::paintMandoFamilia (g, { 0.0f, 0.0f, (float) N, (float) N }, true);
    }

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

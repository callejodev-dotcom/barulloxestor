#pragma once
#include "Aspecto.h"
#include "Modelo.h"

// O que se abre por riba da lista, coma o visor das capturas: un panel con título, texto e teclas
// (as notas da versión, o aviso do DAW aberto, a versión nova do xestor), e o menú ••• do xestor.
// Co teclado: Tab entre as teclas, Intro preme, Esc pecha.
namespace paneis
{
class Modal : public juce::Component
{
public:
    struct Tecla
    {
        juce::String texto;
        std::function<void()> accion;
        bool primaria = false;
    };

    Modal();
    // o panel axústase ao texto (e desprázase se non colle); «longo» (as notas): un pouco máis alto de mínimo
    void abrir (const juce::String& titulo, const juce::String& subtitulo, const juce::String& texto,
                juce::Colour luz, std::vector<Tecla> teclas, bool longo = false);
    void pecharSen();   // pecha sen facer nada (Esc, clic fóra, a ×)
    void porTexto (const juce::String&);   // cambia o texto (as notas chegan despois)
    bool estaAberto() const { return isVisible(); }

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void colocar();
    juce::String titulo, subtitulo;
    juce::Colour luz = aspecto::col::laranxa;
    bool longo = false;
    juce::TextEditor corpo;
    juce::TextButton pecharBtn;
    std::vector<std::unique_ptr<juce::TextButton>> botons;
    std::vector<Tecla> teclas;
    juce::Rectangle<int> caixa;
};

// O Modal da xanela onde está «desde» (nullptr se non hai)
Modal* atopar (juce::Component* desde);

// Antes de instalar, actualizar, volver atrás ou desinstalar: se hai un DAW aberto, avísase
// (agardar a que se peche / continuar igual / cancelar); se non, faise xa.
void protexer (Modelo&, juce::Component* desde, std::function<void()> accion);
void avisoDaw (Modelo&, juce::Component* desde, const juce::StringArray& daws, std::function<void()> accion);
// As notas da versión publicada dun produto (coa tecla de actualizar, se hai que facelo)
void notas (Modelo&, juce::Component* desde, const xestor::Produto&);
// «Hai Barullo Xestor X»: as notas e «Actualizar e reiniciar»
void xestorNovo (Modelo&, juce::Component* desde);
// O menú ••• da cabeceira: idioma, a icona da bandeja, a copia dos presets, o informe...
void menuXestor (Modelo&, juce::Component* boton);
// Os atallos do teclado
void atallos (juce::Component* desde);
} // namespace paneis

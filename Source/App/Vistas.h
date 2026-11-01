#pragma once
#include "Aspecto.h"
#include "Modelo.h"
#include <escoita_nucleo/escoita_nucleo.h>

// A xanela: a cabeceira (a placa, o visor do resumo, os formatos e «Actualizar todo»), a lista
// de tarxetas (unha por plugin, coa súa captura e a súa cor) e o pé coma nos plugins.
namespace vistas
{
// ------------------------------------------------------------------ unha tarxeta por produto
class Tarxeta : public juce::Component
{
public:
    Tarxeta (Modelo&, std::function<void (const xestor::Produto&, int)> abrirVisor, std::function<void()> refacer);
    void mostrar (const xestor::Produto&);
    const xestor::Produto& getProduto() const { return produto; }
    int alto (int ancho) const;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void menu();
    void confirmarDesinstalar();
    juce::Rectangle<int> zonaMiniatura() const;
    void paintMiniatura (juce::Graphics&, juce::Rectangle<float>);
    void paintSerie (juce::Graphics&, juce::Rectangle<float>);

    Modelo& modelo;
    xestor::Produto produto;
    std::function<void (const xestor::Produto&, int)> abrirVisor;
    std::function<void()> refacer;
    juce::TextButton accion, mais, abrir;
    bool aberta = false, sobreMiniatura = false;
    static constexpr int altoBase = 136, altoFila = 26;
};

// ------------------------------------------------------------------ un bundle de Escoita sen repo
class FilaOrfo : public juce::Component
{
public:
    FilaOrfo (Modelo&, const xestor::Instalado&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    Modelo& modelo;
    xestor::Instalado inst;
    juce::TextButton quitar;
};

// ------------------------------------------------------------------ a lista (dentro do Viewport)
class Lista : public juce::Component
{
public:
    Lista (Modelo&, std::function<void (const xestor::Produto&, int)> abrirVisor);
    void actualizar();
    void colocar (int ancho);   // reparte as tarxetas e cambia o alto
    void paint (juce::Graphics&) override;

private:
    Modelo& modelo;
    std::function<void (const xestor::Produto&, int)> abrirVisor;
    std::vector<std::unique_ptr<Tarxeta>> tarxetas;
    std::vector<std::unique_ptr<FilaOrfo>> orfos;
    struct Titulo { juce::String texto; juce::Rectangle<int> r; int n; };
    std::vector<Titulo> titulos;
    juce::Rectangle<int> faixa;   // o aviso de arriba (sen rede, erro)
    juce::TextButton pecharFaixa;
};

// ------------------------------------------------------------------ as capturas en grande
class Visor : public juce::Component, public juce::KeyListener
{
public:
    explicit Visor (Modelo&);
    void abrir (const xestor::Produto&, int indice);
    void pechar();
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyPressed (const juce::KeyPress& k, juce::Component*) override { return keyPressed (k); }
private:
    void ir (int delta);
    juce::Rectangle<float> zonaImaxe() const;
    Modelo& modelo;
    xestor::Produto produto;
    int indice = 0;
    juce::Image imaxe;
    juce::TextButton anterior, seguinte, pecharBtn;
};

// ------------------------------------------------------------------ a xanela enteira
class Principal : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    explicit Principal (Modelo&);
    ~Principal() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    int altoCompleto (int ancho);   // o alto para que se vexa toda a lista (captura --completa)
    void abrirVisor (const xestor::Produto& p, int i) { visor.abrir (p, i); }

    static constexpr int anchoInicial = 1040, altoInicial = 760;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void actualizar();
    void paintCabeceira (juce::Graphics&);
    void paintBaleiro (juce::Graphics&);
    bool baleiro() const;

    Modelo& modelo;
    aspecto::Look look;
    juce::TooltipWindow tooltips { this, 600 };
    std::vector<std::unique_ptr<juce::TextButton>> formatoBtn;
    std::vector<xestor::Formato> formatos;
    juce::TextButton refrescar, actualizarTodo, volverProbar, novaVersion;
    std::unique_ptr<juce::SharedResourcePointer<escoita::Actualizacions>> versions;   // o aviso das versións do propio xestor
    juce::Viewport vista;
    Lista lista;
    Visor visor;
    juce::Rectangle<int> cabeceira, placa, resumo, zonaFormatos, corpo;
    float pulso = 0.0f;
};
} // namespace vistas

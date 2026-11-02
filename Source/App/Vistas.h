#pragma once
#include "Aspecto.h"
#include "Modelo.h"
#include "Paneis.h"
#include <escoita_nucleo/escoita_nucleo.h>

// A xanela, coma un escaparate: a cabeceira (a icona, as pestanas, o buscador, comprobar e os
// axustes), a banda das actualizacións, as tarxetas de cada plugin (a captura a sangue e a súa
// cor) en reixa, o pé (a hora, os formatos e a versión) e a ficha de cada plugin por riba.
namespace vistas
{
// ------------------------------------------------------------------ unha capa por produto
// Coma a portada dun disco: un degradado coa cor do plugin, a súa interface inclinada en
// perspectiva (que sae pola dereita), o nome arriba e o LED do estado; debaixo, o nome e a versión.
class Tarxeta : public juce::Component
{
public:
    Tarxeta (Modelo&, std::function<void (const xestor::Produto&)> abrirFicha);
    void mostrar (const xestor::Produto&);
    const xestor::Produto& getProduto() const { return produto; }
    static int alto (int ancho);       // o alto (sen a marxe) para este ancho
    static constexpr int marxe = 16;   // arredor, para a sombra e o brillo

    bool animar (float dt);
    void entrar (float atraso);
    void porSobre (bool s) { forzarSobre = s; sobre = s ? 1.0f : 0.0f; resized(); repaint(); }

    std::function<void (Tarxeta*, int dx, int dy)> mover;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool hitTest (int x, int y) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void focusGained (FocusChangeType) override { resized(); repaint(); }
    void focusLost (FocusChangeType) override { resized(); repaint(); }

private:
    juce::Rectangle<float> capa() const;   // o cadrado (subido co rato e na aparición)
    float alzado() const;
    Modelo& modelo;
    xestor::Produto produto;
    std::function<void (const xestor::Produto&)> abrirFicha;
    juce::TextButton accion, mais;
    float sobre = 0.0f, aparicion = 1.0f, atraso = 0.0f, visto = 0.0f;
    bool forzarSobre = false, traballaba = false;
    // a interface deformada, gardada (cambia só co tamaño ou coa escala)
    juce::Image deformada;
    juce::Rectangle<float> caixaDeformada;
    int anchoCache = 0;
    float escalaCache = 0.0f;
    bool tinhaImaxe = false;
};

// ------------------------------------------------------------------ o heroe: un plugin destacado
class Heroe : public juce::Component
{
public:
    Heroe (Modelo&, std::function<void (const xestor::Produto&)> abrirFicha);
    void actualizar();   // escolle os destacados (os que se actualizan, se non os últimos publicados)
    bool animar (float dt);
    void ir (int i);
    bool hai() const { return ! slides.isEmpty(); }
    static int alto (int ancho);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    struct Arte { juce::Image im; int w = 0, h = 0; float escala = 0.0f; };
    const xestor::Produto* produto (int i) const;
    juce::Image arte (int i, float escala);   // o fondo, o halo e a interface en perspectiva (gardado)
    void textos();
    juce::Rectangle<float> zonaPuntos (int i) const;
    bool estreito() const { return getWidth() < 1000; }

    Modelo& modelo;
    std::function<void (const xestor::Produto&)> abrirFicha;
    juce::StringArray slides;   // os id dos produtos
    std::map<juce::String, Arte> artes;
    int actual = 0, anterior = -1;
    float mestura = 1.0f, tempo = 0.0f;
    juce::TextButton principal, novidades, ficha;
    juce::StringArray pedidas;   // as capturas enteiras xa pedidas
};

// ------------------------------------------------------------------ a banda das actualizacións
class Destacado : public juce::Component
{
public:
    explicit Destacado (Modelo&);
    void actualizar();
    bool hai() const;
    int alto (int ancho) const;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    bool estreito() const { return getWidth() < 1000; }
    Modelo& modelo;
    juce::TextButton todo, vella;
    juce::String titulo, subtitulo;
    std::vector<juce::Image> minis;
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
    Lista (Modelo&, std::function<void (const xestor::Produto&)> abrirFicha);
    void actualizar();
    void colocar (int ancho);   // reparte as tarxetas en reixa e cambia o alto
    void paint (juce::Graphics&) override;
    juce::String textoFaixa() const;   // o aviso de arriba (baleiro: non hai)
    bool animar (float dt);
    void entrada();   // a aparición escalonada das tarxetas
    Tarxeta* tarxeta (const juce::String& nome);   // (as escenas)

    int seccion = 0;   // 0 inicio, 1 instrumentos, 2 efectos, 3 paquetes, 4 actualizacións
    juce::String busca;
    bool inicio() const { return seccion == 0 && busca.trim().isEmpty(); }
    bool animarHeroe (float dt) { return heroe.isVisible() && heroe.animar (dt); }
    std::function<void()> alCambiar;   // (os recontos das pestanas)

private:
    void moverFoco (Tarxeta*, int dx, int dy);
    int visibles() const;
    Modelo& modelo;
    std::function<void (const xestor::Produto&)> abrirFicha;
    Heroe heroe;
    Destacado destacado;
    std::vector<std::unique_ptr<Tarxeta>> tarxetas;
    std::vector<std::unique_ptr<FilaOrfo>> orfos;
    struct Titulo { juce::String texto; juce::Rectangle<int> r; int n; bool grande = false; juce::String resumo; };
    std::vector<Titulo> titulos;
    juce::Rectangle<int> faixa, baleiro;   // o aviso de arriba (sen rede, erro); «nada casa»
    juce::TextButton pecharFaixa;
};

// ------------------------------------------------------------------ a ficha dun plugin
class Ficha : public juce::Component
{
public:
    explicit Ficha (Modelo&);
    void abrir (const xestor::Produto&, int captura = 0);
    void pechar();
    void refrescar();   // o modelo cambiou (o estado, o progreso)
    bool estaAberta() const { return isVisible(); }
    const xestor::Produto& getProduto() const { return produto; }
    void porNotas (const juce::String&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void ir (int indice);
    void colocar();
    juce::Rectangle<float> zonaImaxe() const;
    std::vector<juce::Rectangle<float>> zonasMiniaturas() const;
    juce::Image imaxe (int i);
    Modelo& modelo;
    xestor::Produto produto;
    int indice = 0;
    std::map<int, juce::Image> imaxes;
    juce::Image deformada;   // a captura en perspectiva (gardada para este índice e tamaño)
    juce::Rectangle<float> caixaDeformada, zonaDeformada;
    int indiceDeformada = -1;
    float escalaDeformada = 0.0f;
    bool deformadaBoa = false;
    juce::TextButton pecharBtn, anterior, seguinte, accion, manual, presets, mais;
    juce::TextEditor notas;
    std::vector<std::unique_ptr<juce::TextButton>> volver;
    struct FilaVersion { juce::String esquerda, dereita; bool forte = false; std::optional<xestor::Version> volverA; };
    std::vector<FilaVersion> filas;
    juce::StringArray chips;
    juce::String tituloNotas;
    // as zonas (calculadas en colocar)
    juce::Rectangle<int> caixa, esquerda, dereita, zonaChips, zonaNotas, zonaVersions, zonaSerie, zonaTexto;
    int yNome = 0, yDesc = 0, altoDesc = 0;
};

// ------------------------------------------------------------------ a xanela enteira
class Principal : public juce::Component, private juce::ChangeListener, private juce::Timer, private juce::FocusChangeListener
{
public:
    explicit Principal (Modelo&);
    ~Principal() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    int altoCompleto (int ancho);   // o alto para que se vexa toda a lista (captura --completa)
    void abrirFicha (const xestor::Produto& p, int i = 0) { ficha.abrir (p, i); }
    void escena (const juce::String&);   // os paneis, para as capturas
    void visibilityChanged() override;

    static constexpr int anchoInicial = 1280, altoInicial = 840;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void globalFocusChanged (juce::Component*) override;   // co teclado: que se vexa a tecla co foco
    void actualizar();
    void recontos();
    void paintPe (juce::Graphics&);
    void paintBaleiro (juce::Graphics&);
    bool baleiro() const;
    bool dobreFila() const { return getWidth() < 980; }

    Modelo& modelo;
    aspecto::Look look;
    juce::TooltipWindow tooltips { this, 600 };
    std::vector<std::unique_ptr<juce::TextButton>> formatoBtn, pestanaBtn;
    std::vector<xestor::Formato> formatos;
    juce::TextEditor buscador;
    juce::TextButton lupa, refrescar, axustes, volverProbar, novaVersion;
    struct Barra : juce::Component
    {
        Principal& p;
        explicit Barra (Principal& pp) : p (pp) { setInterceptsMouseClicks (false, false); }
        void paint (juce::Graphics&) override;
    } barra { *this };
    bool buscando = false;
    int ultimoY = -1;
    void abrirBusca (bool);
    juce::Viewport vista;
    Lista lista;
    Ficha ficha;
    paneis::Modal modal;
    juce::Rectangle<int> logo, pe, corpo, zonaBusca;
    float pulso = 0.0f;
    juce::uint32 ultimoTic = 0;
    bool xaEntrou = false;
};
} // namespace vistas

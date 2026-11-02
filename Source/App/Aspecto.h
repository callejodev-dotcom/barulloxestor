#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

// O aspecto da familia Barullo (o mesmo dos editores dos plugins: Barullo Ley, ADT, Tune...):
// o azul mariño da noite de Vigo, a crema da letra de Escoita e a luz laranxa, nun aparello de
// estudio (paneis en relevo, pantallas de cristal, visores dourados, teclas redondeadas con LED).
namespace aspecto
{
namespace col
{
    const juce::Colour noite    { 0xff0c111f };
    const juce::Colour crema    { 0xfffff7c1 };
    const juce::Colour crema2   { 0xffbdb792 };
    const juce::Colour crema3   { 0xff857f67 };
    const juce::Colour laranxa  { 0xffe08b16 };
    const juce::Colour ouro     { 0xffedba23 };
    const juce::Colour verde    { 0xff9dc6a0 };
    const juce::Colour azul     { 0xff8fb0e8 };
    const juce::Colour panel1   { 0xff1c2540 }, panel2 { 0xff111830 };
    const juce::Colour tecla1   { 0xff27314f }, tecla2 { 0xff1a2239 };
    const juce::Colour visor    { 0xff070b16 };
    const juce::Colour cristal1 { 0xff142046 }, cristal2 { 0xff070c1c };
    const juce::Colour lina     { 0xff2a3350 };
}

// a cor propia de cada plugin (sempre a mesma para o mesmo nome)
juce::Colour acento (const juce::String& nome);
// as oito luces da familia, na orde do arco da icona
const std::vector<juce::Colour>& coresFamilia();

// Os tamaños da maqueta (CSS, en px) nas fontes de JUCE (que miden o alto enteiro, non o «em»)
inline float css (float px) { return px * 1.21f; }

juce::Font body (float size);    // Inter
juce::Font semi (float size);    // Montserrat SemiBold
juce::Font cond (float size);    // Montserrat Bold
juce::Font caps (float size, float tracking = 0.18f);
juce::String upper (const juce::String&);
float anchoTexto (const juce::Font&, const juce::String&);
void drawCaps (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, float size, float tracking = 0.18f);
void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, juce::Colour);
void paintPanel (juce::Graphics&, juce::Rectangle<float>, float radius);
void paintWell (juce::Graphics&, juce::Rectangle<float>, float radius);
void paintGlass (juce::Graphics&, juce::Rectangle<float>, float radius);
void paintKey (juce::Graphics&, juce::Rectangle<float>, bool over, bool down);
void paintLed (juce::Graphics&, juce::Point<float>, bool on, juce::Colour = col::laranxa);
void groupTitle (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour = col::laranxa);
// «ESCOITA.» co punto laranxa (devolve o ancho)
float marcaEscoita (juce::Graphics&, juce::Rectangle<float>, float size, float tracking, juce::Colour);
// unha frecha en arco (refrescar)
void paintRefrescar (juce::Graphics&, juce::Rectangle<float>, juce::Colour);
// as iconas pequenas das teclas: a roda dos axustes, a lupa, o visto
void paintEngrenaxe (juce::Graphics&, juce::Rectangle<float>, juce::Colour);
void paintLupa (juce::Graphics&, juce::Rectangle<float>, juce::Colour);
void paintVisto (juce::Graphics&, juce::Rectangle<float>, juce::Colour, float grosor = 2.0f);

// ---------------------------------------------------------------- o escaparate
// o fondo: a noite cun brillo azul arriba á esquerda e un morno abaixo á dereita (e un gran fino)
void paintFondo (juce::Graphics&, juce::Rectangle<float>);
// unha sombra suave (capas) baixo un rectángulo redondeado
void paintSombra (juce::Graphics&, juce::Rectangle<float>, float radio, juce::Colour, float difuso, float baixa);
// unha pastilla escura cun LED e un texto (o estado na captura)
void paintPastilla (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour, bool led = true);
float anchoPastilla (const juce::String&);
// unha imaxe recortada para encher (desde arriba á esquerda), cun zoom opcional
void paintCuberta (juce::Graphics&, const juce::Image&, juce::Rectangle<float>, float zoom = 1.0f, float opacidade = 1.0f);

// ---------------------------------------------------------------- a perspectiva (coma o CSS 3D)
// Un rectángulo w×h (centrado na orixe) xirado (rotateY, rotateX, rotateZ, en graos) e visto cunha
// perspectiva «distancia» (px). JUCE non ten perspectiva: a imaxe deformase píxel a píxel (unha
// homografía exacta con mostraxe bilineal) e gárdase, así non custa nada en cada fotograma.
struct Perspectiva
{
    float rotY = 0.0f, rotX = 0.0f, rotZ = 0.0f, distancia = 1600.0f;
};
// as catro esquinas proxectadas (arriba-esq., arriba-der., abaixo-der., abaixo-esq.), relativas ao centro
std::array<juce::Point<float>, 4> esquinas (float w, float h, const Perspectiva&);
// A imaxe deformada (en píxeles físicos: «escala» por punto lóxico), cos cantos redondeados «radio»
// (en puntos). «caixa» devolve onde vai, en puntos e relativa ao centro do rectángulo.
juce::Image deformar (const juce::Image& src, float w, float h, const Perspectiva&, float escala, float radio,
                      juce::Rectangle<float>& caixa);
// unha imaxe reducida con calidade (a metades sucesivas e despois ao tamaño)
juce::Image reducir (const juce::Image&, int ancho);

// ---------------------------------------------------------------- a icona
// O mando da familia: o arco de segmentos coas oito cores, o disco e a agulla. En «r» (cadrado)
// como a icona de 1024 (coa placa); «monocromo»: só o mando, en negro (a barra de menús do Mac).
void paintMandoFamilia (juce::Graphics&, juce::Rectangle<float> r, bool placa, bool monocromo = false);
// O cargador: o mando da familia coma se alguén o xirase. A agulla vai e vén suave polo arco e os
// segmentos acéndense ao seu paso (cun brillo), un tras outro. «t» en segundos; «movese» false: quieto.
void paintCargador (juce::Graphics&, juce::Rectangle<float> r, double t, bool movese = true);
// o «t» de agora (para animar): segundos desde que arrancou a app
double tempoCargador();
// a icona da app (Resources/Icona.png), sen o marxe transparente
juce::Image iconaApp();

// Propiedades das teclas (TextButton):
//   «primaria»: a acción principal (ámbar); «icona» = "refrescar" | "axustes" | "mais" | "pechar" | "anterior" | "seguinte"
//   «estilo» = "contorno" (só o bordo e o texto na cor «cor») | "fantasma" | "pestana" (co «reconto») | "chip"
//              (os formatos do pé, acesos co toggle) | "aviso" (ouro, cun LED) | "ligazon" (só o texto)
//   «led»: a cor do aro do foco; «fontSize»: o tamaño do texto (CSS)
struct Look : juce::LookAndFeel_V4
{
    // o ancho que precisa unha tecla co seu texto (para colocalas)
    static int anchoTecla (const juce::TextButton&, int marxe = 32);
    Look();
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
    juce::Font getPopupMenuFont() override { return body (14.5f); }
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart,
                        int thumbSize, bool over, bool down) override;
    int getDefaultScrollbarWidth() override { return 10; }
    juce::Font getAlertWindowMessageFont() override { return body (14.5f); }
    juce::Font getAlertWindowTitleFont() override { return cond (17.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return semi (css (13.0f)); }
    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>&, bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
    int getPopupMenuBorderSize() override { return 6; }
    void drawAlertBox (juce::Graphics&, juce::AlertWindow&, const juce::Rectangle<int>& textArea, juce::TextLayout&) override;
};
} // namespace aspecto

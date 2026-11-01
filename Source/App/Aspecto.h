#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

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

// Propiedades das teclas (TextButton): «led» = cor do LED, «noLed», «primaria» (a acción principal,
// co LED aceso na cor do plugin), «gold» (visor dourado), «icona» = "refrescar" | "mais" | "pechar"
struct Look : juce::LookAndFeel_V4
{
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
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return caps (10.0f, 0.15f); }
    void drawAlertBox (juce::Graphics&, juce::AlertWindow&, const juce::Rectangle<int>& textArea, juce::TextLayout&) override;
};
} // namespace aspecto

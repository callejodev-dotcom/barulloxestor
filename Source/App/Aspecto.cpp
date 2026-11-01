#include "Aspecto.h"
#include "BinaryData.h"
#include <escoita_nucleo/escoita_nucleo.h>

namespace aspecto
{
// ================================================================ cor de cada plugin
// Unha paleta curta que vive ben co azul mariño: as luces da familia e catro máis.
juce::Colour acento (const juce::String& nome)
{
    static const juce::Colour paleta[] = {
        juce::Colour (0xffe08b16),   // laranxa
        juce::Colour (0xff8fb0e8),   // azul
        juce::Colour (0xff9dc6a0),   // verde
        juce::Colour (0xffedba23),   // ouro
        juce::Colour (0xffe58fa6),   // rosa
        juce::Colour (0xff6fc6c0),   // turquesa
        juce::Colour (0xffb59de0),   // lila
        juce::Colour (0xffe0735a),   // vermello
    };
    juce::uint32 h = 2166136261u;
    for (auto* p = nome.toRawUTF8(); *p != 0; ++p) { h ^= (juce::uint8) *p; h *= 16777619u; }
    return paleta[h % (sizeof (paleta) / sizeof (paleta[0]))];
}

// ================================================================ tipografía
// Montserrat (títulos e rótulos, en maiúsculas espaciadas coma na web de Escoita) e Inter
// (texto corrido): as da familia Barullo. As dúas, licenza OFL.
static juce::Typeface::Ptr tf (int which)
{
    static juce::Typeface::Ptr t[3] = {
        juce::Typeface::createSystemTypefaceFor (BinaryData::InterMedium_ttf, BinaryData::InterMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize) };
    return t[which];
}
juce::Font body (float s) { return juce::Font (juce::FontOptions (tf (0)).withHeight (s)); }
juce::Font semi (float s) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s)); }
juce::Font cond (float s) { return juce::Font (juce::FontOptions (tf (2)).withHeight (s)); }
juce::Font caps (float s, float tracking) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s).withKerningFactor (tracking)); }
juce::String upper (const juce::String& t) { return escoita::maiusculas (t); }
float anchoTexto (const juce::Font& f, const juce::String& t) { return juce::GlyphArrangement::getStringWidth (f, t); }

void drawCaps (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, float size, float tracking)
{
    g.setFont (caps (size, tracking));
    g.drawText (upper (t), r, j, false);
}

void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, juce::Colour c)
{
    g.setColour (c.withAlpha (0.16f));
    for (const auto d : { juce::Point<float> (-0.8f, 0.0f), { 0.8f, 0.0f }, { 0.0f, -0.8f }, { 0.0f, 0.8f } })
        g.drawText (t, r.translated (d.x, d.y), j, false);
    g.setColour (c);
    g.drawText (t, r, j, false);
}

// ================================================================ pezas do aparello
void paintPanel (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.11f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f * (float) i).expanded ((float) i), rad + (float) i);
    }
    g.setGradientFill (juce::ColourGradient (col::panel1, 0.0f, r.getY(), col::panel2, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRect (r.getX() + rad, r.getY() + 0.5f, r.getWidth() - 2.0f * rad, 1.0f);
}

void paintWell (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
    g.setColour (col::visor.brighter (0.06f));
    g.fillRoundedRectangle (r, rad);
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.65f), 0.0f, r.getY(), juce::Colours::transparentBlack, 0.0f, r.getY() + 5.0f, false));
    g.fillRect (r.withHeight (6.0f));
    g.restoreState();
}

void paintGlass (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
    juce::ColourGradient gr (col::cristal1, r.getCentreX(), r.getY(), col::cristal2, r.getCentreX(), r.getBottom(), false);
    gr.addColour (0.55, juce::Colour (0xff0a1129));
    g.setGradientFill (gr);
    g.fillRoundedRectangle (r, rad);
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    for (int i = 0; i < 5; ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.16f - 0.03f * (float) i));
        g.drawRoundedRectangle (r.reduced ((float) i + 0.5f), rad, 1.0f);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), r.getX(), r.getY(),
                                             juce::Colours::transparentWhite, r.getX() + r.getWidth() * 0.32f, r.getY() + r.getHeight() * 0.55f, false));
    g.fillRect (r);
    g.restoreState();
}

void paintKey (juce::Graphics& g, juce::Rectangle<float> r, bool over, bool down)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 7.0f);
    if (down) r.translate (0.0f, 1.0f);
    const float lift = over && ! down ? 0.10f : 0.0f;
    g.setGradientFill (juce::ColourGradient (col::tecla1.brighter (lift), 0.0f, r.getY(), col::tecla2.brighter (lift * 0.6f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (juce::Colours::white.withAlpha (down ? 0.04f : 0.10f));
    g.fillRect (r.getX() + 6.0f, r.getY() + 0.5f, r.getWidth() - 12.0f, 1.0f);
}

void paintLed (juce::Graphics& g, juce::Point<float> c, bool on, juce::Colour luz)
{
    if (on)
    {
        g.setColour (luz.withAlpha (0.16f)); g.fillEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
        g.setColour (luz.withAlpha (0.34f)); g.fillEllipse (c.x - 4.6f, c.y - 4.6f, 9.2f, 9.2f);
        g.setColour (luz); g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f)); g.fillEllipse (c.x - 1.3f, c.y - 2.0f, 2.2f, 2.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff3a3326)); g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f, 0.8f);
    }
}

void groupTitle (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, juce::Colour c)
{
    g.setColour (c);
    drawCaps (g, t, r.withHeight (14.0f), juce::Justification::centredLeft, 9.0f, 0.28f);
    const float w = anchoTexto (caps (9.0f, 0.28f), upper (t)) + 14.0f;
    g.setColour (c.withAlpha (0.22f));
    g.fillRect (r.getX() + w, r.getY() + 7.0f, r.getWidth() - w, 1.0f);
}

float marcaEscoita (juce::Graphics& g, juce::Rectangle<float> r, float size, float tracking, juce::Colour c)
{
    const float we = anchoTexto (caps (size, tracking), "ESCOITA");
    g.setColour (c);
    drawCaps (g, "ESCOITA", r, juce::Justification::centredLeft, size, tracking);
    g.setColour (col::laranxa);
    drawCaps (g, ".", r.withTrimmedLeft (we - 1.5f), juce::Justification::centredLeft, size, tracking);
    return we;
}

void paintRefrescar (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    const auto ctr = r.getCentre();
    const float rad = 6.5f;
    juce::Path arc;
    arc.addCentredArc (ctr.x, ctr.y, rad, rad, 0.0f, 0.5f, juce::MathConstants<float>::twoPi - 0.2f, true);
    g.setColour (c);
    g.strokePath (arc, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float a1 = juce::MathConstants<float>::twoPi - 0.2f;
    const juce::Point<float> tip (ctr.x + rad * std::sin (a1), ctr.y - rad * std::cos (a1));
    juce::Path head;
    head.addTriangle (tip.x + 4.0f, tip.y, tip.x - 1.5f, tip.y - 4.0f, tip.x - 1.5f, tip.y + 4.0f);
    g.fillPath (head);
}

// ================================================================ aspecto
Look::Look()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::noite);
    setColour (juce::DocumentWindow::textColourId, col::crema);
    setColour (juce::Label::textColourId, col::crema);
    setColour (juce::TextEditor::backgroundColourId, col::visor);
    setColour (juce::TextEditor::textColourId, col::ouro);
    setColour (juce::TextEditor::outlineColourId, col::lina);
    setColour (juce::TextEditor::focusedOutlineColourId, col::laranxa);
    setColour (juce::PopupMenu::backgroundColourId, col::panel2);
    setColour (juce::PopupMenu::textColourId, col::crema);
    setColour (juce::PopupMenu::headerTextColourId, col::laranxa);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::laranxa);
    setColour (juce::PopupMenu::highlightedTextColourId, col::noite);
    setColour (juce::TextButton::buttonColourId, col::tecla2);
    setColour (juce::TextButton::textColourOffId, col::crema2);
    setColour (juce::TextButton::textColourOnId, col::crema);
    setColour (juce::AlertWindow::backgroundColourId, col::panel2);
    setColour (juce::AlertWindow::textColourId, col::crema);
    setColour (juce::AlertWindow::outlineColourId, col::lina);
    setColour (juce::ScrollBar::thumbColourId, col::crema3);
    setColour (juce::CaretComponent::caretColourId, col::laranxa);
}

juce::Typeface::Ptr Look::getTypefaceForFont (const juce::Font& f) { return tf (f.isBold() ? 1 : 0); }

static juce::Colour corProp (juce::Button& b, const char* k, juce::Colour porDefecto)
{
    return b.getProperties().contains (k) ? juce::Colour ((juce::uint32) (juce::int64) b.getProperties()[k]) : porDefecto;
}

// as teclas: en relevo, cun LED (aceso se está escollida, ou sempre na acción principal) e o texto
// en maiúsculas espaciadas; co foco do teclado, un aro na cor do LED
void Look::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (1.5f);
    const auto luz = corProp (b, "led", col::laranxa);
    if (b.getProperties().contains ("gold"))
    {
        paintWell (g, r, 8.0f);
        if (over) { g.setColour (col::laranxa.withAlpha (0.08f)); g.fillRoundedRectangle (r, 8.0f); }
    }
    else
    {
        const bool primaria = b.getProperties().contains ("primaria");
        paintKey (g, r, over && b.isEnabled(), down);
        if (primaria && b.isEnabled())
        {
            // a acción principal: un bordo morno na cor do plugin
            g.setColour (luz.withAlpha (over ? 0.55f : 0.38f));
            g.drawRoundedRectangle (r.translated (0.0f, down ? 1.0f : 0.0f).reduced (0.5f), 7.0f, 1.0f);
        }
        if (down) r.translate (0.0f, 1.0f);
        if (! b.getProperties().contains ("noLed") && ! b.getProperties().contains ("icona"))
            paintLed (g, { r.getX() + 13.0f, r.getCentreY() }, (primaria && b.isEnabled()) || b.getToggleState(), luz);
    }
    if (b.hasKeyboardFocus (false))
    {
        g.setColour (luz.withAlpha (0.9f));
        g.drawRoundedRectangle (b.getLocalBounds().toFloat().reduced (0.5f), 8.0f, 1.6f);
    }
}

void Look::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (1.5f);
    if (b.getProperties().contains ("gold"))
    {
        g.setFont (caps (11.0f, 0.12f));
        glowText (g, upper (b.getButtonText()), r.reduced (12.0f, 0.0f), juce::Justification::centred, col::ouro);
        return;
    }
    if (down) r.translate (0.0f, 1.0f);
    juce::Colour c = b.getToggleState() || over || b.getProperties().contains ("primaria") ? col::crema : col::crema2;
    if (! b.isEnabled()) c = col::crema3.withAlpha (0.6f);
    g.setColour (c);
    const auto icona = b.getProperties()["icona"].toString();
    if (icona == "refrescar") { paintRefrescar (g, r, c); return; }
    if (icona == "mais")
    {
        for (int i = -1; i <= 1; ++i) g.fillEllipse (r.getCentreX() + (float) i * 6.0f - 1.7f, r.getCentreY() - 1.7f, 3.4f, 3.4f);
        return;
    }
    if (icona == "pechar")
    {
        const auto c0 = r.getCentre();
        g.drawLine (c0.x - 5.0f, c0.y - 5.0f, c0.x + 5.0f, c0.y + 5.0f, 1.8f);
        g.drawLine (c0.x + 5.0f, c0.y - 5.0f, c0.x - 5.0f, c0.y + 5.0f, 1.8f);
        return;
    }
    if (icona == "anterior" || icona == "seguinte")
    {
        const float s = icona == "anterior" ? -1.0f : 1.0f;
        const auto c0 = r.getCentre();
        juce::Path p;
        p.startNewSubPath (c0.x - 2.5f * s, c0.y - 6.0f);
        p.lineTo (c0.x + 3.5f * s, c0.y);
        p.lineTo (c0.x - 2.5f * s, c0.y + 6.0f);
        g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }
    if (! b.getProperties().contains ("noLed")) r.removeFromLeft (16.0f);
    g.setFont (caps ((float) b.getProperties().getWithDefault ("fontSize", 9.5f), 0.15f));
    g.drawFittedText (upper (b.getButtonText()), r.toNearestInt().reduced (4, 0), juce::Justification::centred, 1, 0.75f);
}

void Look::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    auto r = juce::Rectangle<int> (w, h).toFloat();
    g.setColour (col::panel2); g.fillRoundedRectangle (r, 6.0f);
    g.setColour (col::lina); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    juce::AttributedString s; s.setJustification (juce::Justification::topLeft);
    s.append (text, body (13.5f), col::crema);
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    tl.draw (g, r.reduced (10.0f, 7.0f));
}

juce::Rectangle<int> Look::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    juce::AttributedString s; s.append (text, body (13.5f));
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    const int w = (int) tl.getWidth() + 22, h = (int) tl.getHeight() + 16;
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 12,
                                 pos.y > parent.getCentreY() ? pos.y - (h + 8) : pos.y + 18, w, h).constrainedWithin (parent);
}

void Look::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::panel2);
    g.setColour (col::lina); g.drawRect (0, 0, w, h);
}

void Look::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool over, bool down)
{
    auto t = vertical ? juce::Rectangle<int> (x, thumbStart, w, thumbSize) : juce::Rectangle<int> (thumbStart, y, thumbSize, h);
    g.setColour (col::crema3.withAlpha (over || down ? 0.55f : 0.28f));
    g.fillRoundedRectangle (t.toFloat().reduced (3.0f, 2.0f), 2.5f);
}

void Look::drawAlertBox (juce::Graphics& g, juce::AlertWindow& a, const juce::Rectangle<int>& textArea, juce::TextLayout& tl)
{
    auto r = a.getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (col::panel1, 0.0f, 0.0f, col::panel2, 0.0f, r.getBottom(), false));
    g.fillRect (r);
    g.setColour (col::lina);
    g.drawRect (r, 1.0f);
    tl.draw (g, textArea.toFloat());
}
} // namespace aspecto

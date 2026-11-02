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

const std::vector<juce::Colour>& coresFamilia()
{
    static const std::vector<juce::Colour> c {
        juce::Colour (0xffe08b16), juce::Colour (0xffedba23), juce::Colour (0xff9dc6a0), juce::Colour (0xff6fc6c0),
        juce::Colour (0xff8fb0e8), juce::Colour (0xffb59de0), juce::Colour (0xffe58fa6), juce::Colour (0xffe0735a) };
    return c;
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


void paintEngrenaxe (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    // unha roda dentada: oito dentes arredor dun anel
    const auto ctr = r.getCentre();
    const float ro = 7.6f, ri = 5.6f;
    juce::Path p;
    const int n = 8;
    for (int i = 0; i < n * 4; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * ((float) i - 0.5f) / (float) (n * 4);
        const bool fora = (i % 4) == 1 || (i % 4) == 2;
        const float rr = fora ? ro : ri;
        const juce::Point<float> pt (ctr.x + rr * std::sin (a), ctr.y - rr * std::cos (a));
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    p.closeSubPath();
    p.addEllipse (ctr.x - 2.6f, ctr.y - 2.6f, 5.2f, 5.2f);
    p.setUsingNonZeroWinding (false);
    g.setColour (c);
    g.fillPath (p);
}

void paintLupa (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    const auto ctr = r.getCentre().translated (-1.5f, -1.5f);
    g.setColour (c);
    g.drawEllipse (ctr.x - 5.0f, ctr.y - 5.0f, 10.0f, 10.0f, 1.6f);
    g.drawLine (ctr.x + 3.7f, ctr.y + 3.7f, ctr.x + 7.5f, ctr.y + 7.5f, 1.8f);
}

void paintVisto (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float grosor)
{
    const auto ctr = r.getCentre();
    const float s = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    juce::Path p;
    p.startNewSubPath (ctr.x - s * 0.55f, ctr.y + s * 0.02f);
    p.lineTo (ctr.x - s * 0.15f, ctr.y + s * 0.42f);
    p.lineTo (ctr.x + s * 0.6f, ctr.y - s * 0.4f);
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (grosor, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ================================================================ o escaparate
void paintFondo (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (col::noite);
    g.fillRect (r);
    // un brillo elíptico: un degradado circular estirado
    auto brillo = [&] (juce::Point<float> c, float rx, float ry, juce::Colour cor, float ata)
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::scale (1.0f, ry / rx, c.x, c.y));
        juce::ColourGradient gr (cor, c.x, c.y, cor.withAlpha (0.0f), c.x + rx, c.y, true);
        gr.addColour (ata * 0.5, cor.withMultipliedAlpha (0.55f));
        gr.addColour (ata, cor.withAlpha (0.0f));
        g.setGradientFill (gr);
        g.fillRect (juce::Rectangle<float> (c.x - rx, c.y - rx, rx * 2.0f, rx * 2.0f));
    };
    brillo ({ r.getX() + r.getWidth() * 0.2f, r.getY() - r.getHeight() * 0.1f }, 1200.0f, 600.0f, juce::Colour (0xff1a2550), 0.6f);
    brillo ({ r.getX() + r.getWidth() * 1.1f, r.getY() + r.getHeight() * 1.1f }, 900.0f, 500.0f, juce::Colour (0xff2a1d10), 0.55f);
    // o gran: unha textura de ruído moi tenue, sempre a mesma
    static const juce::Image gran = []
    {
        juce::Image im (juce::Image::ARGB, 160, 160, true);
        juce::Random rnd (1966);
        for (int y = 0; y < 160; ++y)
            for (int x = 0; x < 160; ++x)
            {
                const auto v = (juce::uint8) rnd.nextInt (256);
                im.setPixelAt (x, y, juce::Colour (v, v, v, (juce::uint8) 9));
            }
        return im;
    }();
    g.setTiledImageFill (gran, 0, 0, 1.0f);
    g.fillRect (r);
}

void paintSombra (juce::Graphics& g, juce::Rectangle<float> r, float radio, juce::Colour c, float difuso, float baixa)
{
    const int capas = 7;
    const float a = c.getFloatAlpha() / (float) capas * 1.6f;
    for (int i = capas; i >= 1; --i)
    {
        const float t = (float) i / (float) capas;
        g.setColour (c.withAlpha (a * (1.0f - t * 0.75f)));
        g.fillRoundedRectangle (r.translated (0.0f, baixa * (0.4f + 0.6f * t)).expanded (difuso * t * 0.6f), radio + difuso * t * 0.6f);
    }
}

float anchoPastilla (const juce::String& t)
{
    return anchoTexto (semi (css (10.5f)).withExtraKerningFactor (0.06f), t) * 1.06f + 18.0f + 13.0f + 4.0f;
}

void paintPastilla (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, juce::Colour c, bool led)
{
    g.setColour (juce::Colour (0xbf070b16));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);
    auto in = r.reduced (9.0f, 0.0f);
    if (led)
    {
        const auto cc = juce::Point<float> (in.getX() + 3.5f, in.getCentreY());
        g.setColour (c.withAlpha (0.35f));
        g.fillEllipse (cc.x - 5.0f, cc.y - 5.0f, 10.0f, 10.0f);
        g.setColour (c);
        g.fillEllipse (cc.x - 3.5f, cc.y - 3.5f, 7.0f, 7.0f);
        in.removeFromLeft (13.0f);
    }
    g.setColour (c);
    g.setFont (semi (css (10.5f)).withExtraKerningFactor (0.06f));
    g.drawText (t, in, juce::Justification::centredLeft, false);
}

void paintCuberta (juce::Graphics& g, const juce::Image& im, juce::Rectangle<float> r, float zoom, float opacidade)
{
    if (! im.isValid()) return;
    // «cover» desde arriba á esquerda: a escala que enche, e o zoom desde o centro da zona
    const float e = juce::jmax (r.getWidth() / (float) im.getWidth(), r.getHeight() / (float) im.getHeight()) * zoom;
    const float dx = (r.getWidth() - r.getWidth() * zoom) * 0.5f, dy = (r.getHeight() - r.getHeight() * zoom) * 0.5f;
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (r.toNearestIntEdges());
    g.setOpacity (opacidade);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImageTransformed (im, juce::AffineTransform::scale (e).translated (r.getX() + dx, r.getY() + dy));
}


// ================================================================ a perspectiva
std::array<juce::Point<float>, 4> esquinas (float w, float h, const Perspectiva& p)
{
    const float ry = juce::degreesToRadians (p.rotY), rx = juce::degreesToRadians (p.rotX), rz = juce::degreesToRadians (p.rotZ);
    auto proxectar = [&] (float x, float y)
    {
        // coma no CSS: primeiro rotateZ, despois rotateX e rotateY (a lista aplícase de dereita a esquerda);
        // o eixo y vai cara abaixo e o z cara a quen mira
        float x1 = x * std::cos (rz) - y * std::sin (rz), y1 = x * std::sin (rz) + y * std::cos (rz), z1 = 0.0f;
        const float y2 = y1 * std::cos (rx) - z1 * std::sin (rx), z2 = y1 * std::sin (rx) + z1 * std::cos (rx);
        const float x3 = x1 * std::cos (ry) + z2 * std::sin (ry), z3 = -x1 * std::sin (ry) + z2 * std::cos (ry);
        const float k = p.distancia / (p.distancia - z3);
        return juce::Point<float> (x3 * k, y2 * k);
    };
    return { proxectar (-w * 0.5f, -h * 0.5f), proxectar (w * 0.5f, -h * 0.5f), proxectar (w * 0.5f, h * 0.5f), proxectar (-w * 0.5f, h * 0.5f) };
}

juce::Image reducir (const juce::Image& im, int ancho)
{
    if (! im.isValid() || ancho <= 0) return {};
    auto r = im.convertedToFormat (juce::Image::ARGB);
    while (r.getWidth() > ancho * 2)
        r = r.rescaled (r.getWidth() / 2, juce::jmax (1, r.getHeight() / 2), juce::Graphics::highResamplingQuality);
    const int alto = juce::jmax (1, juce::roundToInt ((float) ancho * (float) im.getHeight() / (float) im.getWidth()));
    if (r.getWidth() != ancho) r = r.rescaled (ancho, alto, juce::Graphics::highResamplingQuality);
    return r;
}

juce::Image deformar (const juce::Image& src, float w, float h, const Perspectiva& p, float escala, float radio, juce::Rectangle<float>& caixa)
{
    const auto q = esquinas (w, h, p);
    caixa = juce::Rectangle<float>::findAreaContainingPoints (q.data(), 4);
    if (! src.isValid()) return {};
    // a orixe, á escala física e axustada a píxeles enteiros
    caixa = juce::Rectangle<float> (std::floor (caixa.getX() * escala) / escala, std::floor (caixa.getY() * escala) / escala,
                                    std::ceil (caixa.getWidth() * escala + 2.0f) / escala, std::ceil (caixa.getHeight() * escala + 2.0f) / escala);
    const int W = juce::roundToInt (caixa.getWidth() * escala), H = juce::roundToInt (caixa.getHeight() * escala);
    // a fonte, ao tamaño que vai ter (máis ou menos) no destino
    const auto fonte = reducir (src, juce::jmax (2, juce::roundToInt (w * escala)));
    const int sw = fonte.getWidth(), sh = fonte.getHeight();

    // a homografía do cadrado unidade ao cuadrilátero (Heckbert), en píxeles físicos do destino
    double x[4], y[4];
    for (int i = 0; i < 4; ++i) { x[i] = (q[(size_t) i].x - caixa.getX()) * escala; y[i] = (q[(size_t) i].y - caixa.getY()) * escala; }
    const double dx1 = x[1] - x[2], dx2 = x[3] - x[2], dx3 = x[0] - x[1] + x[2] - x[3];
    const double dy1 = y[1] - y[2], dy2 = y[3] - y[2], dy3 = y[0] - y[1] + y[2] - y[3];
    const double den = dx1 * dy2 - dx2 * dy1;
    const double G = std::abs (den) < 1e-12 ? 0.0 : (dx3 * dy2 - dx2 * dy3) / den;
    const double Hh = std::abs (den) < 1e-12 ? 0.0 : (dx1 * dy3 - dx3 * dy1) / den;
    const double A = x[1] - x[0] + G * x[1], B = x[3] - x[0] + Hh * x[3], C = x[0];
    const double D = y[1] - y[0] + G * y[1], E = y[3] - y[0] + Hh * y[3], F = y[0];
    // a inversa (a adxunta abonda: o factor común desaparece ao dividir)
    const double i00 = E - F * Hh, i01 = C * Hh - B, i02 = B * F - C * E;
    const double i10 = F * G - D, i11 = A - C * G, i12 = C * D - A * F;
    const double i20 = D * Hh - E * G, i21 = B * G - A * Hh, i22 = A * E - B * D;

    juce::Image out (juce::Image::ARGB, juce::jmax (1, W), juce::jmax (1, H), true);
    const juce::Image::BitmapData so (fonte, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData de (out, juce::Image::BitmapData::writeOnly);
    const float rr = radio * escala;
    for (int py = 0; py < H; ++py)
    {
        auto* fila = reinterpret_cast<juce::PixelARGB*> (de.getLinePointer (py));
        for (int px = 0; px < W; ++px)
        {
            const double X = px + 0.5, Y = py + 0.5;
            const double ww = i20 * X + i21 * Y + i22;
            if (std::abs (ww) < 1e-12) continue;
            const double u = (i00 * X + i01 * Y + i02) / ww, v = (i10 * X + i11 * Y + i12) / ww;
            const float sx = (float) (u * sw), sy = (float) (v * sh);
            if (sx < -1.0f || sy < -1.0f || sx > (float) sw + 1.0f || sy > (float) sh + 1.0f) continue;
            // a cobertura: a distancia ao bordo redondeado (en píxeles da fonte, que van case 1:1)
            const float qx = juce::jmax (std::abs (sx - (float) sw * 0.5f) - ((float) sw * 0.5f - rr), 0.0f);
            const float qy = juce::jmax (std::abs (sy - (float) sh * 0.5f) - ((float) sh * 0.5f - rr), 0.0f);
            const float dist = std::sqrt (qx * qx + qy * qy) - rr
                             + juce::jmin (juce::jmax (std::abs (sx - (float) sw * 0.5f) - ((float) sw * 0.5f - rr), std::abs (sy - (float) sh * 0.5f) - ((float) sh * 0.5f - rr)), 0.0f);
            const float cob = juce::jlimit (0.0f, 1.0f, 0.5f - dist);
            if (cob <= 0.0f) continue;
            // bilineal
            const float fx = juce::jlimit (0.0f, (float) sw - 1.001f, sx - 0.5f), fy = juce::jlimit (0.0f, (float) sh - 1.001f, sy - 0.5f);
            const int x0 = (int) fx, y0 = (int) fy;
            const float ax = fx - (float) x0, ay = fy - (float) y0;
            auto px00 = so.getPixelColour (x0, y0), px10 = so.getPixelColour (x0 + 1, y0);
            auto px01 = so.getPixelColour (x0, y0 + 1), px11 = so.getPixelColour (x0 + 1, y0 + 1);
            auto mestura = [] (juce::Colour a, juce::Colour b, float t) { return a.interpolatedWith (b, t); };
            const auto c = mestura (mestura (px00, px10, ax), mestura (px01, px11, ax), ay);
            fila[px].set (juce::PixelARGB (c.withMultipliedAlpha (cob).getPixelARGB()));
        }
    }
    return out;
}

// ================================================================ a icona
void paintMandoFamilia (juce::Graphics& g, juce::Rectangle<float> r, bool placa, bool monocromo)
{
    // as medidas da icona aprobada (icona2.svg, en 1024)
    const float k = r.getWidth() / 1024.0f;
    auto P = [&] (float x, float y) { return juce::Point<float> (r.getX() + x * k, r.getY() + y * k); };
    const auto c = P (512.0f, 512.0f);
    if (placa && ! monocromo)
    {
        const auto pr = juce::Rectangle<float> (P (100.0f, 100.0f), P (924.0f, 924.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff232c4d), 0.0f, pr.getY(), juce::Colour (0xff0e1325), 0.0f, pr.getBottom(), false));
        g.fillRoundedRectangle (pr, 185.0f * k);
        g.setColour (juce::Colour (0xff2f3a5e));
        g.drawRoundedRectangle (pr.reduced (1.5f * k), 185.0f * k, 3.0f * k);
    }
    // o arco: 270° en oito segmentos (empeza abaixo á esquerda, coma un mando)
    const float ra = 275.0f * k;
    const float a0 = juce::MathConstants<float>::pi * 1.25f;
    const float seg = 134.0f / 275.0f, paso = 162.0f / 275.0f;
    const auto& cores = coresFamilia();
    if (! monocromo)
    {
        // o brillo do primeiro (a luz laranxa da familia)
        juce::Path p;
        p.addCentredArc (c.x, c.y, ra, ra, 0.0f, a0, a0 + seg, true);
        for (auto [w, a] : { std::pair<float, float> { 90.0f, 0.10f }, { 64.0f, 0.18f } })
        {
            g.setColour (cores[0].withAlpha (a));
            g.strokePath (p, juce::PathStrokeType (w * k, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
    for (size_t i = 0; i < cores.size(); ++i)
    {
        const float a = a0 + (float) i * paso;
        juce::Path p;
        p.addCentredArc (c.x, c.y, ra, ra, 0.0f, a, a + seg, true);
        g.setColour (monocromo ? juce::Colours::black : cores[i]);
        g.strokePath (p, juce::PathStrokeType ((monocromo ? 52.0f : 40.0f) * k, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    // o disco e a agulla
    const float kr = 200.0f * k;
    if (monocromo)
    {
        g.setColour (juce::Colours::black);
        g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 46.0f * k);
        g.drawLine ({ c, P (512.0f, 345.0f) }, 46.0f * k);
        return;
    }
    g.setColour (juce::Colour (0xff080b16).withAlpha (0.6f));
    g.fillEllipse (c.x - kr, c.y - kr + 18.0f * k, kr * 2.0f, kr * 2.0f);
    juce::ColourGradient disco (juce::Colour (0xff4a5578), c.x - kr * 0.24f, c.y - kr * 0.4f, juce::Colour (0xff151a2e), c.x - kr * 0.24f + kr * 1.6f, c.y - kr * 0.4f, true);
    disco.addColour (0.6, juce::Colour (0xff232a45));
    g.setGradientFill (disco);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (juce::Colour (0xff3a4468));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 3.0f * k);
    g.setColour (col::crema);
    juce::Path ag;
    ag.startNewSubPath (c);
    ag.lineTo (P (512.0f, 345.0f));
    g.strokePath (ag, juce::PathStrokeType (26.0f * k, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Image iconaApp()
{
    static const juce::Image im = []
    {
        auto i = juce::ImageFileFormat::loadFrom (BinaryData::Icona_png, BinaryData::Icona_pngSize);
        if (! i.isValid()) return i;
        // sen o marxe transparente (a placa vai de 100 a 924 en 1024)
        const int a = i.getWidth() * 100 / 1024, b = i.getWidth() * 824 / 1024;
        return i.getClippedImage ({ a, a, b, b });
    }();
    return im;
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
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colours::white.withAlpha (0.07f));
    setColour (juce::PopupMenu::highlightedTextColourId, col::crema);
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

static juce::String estiloDe (juce::Button& b)
{
    if (b.getProperties().contains ("icona")) return "icona";
    if (b.getProperties().contains ("primaria")) return "primaria";
    return b.getProperties()["estilo"].toString();
}

static juce::Font fonteTecla (juce::Button& b, const juce::String& estilo)
{
    if (estilo == "chip") return caps (css (10.0f), 0.12f);
    const float t = (float) b.getProperties().getWithDefault ("fontSize", 13.0);
    return semi (css (t));
}

int Look::anchoTecla (const juce::TextButton& b, int marxe)
{
    auto& bb = const_cast<juce::TextButton&> (b);
    const auto e = estiloDe (bb);
    const auto t = e == "chip" ? upper (b.getButtonText()) : b.getButtonText();
    int w = juce::roundToInt (anchoTexto (fonteTecla (bb, e), t)) + marxe;
    if (e == "pestana" && bb.getProperties().contains ("reconto"))
        w += 6 + juce::roundToInt (anchoTexto (semi (css (11.0f)), bb.getProperties()["reconto"].toString()));
    if (e == "aviso") w += 18;
    return w;
}

// As teclas do escaparate: a principal en ámbar, as outras escuras ou só co bordo, as iconas en
// cadrado; co foco do teclado, un aro (na cor do plugin, se a ten)
void Look::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    const auto e = estiloDe (b);
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool redonda = b.getProperties().contains ("redonda");
    const float rad = redonda ? (float) b.getHeight() * 0.5f : e == "pestana" ? 8.0f : e == "chip" ? 6.0f : b.getHeight() >= 44 ? 12.0f : 10.0f;
    const bool on = b.isEnabled();
    if (e == "primaria")
    {
        const bool grande = b.getProperties().contains ("grande");
        if (on) paintSombra (g, r.reduced (4.0f, 2.0f), rad, col::laranxa.withAlpha (over ? 0.45f : (grande ? 0.4f : 0.32f)), grande ? 24.0f : 14.0f, grande ? 9.0f : 5.0f);
        auto rr = down ? r.translated (0.0f, 1.0f) : r;
        juce::ColourGradient gr (juce::Colour (0xfff3a634), 0.0f, rr.getY(), juce::Colour (0xffd27d0c), 0.0f, rr.getBottom(), false);
        if (! on) { gr = juce::ColourGradient (col::tecla1, 0.0f, rr.getY(), col::tecla2, 0.0f, rr.getBottom(), false); }
        g.setGradientFill (gr);
        g.fillRoundedRectangle (rr, rad);
        if (over && on) { g.setColour (juce::Colours::white.withAlpha (0.10f)); g.fillRoundedRectangle (rr, rad); }
        g.setColour (juce::Colours::white.withAlpha (on ? 0.25f : 0.06f));
        g.fillRect (rr.getX() + rad, rr.getY() + 0.5f, rr.getWidth() - 2.0f * rad, 1.0f);
    }
    else if (e == "icona")
    {
        if (redonda)
        {
            g.setColour (juce::Colour (0xff0c1020).withAlpha (0.55f));
            g.fillEllipse (r);
            g.setColour (juce::Colours::white.withAlpha (down ? 0.12f : over ? 0.09f : 0.05f));
            g.fillEllipse (r);
        }
        else
        {
            g.setColour (juce::Colours::white.withAlpha (down ? 0.09f : over ? 0.07f : 0.04f));
            g.fillRoundedRectangle (r, rad);
            g.setColour (juce::Colours::white.withAlpha (over ? 0.10f : 0.06f));
            g.drawRoundedRectangle (r, rad, 1.0f);
        }
    }
    else if (e == "escura")
    {
        // a tecla secundaria de D: un vidro escuro cun bordo fino
        g.setColour (juce::Colour (0xff0c1020).withAlpha (0.6f));
        g.fillRoundedRectangle (r, rad);
        g.setColour (juce::Colours::white.withAlpha (down ? 0.12f : over ? 0.10f : 0.063f));
        g.fillRoundedRectangle (r, rad);
        g.setColour (juce::Colours::white.withAlpha (over ? 0.16f : 0.094f));
        g.drawRoundedRectangle (r, rad, 1.0f);
    }
    else if (e == "contorno" || e == "fantasma")
    {
        const auto c = e == "contorno" ? corProp (b, "cor", col::ouro) : col::crema;
        if (e == "contorno") { g.setColour (juce::Colour (0xd90b0f1f)); g.fillRoundedRectangle (r, rad); }
        if (over && on) { g.setColour (c.withAlpha (0.08f)); g.fillRoundedRectangle (r, rad); }
        g.setColour (c.withAlpha (! on ? 0.08f : e == "contorno" ? (over ? 0.55f : 0.35f) : (over ? 0.24f : 0.14f)));
        g.drawRoundedRectangle (r, rad, 1.0f);
    }
    else if (e == "pestana")
    {
        // só o texto; a escollida, cunha liña laranxa debaixo
        if (b.getToggleState())
        {
            const float tw = anchoTexto (fonteTecla (b, e), b.getButtonText());
            g.setColour (col::laranxa);
            g.fillRoundedRectangle (r.getCentreX() - tw * 0.5f, r.getBottom() - 3.0f, tw, 2.0f, 1.0f);
        }
    }
    else if (e == "chip")
    {
        if (b.getToggleState())
        {
            g.setColour (col::laranxa.withAlpha (0.12f));
            g.fillRoundedRectangle (r, rad);
        }
        g.setColour (b.getToggleState() ? col::laranxa.withAlpha (0.5f) : juce::Colours::white.withAlpha (over ? 0.16f : 0.08f));
        g.drawRoundedRectangle (r, rad, 1.0f);
    }
    else if (e == "aviso")
    {
        g.setColour (col::ouro.withAlpha (over ? 0.13f : 0.08f));
        g.fillRoundedRectangle (r, rad);
        g.setColour (col::ouro.withAlpha (over ? 0.35f : 0.2f));
        g.drawRoundedRectangle (r, rad, 1.0f);
    }
    else if (e == "ligazon") {}
    else
    {
        // a tecla de sempre: escura, cun fío de luz arriba
        auto rr = down ? r.translated (0.0f, 1.0f) : r;
        g.setGradientFill (juce::ColourGradient (col::tecla1.brighter (over && on ? 0.12f : 0.0f), 0.0f, rr.getY(), col::tecla2, 0.0f, rr.getBottom(), false));
        g.fillRoundedRectangle (rr, rad);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (rr, rad, 1.0f);
    }
    if (b.hasKeyboardFocus (false))
    {
        g.setColour (corProp (b, "led", col::crema).withAlpha (0.9f));
        g.drawRoundedRectangle (b.getLocalBounds().toFloat().reduced (1.0f), rad + 0.5f, 1.6f);
    }
}

void Look::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool down)
{
    const auto e = estiloDe (b);
    auto r = b.getLocalBounds().toFloat();
    if (down && (e == "primaria" || e == "escura" || e.isEmpty())) r.translate (0.0f, 1.0f);
    const bool on = b.isEnabled();
    juce::Colour c = col::crema;
    if (e == "primaria") c = on ? juce::Colour (0xff1a1205) : col::crema3;
    else if (e == "icona") c = over ? col::crema : col::crema2;
    else if (e == "contorno") c = corProp (b, "cor", col::ouro);
    else if (e == "fantasma") c = over ? col::crema : col::crema2;
    else if (e == "escura") c = col::crema;
    else if (e == "pestana") c = b.getToggleState() ? col::crema : over ? col::crema2 : col::crema3;
    else if (e == "chip") c = b.getToggleState() ? col::crema : over ? col::crema2 : col::crema3;
    else if (e == "aviso") c = col::ouro;
    else if (e == "ligazon") c = over ? col::crema : corProp (b, "cor", col::crema3);
    if (! on) c = c.withMultipliedAlpha (0.5f);
    g.setColour (c);

    if (e == "icona")
    {
        const auto icona = b.getProperties()["icona"].toString();
        if (icona == "refrescar") { paintRefrescar (g, r, c); return; }
        if (icona == "axustes") { paintEngrenaxe (g, r, c); return; }
        if (icona == "lupa") { paintLupa (g, r.translated (1.0f, 1.0f), c); return; }
        if (icona == "mais")
        {
            for (int i = -1; i <= 1; ++i) g.fillEllipse (r.getCentreX() + (float) i * 5.0f - 1.4f, r.getCentreY() - 1.4f, 2.8f, 2.8f);
            return;
        }
        if (icona == "pechar")
        {
            const auto c0 = r.getCentre();
            g.drawLine (c0.x - 5.5f, c0.y - 5.5f, c0.x + 5.5f, c0.y + 5.5f, 1.6f);
            g.drawLine (c0.x + 5.5f, c0.y - 5.5f, c0.x - 5.5f, c0.y + 5.5f, 1.6f);
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
            g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            return;
        }
        return;
    }
    const auto f = fonteTecla (b, e);
    g.setFont (f);
    if (e == "chip")
    {
        g.drawText (upper (b.getButtonText()), r, juce::Justification::centred, false);
        return;
    }
    if (e == "aviso")
    {
        // o LED e o texto
        const auto cc = juce::Point<float> (r.getX() + 15.0f, r.getCentreY());
        g.setColour (col::ouro.withAlpha (0.3f)); g.fillEllipse (cc.x - 5.5f, cc.y - 5.5f, 11.0f, 11.0f);
        g.setColour (col::ouro); g.fillEllipse (cc.x - 4.0f, cc.y - 4.0f, 8.0f, 8.0f);
        g.setFont (body (css (13.0f)));
        g.drawFittedText (b.getButtonText(), r.withTrimmedLeft (28.0f).withTrimmedRight (10.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.85f);
        return;
    }
    if (e == "pestana" && b.getProperties().contains ("reconto"))
    {
        const auto n = b.getProperties()["reconto"].toString();
        const auto fn = semi (css (11.0f));
        const float wt = anchoTexto (f, b.getButtonText()), wn = anchoTexto (fn, n);
        const float x = r.getCentreX() - (wt + 6.0f + wn) * 0.5f;
        g.drawText (b.getButtonText(), juce::Rectangle<float> (x, r.getY(), wt + 2.0f, r.getHeight()), juce::Justification::centredLeft, false);
        g.setColour (col::crema3);
        g.setFont (fn);
        g.drawText (n, juce::Rectangle<float> (x + wt + 6.0f, r.getY() + 1.0f, wn + 2.0f, r.getHeight()), juce::Justification::centredLeft, false);
        return;
    }
    g.drawFittedText (b.getButtonText(), r.toNearestInt().reduced (8, 0), juce::Justification::centred, 1, 0.8f);
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
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawRect (0, 0, w, h);
}

void Look::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool sep, bool activo, bool resaltado, bool marcado,
                              bool submenu, const juce::String& texto, const juce::String& atallo, const juce::Drawable* icona, const juce::Colour* cor)
{
    if (sep || ! resaltado || ! activo)
    {
        LookAndFeel_V4::drawPopupMenuItem (g, area, sep, activo, resaltado, marcado, submenu, texto, atallo, icona, cor);
        return;
    }
    // o resaltado: unha pastilla suave, non unha barra laranxa enteira
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRoundedRectangle (area.toFloat().reduced (4.0f, 1.0f), 6.0f);
    const auto c = col::crema;
    LookAndFeel_V4::drawPopupMenuItem (g, area, sep, activo, false, marcado, submenu, texto, atallo, icona, &c);
}

void Look::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor& t)
{
    if (t.getProperties().contains ("buscador"))
    {
        return;   // (o fondo debúxao a barra)
    }
    LookAndFeel_V4::fillTextEditorBackground (g, w, h, t);
}

void Look::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& t)
{
    if (t.getProperties().contains ("buscador"))
    {
        auto r = juce::Rectangle<float> ((float) w, (float) h).reduced (0.5f);
        g.setColour (t.hasKeyboardFocus (true) ? col::laranxa.withAlpha (0.55f) : juce::Colours::white.withAlpha (0.06f));
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, t.hasKeyboardFocus (true) ? 1.4f : 1.0f);
        return;
    }
    LookAndFeel_V4::drawTextEditorOutline (g, w, h, t);
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

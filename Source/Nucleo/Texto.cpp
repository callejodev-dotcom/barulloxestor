#include "Texto.h"
#include <atomic>
#include <unordered_map>

namespace xestor
{
namespace
{
std::atomic<int> idiomaActual { (int) Idioma::gl };

struct Indice
{
    std::unordered_map<std::string, const textos::Entrada*> porGalego;
    Indice()
    {
        for (auto& e : textos::taboa()) porGalego[e.gl] = &e;
    }
};

const Indice& indice()
{
    static const Indice i;
    return i;
}

juce::CriticalSection bloqueoFaltan;
juce::StringArray faltan;
} // namespace

juce::String clave (Idioma i)
{
    switch (i)
    {
        case Idioma::gl: return "gl";
        case Idioma::es: return "es";
        case Idioma::en: return "en";
    }
    return "gl";
}

juce::String nome (Idioma i)
{
    switch (i)
    {
        case Idioma::gl: return "Galego";
        case Idioma::es: return "Castellano";
        case Idioma::en: return "English";
    }
    return {};
}

std::optional<Idioma> idiomaDeClave (const juce::String& c)
{
    for (auto i : { Idioma::gl, Idioma::es, Idioma::en })
        if (c.trim().equalsIgnoreCase (clave (i))) return i;
    return std::nullopt;
}

Idioma idiomaDoCodigo (const juce::String& codigo)
{
    // «gl-ES», «gl_ES», «es», «es-419», «en-US», «Galician»...
    const auto c = codigo.trim().toLowerCase().initialSectionNotContaining ("-_.@ ");
    if (c == "gl" || c == "glg" || c == "galician" || c == "galego") return Idioma::gl;
    if (c == "es" || c == "spa" || c == "spanish" || c == "castellano") return Idioma::es;
    return Idioma::en;
}

Idioma idiomaDoSistema()
{
    auto c = juce::SystemStats::getDisplayLanguage();
    if (c.isEmpty()) c = juce::SystemStats::getUserLanguage();
    return idiomaDoCodigo (c);
}

void porIdioma (Idioma i) { idiomaActual = (int) i; }
Idioma idioma() { return (Idioma) idiomaActual.load(); }

juce::String tr (const char* galego)
{
    const auto i = idioma();
    if (i == Idioma::gl || galego == nullptr) return juce::String::fromUTF8 (galego);
    const auto& m = indice().porGalego;
    auto e = m.find (galego);
    if (e == m.end())
    {
        const juce::ScopedLock l (bloqueoFaltan);
        faltan.addIfNotAlreadyThere (juce::String::fromUTF8 (galego));
        return juce::String::fromUTF8 (galego);
    }
    return juce::String::fromUTF8 (i == Idioma::es ? e->second->es : e->second->en);
}

juce::String tr (const juce::String& galego) { return tr (galego.toRawUTF8()); }

juce::String tr (const char* galego, const juce::String& a1, const juce::String& a2, const juce::String& a3)
{
    // primeiro o %3 e o %2 (para que un argumento que leve «%1» non se volva substituír)
    return tr (galego).replace ("%1", "\x01").replace ("%2", "\x02").replace ("%3", "\x03")
                      .replace ("\x01", a1).replace ("\x02", a2).replace ("\x03", a3);
}

namespace textos
{
juce::StringArray senTraducion()
{
    const juce::ScopedLock l (bloqueoFaltan);
    return faltan;
}

juce::String mesCurto (int mes)
{
    static const char* gl[] = { "xan", "feb", "mar", "abr", "mai", "xuñ", "xul", "ago", "set", "out", "nov", "dec" };
    static const char* es[] = { "ene", "feb", "mar", "abr", "may", "jun", "jul", "ago", "sep", "oct", "nov", "dic" };
    static const char* en[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    const int m = juce::jlimit (1, 12, mes) - 1;
    const auto i = idioma();
    return juce::String::fromUTF8 (i == Idioma::es ? es[m] : i == Idioma::en ? en[m] : gl[m]);
}

juce::String markdownLexible (const juce::String& md)
{
    juce::StringArray saida;
    bool codigo = false;
    for (auto linha : juce::StringArray::fromLines (md.replace ("\r\n", "\n")))
    {
        auto t = linha.trimEnd();
        if (t.trimStart().startsWith ("```")) { codigo = ! codigo; continue; }
        if (! codigo)
        {
            const auto sen = t.trimStart();
            const int sangria = t.length() - sen.length();
            if (sen.startsWith ("#"))
            {
                // un título: en liña propia, cun oco antes
                if (! saida.isEmpty() && saida[saida.size() - 1].isNotEmpty()) saida.add ({});
                t = sen.trimCharactersAtStart ("#").trim();
            }
            else if (sen.startsWith ("- ") || sen.startsWith ("* ") || sen.startsWith ("+ "))
                t = juce::String::repeatedString (" ", juce::jmin (sangria, 6)) + juce::String::fromUTF8 ("• ") + sen.substring (2).trimStart();
            // as ligazóns [texto](url) → texto; as imaxes ![x](y) fóra
            for (int i = t.indexOf ("]("); i >= 0; i = t.indexOf ("]("))
            {
                const int abre = t.substring (0, i).lastIndexOfChar ('[');
                const int pecha = t.indexOfChar (i + 2, ')');
                if (abre < 0 || pecha < 0) break;
                const bool imaxe = abre > 0 && t[abre - 1] == '!';
                t = t.substring (0, imaxe ? abre - 1 : abre) + (imaxe ? juce::String() : t.substring (abre + 1, i)) + t.substring (pecha + 1);
            }
            t = t.replace ("**", "").replace ("__", "").replace ("`", "");
            // as táboas: «| a | b |» → «a · b»; a liña «|---|---|», fóra
            const auto tt = t.trim();
            if (tt.startsWithChar ('|'))
            {
                if (tt.containsOnly ("|-: ")) continue;
                auto celas = juce::StringArray::fromTokens (tt.substring (1, tt.endsWithChar ('|') ? tt.length() - 1 : tt.length()), "|", "");
                celas.trim();
                t = celas.joinIntoString (juce::String::fromUTF8 (" · "));
            }
            // a cursiva *así* (non os «• » nin un * solto)
            for (int i = t.indexOfChar ('*'); i >= 0; i = t.indexOfChar (i, '*'))
            {
                const int f = t.indexOfChar (i + 1, '*');
                if (f < 0 || f == i + 1 || t[i + 1] == ' ') break;
                t = t.substring (0, i) + t.substring (i + 1, f) + t.substring (f + 1);
                i = f - 1;
            }
        }
        saida.add (t);
    }
    while (! saida.isEmpty() && saida[0].trim().isEmpty()) saida.remove (0);
    while (! saida.isEmpty() && saida[saida.size() - 1].trim().isEmpty()) saida.remove (saida.size() - 1);
    return saida.joinIntoString ("\n");
}
} // namespace textos
} // namespace xestor

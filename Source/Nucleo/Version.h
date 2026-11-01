#pragma once
#include <juce_core/juce_core.h>
#include <optional>
#include <vector>

namespace xestor
{
// un literal UTF-8 do código coma juce::String (coma escoita::U, sen depender de JUCE gui)
inline juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

// Unha versión «maior.menor.parche» (acepta «v0.4.0», «0.4» ou «1.2.3-beta»).
// Compárase coma no núcleo de Escoita: número a número, os que faltan valen 0.
struct Version
{
    std::vector<int> partes;

    static std::optional<Version> ler (juce::String s)
    {
        s = s.trim();
        if (s.startsWithIgnoreCase ("v")) s = s.substring (1);
        // o que vaia tras un «-» ou «+» (beta, compilación...) non conta
        const int corte = s.indexOfAnyOf ("-+");
        if (corte >= 0) s = s.substring (0, corte);
        if (s.isEmpty()) return std::nullopt;
        Version v;
        for (auto& a : juce::StringArray::fromTokens (s, ".", ""))
        {
            const auto dixitos = a.initialSectionContainingOnly ("0123456789");
            if (dixitos.isEmpty()) return std::nullopt;
            v.partes.push_back (dixitos.getIntValue());
        }
        return v;
    }

    int comparar (const Version& o) const
    {
        const size_t n = std::max ({ partes.size(), o.partes.size(), (size_t) 3 });
        for (size_t i = 0; i < n; ++i)
        {
            const int a = i < partes.size() ? partes[i] : 0, b = i < o.partes.size() ? o.partes[i] : 0;
            if (a != b) return a < b ? -1 : 1;
        }
        return 0;
    }

    bool operator<  (const Version& o) const { return comparar (o) < 0; }
    bool operator>  (const Version& o) const { return comparar (o) > 0; }
    bool operator== (const Version& o) const { return comparar (o) == 0; }
    bool operator!= (const Version& o) const { return comparar (o) != 0; }
    bool operator<= (const Version& o) const { return comparar (o) <= 0; }

    juce::String texto() const
    {
        juce::StringArray a;
        for (int p : partes) a.add (juce::String (p));
        return a.joinIntoString (".");
    }
};

inline juce::String texto (const std::optional<Version>& v, const juce::String& senVersion = "?")
{
    return v ? v->texto() : senVersion;
}
} // namespace xestor

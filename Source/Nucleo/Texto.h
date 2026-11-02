#pragma once
#include "Version.h"

namespace xestor
{
// Os idiomas da interface. O texto do código está en galego e é a chave: tr («Instalar») devolve
// «Instalar», «Instalar» ou «Install» segundo o idioma escollido. O que non estea na táboa
// queda en galego (e anótase, para velo coa captura en --idioma en).
// A liña de ordes e os informes van sempre en galego.
enum class Idioma { gl, es, en };

juce::String clave (Idioma);    // «gl», «es», «en»
juce::String nome (Idioma);     // «Galego», «Castellano», «English»
std::optional<Idioma> idiomaDeClave (const juce::String&);   // «gl», «es», «en» (baleiro: nullopt)
// O do sistema: galego se é gl, castelán se é es; se non, inglés («gl-ES», «es_ES», «en-US»...)
Idioma idiomaDoCodigo (const juce::String& codigo);
Idioma idiomaDoSistema();

void porIdioma (Idioma);
Idioma idioma();

// O texto no idioma de agora. «%1», «%2» e «%3» substitúense polos argumentos.
juce::String tr (const char* galego);
juce::String tr (const juce::String& galego);
juce::String tr (const char* galego, const juce::String& a1, const juce::String& a2 = {}, const juce::String& a3 = {});

namespace textos
{
    struct Entrada { const char* gl; const char* es; const char* en; };
    const std::vector<Entrada>& taboa();
    juce::StringArray senTraducion();   // as chaves pedidas que non están na táboa (en es ou en)
    // «xan», «ene», «Jan»...: o mes curto (1–12) no idioma de agora
    juce::String mesCurto (int mes);
    // Un Markdown sinxelo (as notas da versión) en texto para ler: «- x» → «• x», sen ** nin `,
    // os títulos («## x») en liña propia, as ligazóns [texto](url) → texto
    juce::String markdownLexible (const juce::String& md);
}
} // namespace xestor

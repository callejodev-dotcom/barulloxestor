#include "Produto.h"

namespace xestor
{
SO soActual()
{
   #if JUCE_MAC
    return SO::mac;
   #elif JUCE_WINDOWS
    return SO::win;
   #else
    return SO::lin;
   #endif
}

const std::vector<Formato>& todosOsFormatos()
{
    static const std::vector<Formato> t { Formato::vst3, Formato::au, Formato::clap, Formato::lv2, Formato::app };
    return t;
}

std::vector<Formato> formatosDe (SO so)
{
    switch (so)
    {
        case SO::mac:     return { Formato::vst3, Formato::au, Formato::clap, Formato::app };
        case SO::win: return { Formato::vst3, Formato::clap, Formato::app };
        case SO::lin:   return { Formato::vst3, Formato::clap, Formato::lv2, Formato::app };
    }
    return { Formato::vst3 };
}

juce::String clave (Formato f)
{
    switch (f)
    {
        case Formato::vst3: return "vst3";
        case Formato::au:   return "au";
        case Formato::clap: return "clap";
        case Formato::lv2:  return "lv2";
        case Formato::app:  return "app";
    }
    return {};
}

juce::String etiqueta (Formato f)
{
    switch (f)
    {
        case Formato::vst3: return "VST3";
        case Formato::au:   return "AU";
        case Formato::clap: return "CLAP";
        case Formato::lv2:  return "LV2";
        case Formato::app:  return "App";
    }
    return {};
}

std::optional<Formato> formatoDeClave (juce::String t)
{
    t = t.trim().toLowerCase();
    if (t == "component" || t == "components") return Formato::au;
    if (t == "standalone") return Formato::app;
    for (auto f : todosOsFormatos())
        if (clave (f) == t) return f;
    return std::nullopt;
}

std::optional<std::vector<Formato>> listaFormatos (const juce::String& texto)
{
    std::vector<Formato> r;
    for (auto& a : juce::StringArray::fromTokens (texto, ",", ""))
    {
        auto f = formatoDeClave (a);
        if (! f) return std::nullopt;
        if (std::find (r.begin(), r.end(), *f) == r.end()) r.push_back (*f);
    }
    if (r.empty()) return std::nullopt;
    return r;
}

juce::String clave (TipoProduto t)
{
    switch (t)
    {
        case TipoProduto::instrumento: return "instrumento";
        case TipoProduto::efecto:      return "efecto";
        case TipoProduto::paquete:     return "paquete";
        case TipoProduto::desconecido: return U ("descoñecido");
    }
    return {};
}

juce::String titulo (TipoProduto t)
{
    switch (t)
    {
        case TipoProduto::instrumento: return "Instrumentos";
        case TipoProduto::efecto:      return "Efectos";
        case TipoProduto::paquete:     return "Paquetes";
        case TipoProduto::desconecido: return "Outros";
    }
    return {};
}

int orde (TipoProduto t) { return (int) t; }

const Activo* Lanzamento::arquivo (SO so) const
{
    const juce::String sufixo = so == SO::mac ? "-mac.zip" : so == SO::win ? "-windows-x64.zip" : "-linux-x86_64.tar.gz";
    for (auto& a : activos)
        if (a.nome.toLowerCase().endsWith (sufixo)) return &a;
    return nullptr;
}

juce::String normalizar (const juce::String& s)
{
    static const juce::String de  = juce::CharPointer_UTF8 ("áàâäãéèêëíìîïóòôöõúùûüñçÁÀÂÄÃÉÈÊËÍÌÎÏÓÒÔÖÕÚÙÛÜÑÇ");
    static const juce::String a   = "aaaaaeeeeiiiiooooouuuuncAAAAAEEEEIIIIOOOOOUUUUNC";
    juce::String r;
    for (auto c : s)
    {
        const int i = de.indexOfChar (c);
        if (i >= 0) c = a[i];
        if (juce::CharacterFunctions::isLetterOrDigit (c)) r += juce::CharacterFunctions::toLowerCase (c);
    }
    return r;
}

juce::String Produto::nomeCurto() const
{
    return nome.startsWith ("Barullo ") ? nome.substring (8) : nome;
}

bool Produto::casa (const juce::String& consulta) const
{
    const auto q = normalizar (consulta);
    if (q.isEmpty()) return false;
    juce::StringArray nomes { nome, curto, nomeCurto() };
    if (repo.isNotEmpty()) nomes.add (repo.fromLastOccurrenceOf ("/", false, false));
    for (auto& n : nomes)
        if (normalizar (n) == q) return true;
    return false;
}

std::optional<Produto> Produto::deProdutoEnv (const juce::String& texto, const juce::String& repo, const juce::String& descricionRepo)
{
    const auto v = lerProdutoEnv (texto);
    Produto p;
    p.nome = v["NOME"];
    if (p.nome.isEmpty()) return std::nullopt;
    p.curto = v["CURTO"].isNotEmpty() ? v["CURTO"] : p.nome.removeCharacters (" ");
    p.binario = v["BINARIO"].isNotEmpty() ? v["BINARIO"] : p.nome.toLowerCase().replace (" ", "-");
    const auto au = v["AU"];
    p.temAU = au.isNotEmpty();
    const auto lista = v["PLUGINS"];
    if (lista.trim().isNotEmpty())
    {
        for (auto& linha : juce::StringArray::fromLines (lista))
        {
            auto campos = juce::StringArray::fromTokens (linha, "|", "");
            campos.trim();
            if (campos.size() > 0 && campos[0].isNotEmpty()) p.plugins.add (campos[0]);
            if (campos.size() > 2 && campos[2].isNotEmpty()) p.temAU = true;
        }
        p.tipo = TipoProduto::paquete;
    }
    else
    {
        p.plugins.add (p.nome);
        if (au.startsWith ("aumu") || au.startsWith ("aumi")) p.tipo = TipoProduto::instrumento;
        else if (au.startsWith ("aufx") || au.startsWith ("aumf")) p.tipo = TipoProduto::efecto;
    }
    p.descricion = v["DESCRICION"].isNotEmpty() ? v["DESCRICION"] : descricionRepo;
    p.repo = repo;
    // CAPTURAS (opcional): as imaxes da interface, relativas á raíz do repo, separadas por espazos ou liñas
    p.capturasDeclaradas = juce::StringArray::fromTokens (v["CAPTURAS"], " \n\t", "\"'");
    p.capturasDeclaradas.removeEmptyStrings();
    return p;
}

juce::StringPairArray lerProdutoEnv (const juce::String& texto)
{
    juce::StringPairArray r;
    const auto c = texto.replace ("\r\n", "\n");
    const int n = c.length();
    int i = 0;
    auto saltarLinha = [&] { while (i < n && c[i] != '\n') ++i; };
    while (i < n)
    {
        // inicio de liña: espazos, comentario ou CHAVE=
        while (i < n && (c[i] == ' ' || c[i] == '\t')) ++i;
        if (i >= n) break;
        if (c[i] == '\n') { ++i; continue; }
        if (c[i] == '#') { saltarLinha(); continue; }
        juce::String chave;
        while (i < n && (juce::CharacterFunctions::isLetterOrDigit (c[i]) || c[i] == '_')) chave += c[i++];
        if (chave == "export") continue;
        if (i >= n || c[i] != '=' || chave.isEmpty()) { saltarLinha(); continue; }
        ++i;
        juce::String valor;
        while (i < n && c[i] != '\n')
        {
            const auto ch = c[i];
            if (ch == '"' || ch == '\'')
            {
                ++i;
                while (i < n && c[i] != ch)
                {
                    if (ch == '"' && c[i] == '\\' && i + 1 < n) ++i;
                    valor += c[i++];
                }
                ++i;
            }
            else if (ch == ' ' || ch == '\t')
            {
                saltarLinha();   // fóra das comiñas, un espazo remata o valor (o resto é un comentario)
            }
            else
            {
                valor += ch; ++i;
            }
        }
        r.set (chave, valor);
    }
    return r;
}
} // namespace xestor

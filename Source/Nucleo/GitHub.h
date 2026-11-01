#pragma once
#include "Rede.h"
#include <atomic>
#include <stdexcept>

namespace xestor
{
struct RepoInfo
{
    juce::String nomeCompleto;   // «propietario/nome»
    juce::String descricion, rama = "main";
    bool privado = false, arquivado = false, bifurcacion = false;
};

struct ErroGitHub : std::runtime_error
{
    using std::runtime_error::runtime_error;
    juce::String texto() const { return juce::String::fromUTF8 (what()); }
};

// Cliente pequeno da API de GitHub, con caché no disco.
// Sen sesión, GitHub deixa 60 preguntas por hora: por iso garda as respostas (e o seu ETag, para
// preguntar «cambiou algo?», que se non cambiou non gasta) e non volve preguntar ata pasada a vixencia.
// O de raw.githubusercontent.com (produto.env, capturas) non gasta preguntas.
class GitHub
{
public:
    GitHub (juce::String conta, juce::File cartafolCache, Transporte& t, juce::String tokenGh = {})
        : propietario (std::move (conta)), cache (std::move (cartafolCache)), transporte (t), token (std::move (tokenGh)) {}

    static juce::File cacheEstandar();
    // GITHUB_TOKEN / GH_TOKEN, ou o de `gh auth token` se o gh está instalado e ten sesión
    static juce::String tokenDisponible();

    juce::RelativeTime vixencia = juce::RelativeTime::minutes (15);
    bool soCache = false;   // sen rede: só o que estea gardado

    // Pide un URL (con caché). 200 e 404 gárdanse (404: non hai versión, non é un plugin...).
    Resposta pedir (const juce::String& url, bool forzar = false);

    std::vector<RepoInfo> repos (bool forzar = false);
    std::optional<RepoInfo> repo (const juce::String& nomeCompleto, bool forzar = false);
    // nullopt: o repo non é un plugin de Escoita
    std::optional<juce::String> produtoEnv (const RepoInfo&, bool forzar = false);
    std::optional<Lanzamento> ultimoLanzamento (const juce::String& nomeCompleto, bool forzar = false);
    // As capturas da interface: as de CAPTURAS, a estándar docs/img/captura.png e as imaxes do
    // LEEME.md, README.md e docs/MANUAL.md (na orde en que aparecen). URL de raw.githubusercontent.com.
    juce::StringArray capturas (const RepoInfo&, const juce::StringArray& declaradas, bool forzar = false);
    // Os bytes dunha imaxe (da caché se a hai); baleiro se non se pode
    juce::MemoryBlock imaxe (const juce::String& url);

    static std::optional<Lanzamento> lanzamento (const juce::var& o);
    void baleirarCache() { cache.deleteRecursively(); }

    int restantes() const { return restantesAPI.load(); }   // −1: non se sabe
    bool usouCacheVella() const { return cacheVella.load(); }
    const juce::String& getPropietario() const { return propietario; }

private:
    juce::var json (const juce::String& url, bool forzar);
    juce::File ficheiro (const juce::String& url, const char* ext) const;
    juce::String propietario;
    juce::File cache;
    Transporte& transporte;
    juce::String token;
    std::atomic<int> restantesAPI { -1 };
    std::atomic<bool> cacheVella { false };
    juce::CriticalSection bloqueoCache;
};

// As imaxes que se mencionan nun texto Markdown (![...](ruta) e <img src="ruta">), na orde en que aparecen
juce::StringArray imaxesMarkdown (const juce::String& md);
} // namespace xestor

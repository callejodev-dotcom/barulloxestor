#pragma once
#include "Version.h"

namespace xestor
{
// O sistema para o que se fan as rutas e se escolle o ficheiro publicado. Por defecto, este;
// as probas poden pedir outro para ver as rutas de Windows ou Linux desde un Mac.
enum class SO { mac, win, lin };   // (non «linux»: en GCC «linux» é unha macro que vale 1)
SO soActual();

// Os formatos que se poden instalar. O usuario escolle cales; por defecto, só VST3.
enum class Formato { vst3, au, clap, lv2, app };

const std::vector<Formato>& todosOsFormatos();
std::vector<Formato> formatosDe (SO);                    // os que hai nese sistema
juce::String clave (Formato);                            // «vst3», «au», «clap», «lv2», «app»
juce::String etiqueta (Formato);                         // «VST3», «AU», «CLAP», «LV2», «App»
std::optional<Formato> formatoDeClave (juce::String);    // «vst3», «component», «standalone»...
std::optional<std::vector<Formato>> listaFormatos (const juce::String&);   // «vst3,clap»

enum class TipoProduto { instrumento, efecto, paquete, desconecido };
juce::String clave (TipoProduto);
juce::String titulo (TipoProduto);   // «Instrumentos»...
int orde (TipoProduto);

// Un ficheiro dunha versión publicada en GitHub.
struct Activo
{
    juce::String nome, url;
    juce::int64 tamano = 0;
    juce::String sha256;   // baleiro se GitHub non o dá
};

// A última versión publicada dun produto.
struct Lanzamento
{
    juce::String tag, nome, paxina;
    juce::Time publicado;
    std::vector<Activo> activos;
    juce::String notas;   // o texto da versión en GitHub (Markdown); baleiro se non o ten

    std::optional<Version> version() const { return Version::ler (tag); }
    // o arquivo «á man» (bundles soltos) deste sistema: é o que instala o xestor
    const Activo* arquivo (SO = soActual()) const;
};

// Un produto de Escoita: un plugin ou unha serie (varios plugins nun instalador, coma Enxebres).
struct Produto
{
    juce::String nome;          // «Barullo Tune» (o do paquete, nunha serie)
    juce::String curto;         // «BarulloTune»: prefixo dos ficheiros publicados
    juce::String binario;       // «barullo-tune»: a app en Linux
    juce::String descricion;
    juce::StringArray plugins;  // os nomes dos bundles, sen tiles
    TipoProduto tipo = TipoProduto::desconecido;
    bool temAU = false;
    juce::String repo;          // «propietario/nome» en GitHub (baleiro: só local)
    juce::String rama = "main";
    juce::File rutaLocal;       // o repositorio na máquina, se se atopou
    std::optional<Version> versionLocal;   // a do CMakeLists local
    std::optional<Lanzamento> ultima;
    bool lanzamentoConsultado = false;     // preguntouse (se ultima está baleira, é que non hai ningunha)
    juce::StringArray capturas; // as imaxes da interface (URL https:// ou ficheiros locais)
    juce::StringArray capturasDeclaradas;  // as que di produto.env (CAPTURAS), relativas ao repo
    juce::String manual;        // o manual: URL de GitHub (docs/MANUAL.md, LEEME.md, README.md) ou ficheiro local

    juce::String id() const { return curto; }
    bool eSerie() const { return tipo == TipoProduto::paquete; }
    juce::String paxinaRepo() const { return repo.isEmpty() ? juce::String() : "https://github.com/" + repo; }
    // «Barullo Opto Comp» → «Opto Comp»
    juce::String nomeCurto() const;
    // «tune», «Barullo Tune», «barullotune», «enxebres»... valen para buscalo
    bool casa (const juce::String& consulta) const;

    // a partir do packaging/produto.env (o formato do núcleo de Escoita); nullopt se non é un produto
    static std::optional<Produto> deProdutoEnv (const juce::String& texto, const juce::String& repo = {},
                                                const juce::String& descricionRepo = {});
};

// Le un produto.env (sintaxe de shell sinxela: CHAVE="valor", que pode ocupar varias liñas, e # comentarios).
juce::StringPairArray lerProdutoEnv (const juce::String& texto);

// Para comparar nomes sen tiles nin maiúsculas nin espazos («Ambiénte» → «ambiente»)
juce::String normalizar (const juce::String&);
} // namespace xestor

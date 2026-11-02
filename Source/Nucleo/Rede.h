#pragma once
#include "Produto.h"
#include <functional>
#include <atomic>

namespace xestor
{
struct Resposta
{
    int estado = 0;                      // 0: non houbo resposta (sen rede)
    juce::MemoryBlock corpo;
    juce::StringPairArray cabeceiras;    // sen distinguir maiúsculas
    juce::String erro;

    juce::String texto() const { return corpo.toString(); }
};

// Para deixar de agardar pola rede: o que estea pendente volve canto antes (cun erro) e o que se
// pida despois xa non sae. Compártese entre fíos.
class Cancelacion
{
public:
    void cancelar() { feito = true; ev.signal(); }
    bool cancelado() const { return feito.load(); }
    // agarda ata «ms» (ou ata que se cancele); true se se cancelou
    bool agardar (int ms) const { return ev.wait (ms); }
private:
    std::atomic<bool> feito { false };
    mutable juce::WaitableEvent ev { true };
};

// Por onde se fala coa rede. O de verdade é TransporteHttp; as probas usan un GitHub de mentira.
class Transporte
{
public:
    virtual ~Transporte() = default;
    virtual Resposta pedir (const juce::String& url, const juce::StringPairArray& cabeceiras) = 0;
    // o mesmo, pero volve en canto se cancele (por defecto, o de arriba)
    virtual Resposta pedirCancelable (const juce::String& url, const juce::StringPairArray& cabeceiras, const Cancelacion&)
    {
        return pedir (url, cabeceiras);
    }
    // descarga a disco; progreso (0…1) devolve false para cancelar
    virtual juce::Result descargar (const juce::String& url, const juce::File& destino,
                                    std::function<bool (double)> progreso) = 0;
};

// A rede de verdade. No Mac, cunha NSURLSession propia (efémera, con tempo máximo e que se pode
// cancelar: o URL de JUCE pode quedar colgado para sempre ao pechar unha conexión cando hai moitas
// á vez); en Windows e Linux, co de JUCE (cos mesmos tempos máximos entre lecturas).
class TransporteHttp : public Transporte
{
public:
    Resposta pedir (const juce::String& url, const juce::StringPairArray& cabeceiras) override;
    Resposta pedirCancelable (const juce::String& url, const juce::StringPairArray& cabeceiras, const Cancelacion&) override;
    juce::Result descargar (const juce::String& url, const juce::File& destino, std::function<bool (double)> progreso) override;

    static constexpr int limitePedidoMs = 25000;   // unha pregunta enteira
    static constexpr int limiteInactivoMs = 60000; // unha descarga sen recibir nada
};

#if JUCE_MAC
namespace redeMac
{
    Resposta pedir (const juce::String& url, const juce::StringPairArray& cabeceiras, const Cancelacion&, int limiteMs);
    juce::Result descargar (const juce::String& url, const juce::File& destino, std::function<bool (double)> progreso, int inactivoMs);
}
#endif

// Copia un file:// (ou unha ruta) a destino: para as probas e para o --zip local
juce::Result copiarLocal (const juce::String& url, const juce::File& destino);

// A suma SHA-256 dun ficheiro, en hexadecimal
juce::String sha256 (const juce::File&);
// Comproba tamaño e suma contra o que di GitHub
juce::Result verificar (const juce::File&, const Activo&);
// «12,3 MB»
juce::String tamanoLexible (juce::int64 bytes);
} // namespace xestor

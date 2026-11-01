#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Nucleo/Nucleo.h"

// O estado da xanela. A lóxica está no núcleo; isto só a chama nun fío aparte e garda o resultado.
// Avisa á interface (ChangeBroadcaster) cando cambia algo.
class Modelo : public juce::ChangeBroadcaster
{
public:
    Modelo();
    ~Modelo() override;

    xestor::Axustes axustes;
    xestor::Rutas rutas = xestor::Rutas::reais();

    // ---------------------------------------------------------------- o que se sabe
    bool temCatalogo() const { return catalogo.has_value(); }
    const std::vector<xestor::Produto>& produtos() const;
    xestor::Situacion situacion (const xestor::Produto&) const;
    const std::vector<xestor::Instalado>& getOrfos() const { return orfos; }
    std::vector<const xestor::Produto*> porActualizar() const;
    juce::StringArray avisos() const { return catalogo ? catalogo->avisos : juce::StringArray(); }
    bool senRede() const;
    juce::Time dataCatalogo() const { return catalogo ? catalogo->data : juce::Time(); }

    bool cargando = false;
    juce::String mensaxe, erro;
    int restantes = -1;

    // o traballo en marcha de cada produto: 0…1, ou −1 se non se sabe canto queda
    std::map<juce::String, double> traballos;
    std::map<juce::String, juce::String> fases;   // «descargando», «instalando», «pedindo permiso»…
    bool traballando (const xestor::Produto& p) const { return traballos.count (p.id()) > 0; }

    // ---------------------------------------------------------------- capturas
    juce::Image miniatura (const xestor::Produto&) const;      // a primeira captura (ou nada)
    juce::Image captura (const juce::String& url);              // unha enteira (da caché)

    // ---------------------------------------------------------------- accións
    void cargar (bool forzar = false);
    void cargarAgora (bool forzar = false);   // no mesmo fío (para a captura)
    void reescanear();
    void cambiarFormato (xestor::Formato);
    void instalar (const xestor::Produto&, bool forzar = false);
    void desinstalar (const xestor::Produto&);
    void quitarCopiasVellas (const xestor::Produto&);
    void accionPrincipal (const xestor::Produto&);   // a que lle toca segundo o estado
    void actualizarTodo();
    void desinstalarOrfo (const xestor::Instalado&);

    // a captura dun estado concreto (sen rede, traballando...), para revisar o deseño
    void escena (const juce::String&);

private:
    void rematar (const xestor::Produto&, const xestor::Informe&);
    void avisar() { sendChangeMessage(); }
    void noFioPrincipal (std::function<void()>);

public:
    juce::String escenaActual;   // a da captura (o visor ábrese con «captura»)
private:

    std::optional<xestor::Catalogo> catalogo;
    std::map<juce::String, xestor::Situacion> situacions;
    std::vector<xestor::Instalado> orfos;
    std::map<juce::String, juce::Image> miniaturas;
    std::map<juce::String, juce::Image> capturas;

    xestor::TransporteHttp transporte;
    std::shared_ptr<xestor::GitHub> github;
    xestor::LixoDoSistema lixo;
    xestor::ElevadorSistema elevador;
    xestor::Rexistro rexistro { xestor::Rexistro::estandar() };
    juce::ThreadPool fios { juce::ThreadPoolOptions{}.withNumberOfThreads (1) };    // un traballo de cada vez
    juce::ThreadPool carga { juce::ThreadPoolOptions{}.withNumberOfThreads (1) };
    juce::CriticalSection bloqueo;

    JUCE_DECLARE_WEAK_REFERENCEABLE (Modelo)
};

#pragma once
#include "Nucleo/Nucleo.h"

// O modo sen xanela: «Barullo Xestor --listar», «--instalar <nome> --dir <cartafol>»...
// Vai sen interface (as probas enlázano sen a app); a captura e a icona pona a app.
namespace xestor::cli
{
struct Opcions
{
    juce::String orde, nome;
    juce::File dir, dirSistema, papeleira, arquivo, captura, icona, operacions;
    std::optional<std::vector<Formato>> formatos;
    bool forzar = false, incluirSistema = false, senRede = false, refrescar = false, json = false;
    bool senAdmin = false, simularAdmin = false, completa = false;
    int ancho = 1040, alto = 760;
    float escala = 2.0f;
    juce::String escena;   // a captura dun estado concreto: «sen-rede», «traballando», «ao-dia», «baleiro», «captura»
};

struct Contexto
{
    Transporte* transporte = nullptr;   // nullptr: a rede de verdade
    Elevador* elevador = nullptr;       // nullptr: o do sistema (ou o que pidan as opcións)
    juce::File axustes = Axustes::ficheiroEstandar();
    juce::File cache = GitHub::cacheEstandar();
    juce::File rexistro = Rexistro::estandar();
    juce::File temporal = Instalador::cartafolTemporal();
    bool usarToken = true;
    std::function<void (const juce::String&)> saida, erro;   // por defecto, stdout e stderr
    std::function<int (const Opcions&)> captura, icona;      // a app
};

juce::String axuda();
bool eCLI (const juce::StringArray& args);   // hai algunha opción «--algo»?
juce::Result ler (const juce::StringArray& args, Opcions&);
int executar (const juce::StringArray& args, Contexto&);
} // namespace xestor::cli

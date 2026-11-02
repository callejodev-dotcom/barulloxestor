#pragma once
#include "Instalador.h"

namespace xestor
{
// O propio xestor ao día: mira a última versión de callejodev-dotcom/barulloxestor e, se hai unha
// máis nova, descárgaa (comprobando tamaño e suma), pona no sitio da de agora (a vella, ao Lixo) e
// reinicia. Os arquivos publicados:
//   Mac      BarulloXestor-<v>-mac.zip                    («Barullo Xestor.app»)
//   Windows  BarulloXestor-<v>-windows-x64-instalador.exe (Inno Setup, en silencio e sen admin)
//   Linux    BarulloXestor-<v>-linux-x86_64.tar.gz        (o binario «barullo-xestor»)
namespace autoactualizacion
{
    juce::String repo();   // «callejodev-dotcom/barulloxestor»
    const Activo* activo (const Lanzamento&, SO = soActual());
    // A versión publicada, se é máis nova cá de agora e trae o arquivo deste sistema
    std::optional<Lanzamento> nova (const std::optional<Lanzamento>&, const juce::String& versionActual, SO = soActual());

    // O que está en marcha: Mac o .app; Windows e Linux o executable
    juce::File appActual();
    // Onde vai a nova (Mac e Linux): no sitio da de agora se se pode escribir alí; se non (ou se o
    // Mac a abriu desde un sitio provisional, «AppTranslocation»), en ~/Applications ou ~/.local/bin
    juce::File destino (const juce::File& actual, SO = soActual());

    // Descarga e deixa lista a nova en «traballo»: en «lista», o .app (Mac, sen cuarentena e coa
    // firma boa), o binario (Linux) ou o instalador (Windows)
    juce::Result preparar (const Lanzamento&, Transporte&, const juce::File& traballo, juce::File& lista,
                           std::function<bool (double)> progreso = {}, SO = soActual());
    // Mac e Linux: a nova ao lado co nome provisional, a vella ao Lixo e a nova ao seu sitio
    juce::Result substituir (const juce::File& nova, const juce::File& destino, const Papeleira&);

    // A orde que espera a que remate este proceso (pid) e abre a nova:
    //   Mac: sh → open; Linux: sh → o binario; Windows: cmd co guion (o instalador en silencio e despois o .exe)
    juce::StringArray ordeReinicio (SO, int pid, const juce::File& app, const juce::File& guion = {});
    juce::String guionWindows (int pid, const juce::File& instalador, const juce::File& exe);
    // Windows: onde queda o .exe despois do instalador (o de agora se xa é o instalado; se non, o estándar)
    juce::File exeTrasInstalar (const juce::File& actual);
    int pidActual();
    // Lanza a orde de reinicio sen esperar por ela (segue viva cando este proceso remata)
    bool lanzar (const juce::StringArray& orde);
}

// A comprobación en segundo plano (a icona da barra de menús / bandeja)
namespace comprobacion
{
    // Toca preguntar? (unha vez ao día; se o reloxo foi cara atrás, tamén)
    bool toca (juce::int64 ultimaMs, juce::Time agora = juce::Time::getCurrentTime(),
               juce::RelativeTime cada = juce::RelativeTime::days (1));
    // As versións novas que aínda non se avisaron («Barullo FM 0.5.1»)
    juce::StringArray novidades (const juce::StringArray& agora, const juce::StringArray& avisadas);
}
} // namespace xestor

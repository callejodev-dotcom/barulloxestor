#pragma once
#include "Produto.h"

namespace xestor
{
// O que se lle pregunta ao sistema fóra dos plugins: que DAW está aberto, avisar cunha notificación
// e como está asinado un bundle (Mac).
namespace sistema
{
    // Os programas en marcha, un por liña, tal como os dá o sistema:
    //   Mac:     ps -axo comm=          («/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live»)
    //   Linux:   ps -eo comm= e args=   («reaper», «/opt/bitwig-studio/bin/BitwigStudio --x»)
    //   Windows: tasklist /fo csv /nh   («"Ableton Live 12 Suite.exe","1234",...»)
    juce::StringArray procesos();
    // Os DAW (Ableton Live, Logic Pro, REAPER, Bitwig, Cubase, FL Studio, Studio One, Pro Tools...)
    // que hai nesa lista, sen repetir e na orde da lista de DAW
    juce::StringArray daws (const juce::StringArray& procesos);
    juce::StringArray dawsAbertos();   // daws (procesos())
    juce::StringArray todosOsDaws();   // os nomes que se recoñecen

    // A orde que amosa unha notificación do sistema (Mac: osascript «display notification»;
    // Linux: notify-send). Baleira se non hai como (Windows: a burbulla da icona da bandeja).
    juce::StringArray ordeNotificacion (SO, const juce::String& titulo, const juce::String& texto,
                                        const juce::String& notifySend = "notify-send");
    bool notificar (const juce::String& titulo, const juce::String& texto);

    // A firma dun bundle (Mac): «Developer ID (…)», «ad hoc», «sen firma» ou «non válida (…)»;
    // baleiro fóra de Mac
    juce::String estadoFirma (const juce::File&);
    // a saída de «codesign -dv --verbose=2» feita texto curto (para as probas)
    juce::String resumoFirma (bool valida, const juce::String& saidaCodesign);

    // «macOS 15.1 (arm64)», «Windows 11», «Linux (Ubuntu 22.04)»...
    juce::String descricionSO();
}
} // namespace xestor

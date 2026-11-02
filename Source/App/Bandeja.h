#pragma once
#include "Modelo.h"
#include <juce_gui_extra/juce_gui_extra.h>

// A icona na barra de menús (Mac) ou na bandeja do sistema (Windows, Linux): mentres está, o xestor
// pregunta a GitHub unha vez ao día e avisa cunha notificación das versións novas (unha vez cada
// unha). Ao pechar a xanela, a app queda aquí.
class Bandeja : public juce::SystemTrayIconComponent, private juce::Timer, private juce::ChangeListener
{
public:
    Bandeja (Modelo&, std::function<void()> abrirXanela, std::function<void()> sair);
    ~Bandeja() override;

    void mouseDown (const juce::MouseEvent&) override;

    // a icona (debuxada): a cor para Windows e Linux, o «template» monocromo para o Mac
    static juce::Image icona (bool aviso, bool modelo, int tam);

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void actualizarIcona();
    void notificar (const juce::String& titulo, const juce::String& texto);

    Modelo& modelo;
    std::function<void()> abrirXanela, sair;
    bool comprobando = false;
    int avisoActual = -1;
};

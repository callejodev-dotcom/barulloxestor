#pragma once
#include <juce_core/juce_core.h>

namespace xestor
{
// Onde van os bundles que se quitan: nunca se borran, van ao Lixo.
class Papeleira
{
public:
    virtual ~Papeleira() = default;
    // Move o elemento ao Lixo; en «onde», onde quedou (se se sabe)
    virtual juce::Result botar (const juce::File&, juce::File* onde = nullptr) const = 0;
};

// O Lixo de verdade: o do Finder en Mac («Volver poñer» funciona), a Papeleira de reciclaxe en
// Windows e o Lixo de freedesktop.org en Linux (~/.local/share/Trash, co seu .trashinfo).
class LixoDoSistema : public Papeleira
{
public:
    juce::Result botar (const juce::File&, juce::File* onde = nullptr) const override;
};

// Un cartafol calquera feito Lixo (para as probas e para --papeleira).
class CartafolLixo : public Papeleira
{
public:
    explicit CartafolLixo (juce::File c) : cartafol (std::move (c)) {}
    juce::Result botar (const juce::File&, juce::File* onde = nullptr) const override;
    const juce::File cartafol;
};

// Move un ficheiro ou cartafol (se non se pode renomear, porque é outro disco, copia e despois quita
// o orixinal). O destino non pode existir.
juce::Result mover (const juce::File& de, const juce::File& a);
// Un nome libre no cartafol: «Barullo FM.vst3», «Barullo FM 2.vst3»...
juce::File nomeLibre (const juce::File& cartafol, const juce::String& nome);

#if JUCE_MAC
juce::Result botarNoLixoMac (const juce::File&, juce::File* onde);   // Papeleira_mac.mm
#endif
} // namespace xestor

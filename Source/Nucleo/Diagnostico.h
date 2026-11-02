#pragma once
#include "Estado.h"
#include "Copias.h"

namespace xestor
{
// O informe para pedir axuda («Copiar informe»): o sistema, as versións do xestor e de cada plugin,
// onde está cada copia, as duplicadas e (en Mac) a firma. As rutas da casa van coma «~».
// Vai sempre en galego (é para quen o le).
struct DatosInforme
{
    juce::String versionXestor, so, idioma;
    std::vector<Situacion> situacions;
    std::vector<Instalado> orfos;
    Rutas rutas;
    juce::StringArray avisos, daws;
    int restantesAPI = -1;
    bool senRede = false;
    juce::Time dataCatalogo;
    const Copias* copias = nullptr;
    // a firma de cada copia (Mac: sistema::estadoFirma); se non hai, non se pon
    std::function<juce::String (const juce::File&)> firma;
};

namespace diagnostico
{
    juce::String informe (const DatosInforme&);
    // «/Users/x/Library/...» → «~/Library/...»
    juce::String curta (const juce::File&, const juce::File& casa = juce::File::getSpecialLocation (juce::File::userHomeDirectory));
}
} // namespace xestor

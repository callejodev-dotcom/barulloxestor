#pragma once
#include "Escaner.h"

namespace xestor
{
// Como está un produto nesta máquina, mirando só os formatos escollidos.
enum class Estado { alDia, actualizacion, copiaVella, incompleto, nonInstalado, soLocal, senDatos };

juce::String clave (Estado);      // «alDia»... (para o JSON)
juce::String etiqueta (Estado);   // «ao día», «actualización»...
// Hai algo que facer con «Actualizar todo»?
inline bool pideActualizar (Estado e) { return e == Estado::actualizacion || e == Estado::incompleto || e == Estado::copiaVella; }

struct Situacion
{
    Produto produto;
    std::vector<Instalado> instalados;   // todas as copias, en todos os formatos
    std::vector<Formato> formatos;

    std::vector<Instalado> relevantes() const;   // as dos formatos escollidos
    std::vector<Instalado> vellas() const;       // as máis vellas cá última publicada
    std::vector<Instalado> vellasDoSistema() const;
    juce::StringArray faltan() const;            // plugins dunha serie sen ningunha copia (se outros si a teñen)
    std::vector<Instalado> duplicados() const;   // á vez no cartafol do usuario e no do sistema
    std::optional<Version> versionInstalada() const;   // a máis baixa (a que manda á hora de actualizar)
    Estado estado() const;
};
} // namespace xestor

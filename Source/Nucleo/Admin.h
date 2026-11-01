#pragma once
#include "Rutas.h"

namespace xestor
{
// Pedir permiso de administrador só para o que o precisa: sacar un bundle dun cartafol do sistema
// (/Library/Audio/Plug-Ins, C:\Program Files\Common Files\VST3, /usr/lib/vst3...).
//
// O paso con privilexios é sempre o mesmo e moi pequeno: MOVER o bundle do sistema a un cartafol
// temporal do usuario (e, en Mac e Linux, devolverllo ao usuario con chown). Despois, xa sen
// privilexios, o xestor lévao ao Lixo coma calquera outro. Así:
//   - o proceso de root non toca nunca ~/.Trash, ~/Downloads nin ~/Documents (en Mac, TCC
//     non llo deixaría a un root lanzado desde osascript);
//   - no Lixo queda co dono de sempre e «Volver poñer» funciona coma con todo.
// Para instalar non fai falla: instálase sempre no cartafol do usuario.
//
//   Mac:     osascript «do shell script … with administrator privileges» (o diálogo do sistema)
//   Linux:   pkexec /bin/sh -c … (o diálogo de polkit)
//   Windows: o propio xestor con «--operacions <ficheiro>», lanzado con ShellExecuteEx «runas» (UAC)
struct MovementoAdmin
{
    juce::File de, a;
};

class Elevador
{
public:
    virtual ~Elevador() = default;
    // Fai todos os movementos cun só permiso. «motivo» vai no diálogo do sistema (Mac).
    virtual juce::Result mover (const std::vector<MovementoAdmin>&, const juce::String& motivo) = 0;
};

// O texto co que vén un Result se a persoa dixo que non no diálogo
bool foiCancelado (const juce::Result&);
juce::Result cancelado();

// O de verdade, segundo o sistema
class ElevadorSistema : public Elevador
{
public:
    juce::Result mover (const std::vector<MovementoAdmin>&, const juce::String& motivo) override;
};

// Fai exactamente o mesmo, pero sen pedir permiso (o mesmo guion de sh en Mac e Linux; as mesmas
// operacións en Windows). Para as probas en cartafoles temporais e para --simular-admin.
class ElevadorSenPermiso : public Elevador
{
public:
    juce::Result mover (const std::vector<MovementoAdmin>&, const juce::String& motivo) override;
    int chamadas = 0;
    juce::String ultimoGuion, ultimoMotivo;
    bool cancelar = false;   // facer coma se a persoa dixese que non
};

// Nunca pide permiso (--sen-admin): o que precise admin queda onde está
class ElevadorNon : public Elevador
{
public:
    juce::Result mover (const std::vector<MovementoAdmin>&, const juce::String&) override
    {
        return juce::Result::fail ("precisa permiso de administrador (--sen-admin)");
    }
};

namespace admin
{
    juce::String citar (const juce::String&);   // 'texto' para sh (con ' escapadas)
    // o guion de sh: mv de cada un ao seu destino (que non pode existir) e chown ao usuario
    juce::String guionPosix (const std::vector<MovementoAdmin>&, int uid, int gid);
    // a orde de AppleScript que o executa con privilexios
    juce::String appleScript (const juce::String& guion, const juce::String& motivo);
    // Windows: as operacións en JSON, e o que fai o xestor elevado con «--operacions <ficheiro>».
    // Só move desde os cartafoles do sistema de «permitidos» ata o cartafol temporal.
    juce::String operacionsJson (const std::vector<MovementoAdmin>&);
    juce::Result executarOperacions (const juce::String& json, const std::vector<juce::File>& orixesPermitidas,
                                     const juce::File& destinoPermitido);
    // os cartafoles do sistema deste ordenador (de onde se pode sacar algo con --operacions)
    std::vector<juce::File> cartafolesDoSistema();
}
} // namespace xestor

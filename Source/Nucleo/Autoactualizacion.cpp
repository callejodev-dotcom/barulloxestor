#include "Autoactualizacion.h"
#include "Proceso.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#else
 #include <unistd.h>
#endif

namespace xestor
{
namespace autoactualizacion
{
juce::String repo() { return "callejodev-dotcom/barulloxestor"; }

const Activo* activo (const Lanzamento& l, SO so)
{
    const juce::String sufixo = so == SO::mac ? "-mac.zip" : so == SO::win ? "-windows-x64-instalador.exe" : "-linux-x86_64.tar.gz";
    for (auto& a : l.activos)
        if (a.nome.startsWithIgnoreCase ("BarulloXestor-") && a.nome.toLowerCase().endsWith (sufixo)) return &a;
    return nullptr;
}

std::optional<Lanzamento> nova (const std::optional<Lanzamento>& l, const juce::String& versionActual, SO so)
{
    if (! l) return std::nullopt;
    const auto v = l->version();
    const auto a = Version::ler (versionActual);
    if (! v || ! a || ! (*v > *a) || activo (*l, so) == nullptr) return std::nullopt;
    return l;
}

juce::File appActual()
{
    return juce::File::getSpecialLocation (juce::File::currentApplicationFile);
}

juce::File destino (const juce::File& actual, SO so)
{
    const auto casa = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    const bool provisional = actual.getFullPathName().contains ("/AppTranslocation/") || actual.getFullPathName().startsWith ("/Volumes/");
    const bool escribible = actual.exists() && actual.getParentDirectory().hasWriteAccess();
    if (so == SO::mac)
    {
        if (! provisional && escribible && actual.getFileName().endsWithIgnoreCase (".app")) return actual;
        return casa.getChildFile ("Applications").getChildFile ("Barullo Xestor.app");
    }
    if (so == SO::lin)
    {
        if (escribible && actual.existsAsFile()) return actual;
        return casa.getChildFile (".local/bin/barullo-xestor");
    }
    return actual;   // Windows: o instalador sabe onde
}

static juce::File buscar (const juce::File& dir, const juce::String& nome, int nivel = 0)
{
    if (nivel > 3) return {};
    for (auto& f : dir.findChildFiles (juce::File::findFilesAndDirectories, false))
    {
        if (f.getFileName() == "__MACOSX") continue;
        if (f.getFileName().equalsIgnoreCase (nome)) return f;
        if (f.isDirectory() && ! f.getFileName().endsWithIgnoreCase (".app"))
            if (auto r = buscar (f, nome, nivel + 1); r != juce::File()) return r;
    }
    return {};
}

juce::Result preparar (const Lanzamento& l, Transporte& t, const juce::File& traballo, juce::File& lista,
                       std::function<bool (double)> progreso, SO so)
{
    const auto* a = activo (l, so);
    if (a == nullptr) return juce::Result::fail (U ("A versión ") + l.tag + U (" non trae o arquivo para este sistema"));
    traballo.createDirectory();
    const auto arq = traballo.getChildFile (a->nome);
    auto r = t.descargar (a->url, arq, progreso);
    if (r.failed()) return r;
    r = verificar (arq, *a);
    if (r.failed()) return r;
    if (so == SO::win) { lista = arq; return juce::Result::ok(); }

    const auto aberto = traballo.getChildFile ("aberto");
    r = Instalador::extraer (arq, aberto, so);
    if (r.failed()) return r;
    lista = buscar (aberto, so == SO::mac ? "Barullo Xestor.app" : "barullo-xestor");
    if (lista == juce::File()) return juce::Result::fail (U ("O arquivo ") + a->nome + U (" non leva Barullo Xestor"));
    if (so == SO::mac && soActual() == SO::mac)
    {
        Instalador::quitarCuarentena (lista);
        juce::String aviso;
        r = Instalador::asegurarFirma (lista, aviso);
        if (r.failed()) return r;
    }
    if (so == SO::lin) lista.setExecutePermission (true);
    return juce::Result::ok();
}

juce::Result substituir (const juce::File& nova, const juce::File& dest, const Papeleira& papeleira)
{
    dest.getParentDirectory().createDirectory();
    // 1) a nova, ao lado (pode vir doutro disco: copia)
    const auto provisional = dest.getParentDirectory().getChildFile ("." + dest.getFileName() + ".novo-"
                                                                     + juce::String::toHexString (juce::Random::getSystemRandom().nextInt()));
    auto r = mover (nova, provisional);
    if (r.failed()) return r;
    // 2) a vella, ao Lixo
    if (dest.exists())
    {
        r = papeleira.botar (dest);
        if (r.failed() || dest.exists())
        {
            provisional.isDirectory() ? provisional.deleteRecursively() : provisional.deleteFile();
            return juce::Result::fail (U ("Non se puido quitar a versión de agora: ") + r.getErrorMessage());
        }
    }
    // 3) a nova, ao seu sitio
    r = mover (provisional, dest);
    if (r.failed()) return juce::Result::fail (r.getErrorMessage() + U (" (a nova quedou en ") + provisional.getFullPathName() + ")");
    if (dest.existsAsFile()) dest.setExecutePermission (true);
    return juce::Result::ok();
}

juce::StringArray ordeReinicio (SO so, int pid, const juce::File& app, const juce::File& guion)
{
    // agarda (como moito un minuto) a que remate este proceso e abre a nova; a ruta vai coma $0
    const juce::String agardar = "n=0; while kill -0 " + juce::String (pid) + " 2>/dev/null && [ $n -lt 300 ]; do sleep 0.2; n=$((n+1)); done; ";
    if (so == SO::mac) return { "/bin/sh", "-c", agardar + "exec /usr/bin/open \"$0\"", app.getFullPathName() };
    if (so == SO::lin) return { "/bin/sh", "-c", agardar + "exec \"$0\"", app.getFullPathName() };
    return { "cmd.exe", "/c", guion.getFullPathName() };
}

juce::String guionWindows (int pid, const juce::File& instalador, const juce::File& exe)
{
    const auto p = juce::String (pid);
    juce::StringArray l {
        "@echo off",
        "rem Barullo Xestor: agarda a que se peche, instala a nova en silencio e ábrea",
        "set n=0",
        ":agarda",
        "set /a n+=1",
        "if %n% gtr 60 goto instala",
        "tasklist /fi \"PID eq " + p + "\" /nh | find \"" + p + "\" >nul",
        "if errorlevel 1 goto instala",
        "ping -n 2 127.0.0.1 >nul",
        "goto agarda",
        ":instala",
        "\"" + instalador.getFullPathName() + "\" /VERYSILENT /SUPPRESSMSGBOXES /NORESTART",
        "start \"\" \"" + exe.getFullPathName() + "\"",
        "del \"%~f0\"" };
    return l.joinIntoString ("\r\n") + "\r\n";
}

juce::File exeTrasInstalar (const juce::File& actual)
{
    if (actual.getSiblingFile ("unins000.exe").existsAsFile()) return actual;
    const auto local = juce::SystemStats::getEnvironmentVariable ("LOCALAPPDATA", {});
    if (local.isEmpty()) return actual;
    return juce::File (local).getChildFile ("Programs").getChildFile ("Barullo Xestor").getChildFile ("Barullo Xestor.exe");
}

int pidActual()
{
   #if JUCE_WINDOWS
    return (int) GetCurrentProcessId();
   #else
    return (int) getpid();
   #endif
}

bool lanzar (const juce::StringArray& orde)
{
    // sen recoller a saída (vai a /dev/null) e sen esperar: o destrutor de ChildProcess non o mata,
    // e segue vivo cando este proceso remata
    juce::ChildProcess p;
    return p.start (orde, 0);
}
} // namespace autoactualizacion

namespace comprobacion
{
bool toca (juce::int64 ultimaMs, juce::Time agora, juce::RelativeTime cada)
{
    if (ultimaMs <= 0) return true;
    const auto ultima = juce::Time (ultimaMs);
    return ultima > agora || agora - ultima >= cada;
}

juce::StringArray novidades (const juce::StringArray& agora, const juce::StringArray& avisadas)
{
    juce::StringArray r;
    for (auto& a : agora)
        if (! avisadas.contains (a)) r.addIfNotAlreadyThere (a);
    return r;
}
} // namespace comprobacion
} // namespace xestor

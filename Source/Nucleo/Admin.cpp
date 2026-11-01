#include "Admin.h"
#include "Proceso.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <shellapi.h>
 #if defined (_MSC_VER)
  #pragma comment (lib, "shell32.lib")
 #endif
#else
 #include <unistd.h>
#endif

namespace xestor
{
static const char* const textoCancelado = "cancelado: non se deu o permiso de administrador";

bool foiCancelado (const juce::Result& r) { return r.failed() && r.getErrorMessage() == textoCancelado; }
juce::Result cancelado() { return juce::Result::fail (textoCancelado); }

namespace admin
{
juce::String citar (const juce::String& s) { return "'" + s.replace ("'", "'\\''") + "'"; }

juce::String guionPosix (const std::vector<MovementoAdmin>& ms, int uid, int gid)
{
    juce::StringArray l;
    l.add ("set -e");
    for (auto& m : ms)
    {
        const auto de = citar (m.de.getFullPathName()), a = citar (m.a.getFullPathName());
        // nunca se pisa nada: se o destino xa existe, para
        l.add ("if [ -e " + a + " ]; then echo 'xa existe o destino' >&2; exit 3; fi");
        l.add ("mv " + de + " " + a);
        l.add ("chown -R " + juce::String (uid) + ":" + juce::String (gid) + " " + a);
    }
    return l.joinIntoString ("; ");
}

juce::String appleScript (const juce::String& guion, const juce::String& motivo)
{
    auto literal = [] (const juce::String& s) { return "\"" + s.replace ("\\", "\\\\").replace ("\"", "\\\"") + "\""; };
    return "do shell script " + literal (guion) + " with prompt " + literal (motivo) + " with administrator privileges";
}

juce::String operacionsJson (const std::vector<MovementoAdmin>& ms)
{
    juce::Array<juce::var> a;
    for (auto& m : ms)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("de", m.de.getFullPathName());
        o->setProperty ("a", m.a.getFullPathName());
        a.add (juce::var (o));
    }
    return juce::JSON::toString (juce::var (a));
}

juce::Result executarOperacions (const juce::String& json, const std::vector<juce::File>& orixes, const juce::File& destino)
{
    const auto v = juce::JSON::parse (json);
    auto* lista = v.getArray();
    if (lista == nullptr) return juce::Result::fail (U ("operacións ilexibles"));
    // primeiro compróbase todo; non se move nada se algo non é dun sitio permitido
    std::vector<MovementoAdmin> ms;
    for (auto& o : *lista)
    {
        MovementoAdmin m { juce::File (o["de"].toString()), juce::File (o["a"].toString()) };
        bool orixeOk = false;
        for (auto& c : orixes) orixeOk = orixeOk || m.de.isAChildOf (c);
        if (! orixeOk) return juce::Result::fail ("non se pode sacar " + m.de.getFullPathName());
        if (! m.a.isAChildOf (destino)) return juce::Result::fail ("non se pode levar a " + m.a.getFullPathName());
        ms.push_back (m);
    }
    for (auto& m : ms)
    {
        if (m.a.exists()) return juce::Result::fail ("xa existe " + m.a.getFullPathName());
        m.a.getParentDirectory().createDirectory();
        if (! m.de.moveFileTo (m.a))
        {
            const bool ok = m.de.isDirectory() ? (m.de.copyDirectoryTo (m.a) && m.de.deleteRecursively()) : (m.de.copyFileTo (m.a) && m.de.deleteFile());
            if (! ok) return juce::Result::fail ("non se puido mover " + m.de.getFullPathName());
        }
    }
    return juce::Result::ok();
}

std::vector<juce::File> cartafolesDoSistema()
{
    std::vector<juce::File> r;
    for (auto& u : Rutas::reais().sistema)
        for (auto& [f, c] : u.cartafoles) r.push_back (c);
    return r;
}
} // namespace admin

// ============================================================== o de verdade
juce::Result ElevadorSistema::mover (const std::vector<MovementoAdmin>& ms, const juce::String& motivo)
{
    if (ms.empty()) return juce::Result::ok();
   #if JUCE_MAC
    const auto guion = admin::guionPosix (ms, (int) getuid(), (int) getgid());
    auto r = executar ({ "/usr/bin/osascript", "-e", admin::appleScript (guion, motivo) }, 10 * 60 * 1000);
    if (r.ok()) return juce::Result::ok();
    if (r.saida.contains ("-128")) return cancelado();   // «User canceled»
    return juce::Result::fail ("osascript: " + r.saida.trim());
   #elif JUCE_LINUX || JUCE_BSD
    juce::ignoreUnused (motivo);
    const auto pkexec = atoparPrograma ("pkexec", { "/usr/bin/pkexec", "/bin/pkexec" });
    if (pkexec.isEmpty())
        return juce::Result::fail (juce::String::fromUTF8 ("fai falla pkexec (polkit) para pedir o permiso de administrador"));
    const auto guion = admin::guionPosix (ms, (int) getuid(), (int) getgid());
    auto r = executar ({ pkexec, "/bin/sh", "-c", guion }, 10 * 60 * 1000);
    if (r.ok()) return juce::Result::ok();
    if (r.codigo == 126 || r.codigo == 127) return cancelado();   // pechouse o diálogo ou non se autorizou
    return juce::Result::fail ("pkexec: " + r.saida.trim());
   #elif JUCE_WINDOWS
    juce::ignoreUnused (motivo);
    const auto ops = juce::File::createTempFile (".json");
    ops.replaceWithText (admin::operacionsJson (ms));
    const auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
    const juce::String params = "--operacions \"" + ops.getFullPathName() + "\"";
    const juce::String ruta = exe.getFullPathName();
    SHELLEXECUTEINFOW sei {};
    sei.cbSize = sizeof (sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    sei.lpVerb = L"runas";   // o diálogo do UAC
    sei.lpFile = ruta.toWideCharPointer();
    sei.lpParameters = params.toWideCharPointer();
    sei.nShow = SW_HIDE;
    if (! ShellExecuteExW (&sei))
    {
        const auto e = GetLastError();
        ops.deleteFile();
        if (e == ERROR_CANCELLED) return cancelado();
        return juce::Result::fail ("non se puido pedir o permiso de administrador (erro " + juce::String ((int) e) + ")");
    }
    WaitForSingleObject (sei.hProcess, INFINITE);
    DWORD codigo = 1;
    GetExitCodeProcess (sei.hProcess, &codigo);
    CloseHandle (sei.hProcess);
    ops.deleteFile();
    return codigo == 0 ? juce::Result::ok() : juce::Result::fail ("o paso con permiso de administrador fallou (" + juce::String ((int) codigo) + ")");
   #else
    juce::ignoreUnused (motivo);
    return juce::Result::fail ("este sistema non sabe pedir permiso de administrador");
   #endif
}

// ============================================================== o mesmo, sen permiso (probas)
juce::Result ElevadorSenPermiso::mover (const std::vector<MovementoAdmin>& ms, const juce::String& motivo)
{
    ++chamadas;
    ultimoMotivo = motivo;
    if (cancelar) return cancelado();
   #if JUCE_WINDOWS
    ultimoGuion = admin::operacionsJson (ms);
    std::vector<juce::File> orixes;
    for (auto& m : ms) orixes.push_back (m.de.getParentDirectory());
    juce::File destino = ms.empty() ? juce::File() : ms.front().a.getParentDirectory().getParentDirectory();
    return admin::executarOperacions (ultimoGuion, orixes, destino);
   #else
    ultimoGuion = admin::guionPosix (ms, (int) getuid(), (int) getgid());
    auto r = executar ({ "/bin/sh", "-c", ultimoGuion });
    return r.ok() ? juce::Result::ok() : juce::Result::fail ("sh: " + r.saida.trim());
   #endif
}
} // namespace xestor

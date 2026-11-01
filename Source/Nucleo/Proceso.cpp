#include "Proceso.h"

namespace xestor
{
ResultadoProceso executar (const juce::StringArray& orde, int tempoMaximoMs)
{
    ResultadoProceso r;
    juce::ChildProcess p;
    if (! p.start (orde, juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr)) return r;
    r.arrancou = true;
    // lese mentres corre, para que unha saída longa non o bloquee
    juce::MemoryOutputStream mo;
    const auto limite = juce::Time::getMillisecondCounter() + (juce::uint32) tempoMaximoMs;
    char buf[4096];
    for (;;)
    {
        const int n = p.readProcessOutput (buf, sizeof (buf));
        if (n > 0) { mo.write (buf, (size_t) n); continue; }
        if (! p.isRunning()) break;
        if (juce::Time::getMillisecondCounter() > limite) { p.kill(); break; }
        juce::Thread::sleep (10);
    }
    r.saida = mo.toUTF8();
    r.codigo = (int) p.getExitCode();
    return r;
}

juce::String atoparPrograma (const juce::String& nome, const juce::StringArray& candidatos)
{
    for (auto& c : candidatos)
        if (juce::File (c).existsAsFile()) return c;
    const auto path = juce::SystemStats::getEnvironmentVariable ("PATH", {});
   #if JUCE_WINDOWS
    const auto sep = ";"; const juce::String ext = ".exe";
   #else
    const auto sep = ":"; const juce::String ext;
   #endif
    for (auto& d : juce::StringArray::fromTokens (path, sep, ""))
    {
        if (d.isEmpty()) continue;
        auto f = juce::File (d).getChildFile (nome + ext);
        if (f.existsAsFile()) return f.getFullPathName();
    }
    return {};
}
} // namespace xestor

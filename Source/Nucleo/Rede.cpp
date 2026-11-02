#include "Rede.h"
#include <juce_cryptography/juce_cryptography.h>

namespace xestor
{
#if ! JUCE_MAC
static juce::String cabeceirasTexto (const juce::StringPairArray& c)
{
    juce::String r;
    for (int i = 0; i < c.size(); ++i)
        r << c.getAllKeys()[i] << ": " << c.getAllValues()[i] << "\r\n";
    return r;
}
#endif

Resposta TransporteHttp::pedir (const juce::String& url, const juce::StringPairArray& cabeceiras)
{
    const Cancelacion nunca;
    return pedirCancelable (url, cabeceiras, nunca);
}

Resposta TransporteHttp::pedirCancelable (const juce::String& url, const juce::StringPairArray& cabeceiras, const Cancelacion& cancelacion)
{
    auto c = cabeceiras;
    c.set ("User-Agent", "BarulloXestor");
   #if JUCE_MAC
    return redeMac::pedir (url, c, cancelacion, limitePedidoMs);
   #else
    Resposta r;
    if (cancelacion.cancelado()) { r.erro = U ("cancelado"); return r; }
    auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                    .withExtraHeaders (cabeceirasTexto (c))
                    .withConnectionTimeoutMs (20000)
                    .withResponseHeaders (&r.cabeceiras)
                    .withStatusCode (&r.estado)
                    .withNumRedirectsToFollow (5);
    auto s = juce::URL (url).createInputStream (opts);
    if (s == nullptr)
    {
        if (r.estado == 0) r.erro = U ("sen conexión");
        return r;
    }
    // o corpo a anacos, mirando se se cancelou e o tempo máximo
    const auto inicio = juce::Time::getMillisecondCounter();
    juce::HeapBlock<char> buf (1 << 15);
    for (;;)
    {
        if (cancelacion.cancelado() || juce::Time::getMillisecondCounter() - inicio > (juce::uint32) limitePedidoMs)
        {
            r.estado = 0;
            r.corpo.reset();
            r.erro = cancelacion.cancelado() ? U ("cancelado") : U ("GitHub non respondeu a tempo");
            return r;
        }
        const int n = s->read (buf.get(), 1 << 15);
        if (n <= 0) break;
        r.corpo.append (buf.get(), (size_t) n);
    }
    return r;
   #endif
}

juce::Result TransporteHttp::descargar (const juce::String& url, const juce::File& destino, std::function<bool (double)> progreso)
{
    if (url.startsWith ("file:") || juce::File::isAbsolutePath (url))
    {
        auto res = copiarLocal (url, destino);
        if (res.wasOk() && progreso) progreso (1.0);
        return res;
    }
   #if JUCE_MAC
    return redeMac::descargar (url, destino, std::move (progreso), limiteInactivoMs);
   #else
    int estado = 0;
    juce::StringPairArray cab;
    auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                    .withExtraHeaders ("User-Agent: BarulloXestor\r\n")
                    .withConnectionTimeoutMs (60000)
                    .withResponseHeaders (&cab)
                    .withStatusCode (&estado)
                    .withNumRedirectsToFollow (8);
    auto s = juce::URL (url).createInputStream (opts);
    if (s == nullptr || (estado != 0 && estado != 200))
        return juce::Result::fail (estado != 0 ? "A descarga fallou (HTTP " + juce::String (estado) + ")" : U ("Sen conexión para descargar"));
    destino.deleteFile();
    juce::FileOutputStream out (destino);
    if (! out.openedOk()) return juce::Result::fail ("Non se pode escribir " + destino.getFullPathName());
    const auto total = s->getTotalLength();
    juce::HeapBlock<char> buf (1 << 16);
    juce::int64 levo = 0;
    for (;;)
    {
        const int n = s->read (buf.get(), 1 << 16);
        if (n <= 0) break;
        out.write (buf.get(), (size_t) n);
        levo += n;
        if (progreso && ! progreso (total > 0 ? (double) levo / (double) total : 0.0))
            return juce::Result::fail ("Descarga cancelada");
    }
    out.flush();
    if (total > 0 && levo != total) return juce::Result::fail ("A descarga quedou a medias");
    if (progreso) progreso (1.0);
    return juce::Result::ok();
   #endif
}

juce::Result copiarLocal (const juce::String& url, const juce::File& destino)
{
    const juce::File f = url.startsWith ("file:") ? juce::URL (url).getLocalFile() : juce::File (url);
    if (! f.existsAsFile()) return juce::Result::fail ("Non existe " + f.getFullPathName());
    destino.deleteFile();
    return f.copyFileTo (destino) ? juce::Result::ok() : juce::Result::fail ("Non se puido copiar " + f.getFullPathName());
}

juce::String sha256 (const juce::File& f)
{
    juce::FileInputStream in (f);
    if (! in.openedOk()) return {};
    return juce::SHA256 (in).toHexString().toLowerCase();
}

juce::Result verificar (const juce::File& f, const Activo& a)
{
    const auto t = f.getSize();
    if (a.tamano > 0 && t != a.tamano)
        return juce::Result::fail ("O ficheiro descargado non mide o que debe (" + juce::String (t) + " bytes, e son " + juce::String (a.tamano) + ")");
    if (a.sha256.isNotEmpty())
    {
        const auto r = sha256 (f);
        if (r != a.sha256.toLowerCase())
            return juce::Result::fail (juce::String (juce::CharPointer_UTF8 ("A suma SHA-256 do ficheiro non coincide ("))
                                       + r.substring (0, 12) + juce::String (juce::CharPointer_UTF8 ("…, e é "))
                                       + a.sha256.substring (0, 12) + juce::String (juce::CharPointer_UTF8 ("…)")));
    }
    return juce::Result::ok();
}

juce::String tamanoLexible (juce::int64 b)
{
    if (b < 1024) return juce::String (b) + " B";
    if (b < 1024 * 1024) return juce::String (juce::roundToInt ((double) b / 1024.0)) + " kB";
    return juce::String ((double) b / (1024.0 * 1024.0), 1).replaceCharacter ('.', ',') + " MB";
}
} // namespace xestor

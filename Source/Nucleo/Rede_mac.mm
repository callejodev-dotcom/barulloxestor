// A rede no Mac cunha NSURLSession propia. O URL de JUCE (8.0.4) comparte unha sesión e, ao
// destruír unha conexión, agarda a que a sesión diga «rematou»; con moitas á vez ese aviso pode
// non chegar nunca e o fío queda colgado para sempre (e con el, o peche da app). Aquí cada pedido
// ten a súa sesión efémera, agárdase cun semáforo e un tempo máximo, e ao rematar (ou cancelar)
// nunca se agarda por ela.
#include "Rede.h"
#import <Foundation/Foundation.h>
#include <memory>

namespace xestor::redeMac
{
namespace
{
NSString* ns (const juce::String& s) { return [NSString stringWithUTF8String: s.toRawUTF8()]; }
juce::String js (NSString* s) { return s != nil ? juce::String::fromUTF8 ([s UTF8String]) : juce::String(); }

juce::String textoErro (NSError* e)
{
    if (e == nil) return {};
    switch ((int) e.code)
    {
        case NSURLErrorTimedOut:                 return juce::String::fromUTF8 ("GitHub non respondeu a tempo");
        case NSURLErrorCancelled:                return "cancelado";
        case NSURLErrorNotConnectedToInternet:
        case NSURLErrorCannotFindHost:
        case NSURLErrorCannotConnectToHost:
        case NSURLErrorNetworkConnectionLost:
        case NSURLErrorDNSLookupFailed:          return juce::String::fromUTF8 ("sen conexión");
        default:                                 return js (e.localizedDescription);
    }
}

NSURLSession* sesion (double pedidoS, double recursoS)
{
    NSURLSessionConfiguration* c = [NSURLSessionConfiguration ephemeralSessionConfiguration];
    c.timeoutIntervalForRequest = pedidoS;
    c.timeoutIntervalForResource = recursoS;
    c.requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;
    c.URLCache = nil;
    return [NSURLSession sessionWithConfiguration: c];
}

// o que comparten o fío que agarda e o bloque que remata (pode chegar tarde: por iso é compartido)
struct Compartido
{
    Resposta r;
    juce::String erroDescarga;
    bool feito = false;
};

void cabeceiras (NSURLResponse* resp, Resposta& r)
{
    if (! [resp isKindOfClass: [NSHTTPURLResponse class]]) return;
    auto* h = (NSHTTPURLResponse*) resp;
    r.estado = (int) h.statusCode;
    [h.allHeaderFields enumerateKeysAndObjectsUsingBlock: ^(id k, id v, BOOL*)
    {
        r.cabeceiras.set (js ([k description]), js ([v description]));
    }];
}
} // namespace

Resposta pedir (const juce::String& url, const juce::StringPairArray& cab, const Cancelacion& cancelacion, int limiteMs)
{
    @autoreleasepool
    {
        Resposta fallo;
        if (cancelacion.cancelado()) { fallo.erro = "cancelado"; return fallo; }
        NSURL* u = [NSURL URLWithString: ns (url)];
        if (u == nil) { fallo.erro = juce::String::fromUTF8 ("URL non válido"); return fallo; }
        NSMutableURLRequest* req = [NSMutableURLRequest requestWithURL: u];
        for (int i = 0; i < cab.size(); ++i)
            [req setValue: ns (cab.getAllValues()[i]) forHTTPHeaderField: ns (cab.getAllKeys()[i])];

        NSURLSession* s = sesion (limiteMs / 1000.0, limiteMs / 1000.0 + 5.0);
        auto comp = std::make_shared<Compartido>();
        dispatch_semaphore_t sem = dispatch_semaphore_create (0);
        NSURLSessionDataTask* t = [s dataTaskWithRequest: req completionHandler: ^(NSData* d, NSURLResponse* resp, NSError* e)
        {
            if (e != nil) comp->r.erro = textoErro (e);
            else
            {
                cabeceiras (resp, comp->r);
                if (d != nil) comp->r.corpo.append (d.bytes, (size_t) d.length);
            }
            comp->feito = true;
            dispatch_semaphore_signal (sem);
        }];
        [t resume];
        const auto inicio = juce::Time::getMillisecondCounter();
        for (;;)
        {
            if (dispatch_semaphore_wait (sem, dispatch_time (DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC)) == 0) break;
            const bool tarde = juce::Time::getMillisecondCounter() - inicio > (juce::uint32) limiteMs + 3000;
            if (cancelacion.cancelado() || tarde)
            {
                // non se agarda máis: o bloque, se chega, só toca «comp» (que segue vivo)
                [t cancel];
                [s invalidateAndCancel];
                fallo.erro = cancelacion.cancelado() ? juce::String ("cancelado") : juce::String::fromUTF8 ("GitHub non respondeu a tempo");
                return fallo;
            }
        }
        [s finishTasksAndInvalidate];
        return comp->r;
    }
}

juce::Result descargar (const juce::String& url, const juce::File& destino, std::function<bool (double)> progreso, int inactivoMs)
{
    @autoreleasepool
    {
        NSURL* u = [NSURL URLWithString: ns (url)];
        if (u == nil) return juce::Result::fail (juce::String::fromUTF8 ("URL non válido"));
        NSMutableURLRequest* req = [NSMutableURLRequest requestWithURL: u];
        [req setValue: @"BarulloXestor" forHTTPHeaderField: @"User-Agent"];
        NSURLSession* s = sesion (inactivoMs / 1000.0, 4.0 * 3600.0);
        auto comp = std::make_shared<Compartido>();
        destino.deleteFile();
        destino.getParentDirectory().createDirectory();
        NSString* dest = ns (destino.getFullPathName());
        dispatch_semaphore_t sem = dispatch_semaphore_create (0);
        NSURLSessionDownloadTask* t = [s downloadTaskWithRequest: req completionHandler: ^(NSURL* local, NSURLResponse* resp, NSError* e)
        {
            if (e != nil) comp->erroDescarga = textoErro (e);
            else
            {
                cabeceiras (resp, comp->r);
                // o temporal bórrase ao saír deste bloque: móvese xa ao seu sitio
                NSError* em = nil;
                if (comp->r.estado == 200 && local != nil
                    && ! [[NSFileManager defaultManager] moveItemAtURL: local toURL: [NSURL fileURLWithPath: dest] error: &em])
                    comp->erroDescarga = js (em.localizedDescription);
            }
            comp->feito = true;
            dispatch_semaphore_signal (sem);
        }];
        [t resume];
        for (;;)
        {
            if (dispatch_semaphore_wait (sem, dispatch_time (DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC)) == 0) break;
            const auto total = t.countOfBytesExpectedToReceive, levo = t.countOfBytesReceived;
            const double v = total > 0 ? juce::jlimit (0.0, 0.999, (double) levo / (double) total) : 0.0;
            if (progreso && ! progreso (v))
            {
                [t cancel];
                [s invalidateAndCancel];
                return juce::Result::fail ("Descarga cancelada");
            }
        }
        [s finishTasksAndInvalidate];
        if (comp->erroDescarga.isNotEmpty())
            return juce::Result::fail (comp->erroDescarga == juce::String::fromUTF8 ("sen conexión") ? juce::String::fromUTF8 ("Sen conexión para descargar")
                                                                                                      : "A descarga fallou: " + comp->erroDescarga);
        if (comp->r.estado != 200) return juce::Result::fail ("A descarga fallou (HTTP " + juce::String (comp->r.estado) + ")");
        if (! destino.existsAsFile()) return juce::Result::fail ("A descarga quedou a medias");
        if (progreso) progreso (1.0);
        return juce::Result::ok();
    }
}
} // namespace xestor::redeMac

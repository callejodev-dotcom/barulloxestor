#include "Instalador.h"
#include "Proceso.h"

#if JUCE_MAC
 #include <sys/xattr.h>
#endif

namespace xestor
{
// ============================================================== informe
juce::String clave (Accion::Tipo t)
{
    switch (t)
    {
        case Accion::Tipo::instalado:   return "instalado";
        case Accion::Tipo::actualizado: return "actualizado";
        case Accion::Tipo::omitido:     return "omitido";
        case Accion::Tipo::aoLixo:      return "aoLixo";
        case Accion::Tipo::aviso:       return "aviso";
        case Accion::Tipo::erro:        return "erro";
    }
    return {};
}

juce::String Accion::texto() const
{
    return clave (tipo) + ": " + plugin + (formato ? " (" + etiqueta (*formato) + ")" : juce::String()) + U (" — ") + mensaxe;
}

bool Informe::ok() const
{
    for (auto& a : accions) if (a.tipo == Accion::Tipo::erro) return false;
    return true;
}

int Informe::cambios() const
{
    int n = 0;
    for (auto& a : accions)
        if (a.tipo == Accion::Tipo::instalado || a.tipo == Accion::Tipo::actualizado || a.tipo == Accion::Tipo::aoLixo) ++n;
    return n;
}

void Informe::xuntar (const Informe& o)
{
    accions.insert (accions.end(), o.accions.begin(), o.accions.end());
    conAdmin += o.conAdmin;
    cancelouAdmin = cancelouAdmin || o.cancelouAdmin;
}

// ============================================================== instalador
Instalador::Instalador (Rutas r, const Papeleira& p, Elevador* e, Transporte& t, juce::File tmp, Rexistro* rex, SO s)
    : rutas (std::move (r)), papeleira (p), elevador (e), transporte (t), temporal (std::move (tmp)), rexistro (rex), so (s) {}

juce::File Instalador::cartafolTemporal()
{
    return juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("BarulloXestor");
}

bool Instalador::sistemaBloqueado (const Ubicacion& u, Formato f) const
{
    return u.eSistema() && (simularAdmin || ! u.escribible (f));
}

Informe Instalador::instalar (const Produto& p, const std::vector<Formato>& formatos, bool forzar, std::function<bool (double)> progreso)
{
    if (! p.ultima) throw ErroInstalacion ((p.nome + U (" aínda non ten ningunha versión publicada")).toStdString());
    const auto* arq = p.ultima->arquivo (so);
    if (arq == nullptr)
        throw ErroInstalacion ((U ("A versión publicada de ") + p.nome + U (" non trae o arquivo para este sistema")).toStdString());
    const auto traballo = temporal.getChildFile (juce::Uuid().toString());
    traballo.createDirectory();
    struct Limpar { juce::File f; ~Limpar() { f.deleteRecursively(); } } limpar { traballo };
    const auto local = traballo.getChildFile (arq->nome);
    auto r = transporte.descargar (arq->url, local, progreso);
    if (r.failed()) throw ErroInstalacion (r.getErrorMessage().toStdString());
    r = verificar (local, *arq);
    if (r.failed()) throw ErroInstalacion (r.getErrorMessage().toStdString());
    return instalarArquivo (local, p, formatos, p.ultima->version(), forzar);
}

juce::Result Instalador::extraer (const juce::File& arq, const juce::File& destino, SO so)
{
    destino.createDirectory();
    const auto nome = arq.getFileName().toLowerCase();
    if (nome.endsWith (".tar.gz") || nome.endsWith (".tgz"))
    {
        auto r = executar ({ "tar", "-xzf", arq.getFullPathName(), "-C", destino.getFullPathName() });
        return r.ok() ? juce::Result::ok() : juce::Result::fail (U ("Non se puido abrir o .tar.gz: ") + r.saida.trim());
    }
    if (so == SO::mac && soActual() == SO::mac)
    {
        // ditto: mantén as firmas, os atributos e as ligazóns dos bundles
        auto r = executar ({ "/usr/bin/ditto", "-x", "-k", arq.getFullPathName(), destino.getFullPathName() });
        return r.ok() ? juce::Result::ok() : juce::Result::fail (U ("Non se puido abrir o .zip: ") + r.saida.trim());
    }
    juce::ZipFile z (arq);
    if (z.getNumEntries() == 0) return juce::Result::fail (U ("Non se puido abrir o .zip (baleiro ou estragado)"));
    return z.uncompressTo (destino, true);
}

std::vector<Atopado> Instalador::bundles (const juce::File& dir, SO so, const Produto& p)
{
    std::vector<Atopado> r;
    std::function<void (const juce::File&, int)> mirar = [&] (const juce::File& d, int nivel)
    {
        if (nivel > 3) return;
        auto lista = d.findChildFiles (juce::File::findFilesAndDirectories, false);
        lista.sort();
        for (auto& u : lista)
        {
            if (u.getFileName() == "__MACOSX" || u.getFileName().startsWith (".")) continue;
            if (auto f = formatoDoFicheiro (so, u))
                r.push_back ({ u.getFileNameWithoutExtension(), *f, u });
            else if (so == SO::lin && u.existsAsFile() && p.plugins.size() == 1 && u.getFileName() == p.binario)
                r.push_back ({ p.plugins[0], Formato::app, u });   // a app de Linux: o BINARIO, sen extensión
            else if (u.isDirectory())
                mirar (u, nivel + 1);
        }
    };
    mirar (dir, 0);
    return r;
}

Informe Instalador::instalarArquivo (const juce::File& arq, const Produto& p, const std::vector<Formato>& formatos,
                                     std::optional<Version> esperada, bool forzar)
{
    const auto base = temporal.getChildFile (juce::Uuid().toString());
    struct Limpar { juce::File f; ~Limpar() { f.deleteRecursively(); } } limpar { base };
    const auto aberto = base.getChildFile ("aberto");
    auto r = extraer (arq, aberto, so);
    if (r.failed()) throw ErroInstalacion (r.getErrorMessage().toStdString());
    return instalarCartafol (aberto, p, formatos, esperada, forzar);
}

Informe Instalador::restaurar (const Produto& p, const Version& v, const std::vector<Formato>& formatos)
{
    if (copias == nullptr) throw ErroInstalacion ("non hai copias");
    const auto orixe = copias->cartafolDe (p, v);
    if (! orixe.isDirectory())
        throw ErroInstalacion ((U ("Non hai copia gardada da ") + v.texto() + U (" de ") + p.nome).toStdString());
    // cópiase ao temporal (a copia queda para outra vez) e instálase de alí, forzando: é unha versión máis vella
    const auto base = temporal.getChildFile (juce::Uuid().toString());
    struct Limpar { juce::File f; ~Limpar() { f.deleteRecursively(); } } limpar { base };
    for (auto& d : orixe.findChildFiles (juce::File::findDirectories, false))
        for (auto& b : d.findChildFiles (juce::File::findFilesAndDirectories, false))
        {
            auto r = Copias::copiar (b, base.getChildFile (d.getFileName()).getChildFile (b.getFileName()));
            if (r.failed()) throw ErroInstalacion (r.getErrorMessage().toStdString());
        }
    // a que se restaura é a vella: as copias do sistema non se tocan
    struct Volver { bool& b; bool antes; ~Volver() { b = antes; } } volver { quitarVellasDoSistema, quitarVellasDoSistema };
    quitarVellasDoSistema = false;
    return instalarCartafol (base, p, formatos, v, true);
}

Informe Instalador::instalarCartafol (const juce::File& aberto, const Produto& p, const std::vector<Formato>& formatos,
                                      std::optional<Version> esperada, bool forzar)
{
    Informe informe;
    std::vector<Atopado> propios;
    for (auto& a : bundles (aberto, so, p))
    {
        if (p.plugins.contains (a.plugin)) propios.push_back (a);
        else informe.engadir (Accion::Tipo::aviso, a.plugin, a.formato, U ("vén no arquivo pero non é de ") + p.nome + U (": non se instala"));
    }
    if (propios.empty()) throw ErroInstalacion ((U ("O arquivo non leva ningún plugin de ") + p.nome).toStdString());

    std::vector<ParaOLixo> vellasSistema;
    for (auto f : formatos)
    {
        std::vector<Atopado> destes;
        for (auto& a : propios) if (a.formato == f) destes.push_back (a);
        if (destes.empty())
        {
            informe.engadir (Accion::Tipo::aviso, p.nome, f, U ("esta versión non trae ") + etiqueta (f));
            continue;
        }
        for (auto& plugin : p.plugins)
        {
            bool vai = false;
            for (auto& a : destes) vai = vai || a.plugin == plugin;
            if (! vai) informe.engadir (Accion::Tipo::aviso, plugin, f, U ("non vén no arquivo"));
        }
        for (auto& a : destes) instalarBundle (a, p, esperada, forzar, informe, vellasSistema);
    }
    // as copias vellas dos cartafoles do sistema (as que tapaban esta): ao Lixo, co permiso de admin
    if (! vellasSistema.empty())
        botarVarios (vellasSistema, informe, U ("Barullo Xestor quere levar ao Lixo a copia vella de ") + p.nome
                                                 + U (" que está no cartafol do sistema."));
    return informe;
}

void Instalador::instalarBundle (const Atopado& b, const Produto& p, std::optional<Version> esperada, bool forzar,
                                 Informe& informe, std::vector<ParaOLixo>& vellasSistema)
{
    const auto f = b.formato;
    const auto binario = p.plugins.size() == 1 ? p.binario : juce::String();
    // a versión do novo: a que di o bundle ou, se non a di (CLAP, apps de Windows e Linux), a publicada
    juce::String textoNova = b.ficheiro.isDirectory() ? infoBundle::versionPlist (b.ficheiro) : juce::String();
    if (textoNova.isEmpty() && b.ficheiro.isDirectory()) textoNova = infoBundle::versionModuleInfo (b.ficheiro);
    auto nova = Version::ler (textoNova);
    if (esperada && nova && *nova != *esperada)
        informe.engadir (Accion::Tipo::aviso, b.plugin, f, "o bundle di " + nova->texto() + U (" e a versión publicada é ") + esperada->texto());
    if (! nova) nova = esperada;

    const Escaner esc (rutas, rexistro);
    const auto destino = rutas.usuario.ruta (b.plugin, f, binario);
    const auto actual = esc.ler (b.plugin, f, rutas.usuario, binario);
    const bool existe = actual.has_value();

    // a comprobación de versión: nunca se pisa nada sen mirar
    if (existe && ! forzar)
    {
        if (actual->version && nova)
        {
            if (*actual->version > *nova)
            {
                informe.engadir (Accion::Tipo::omitido, b.plugin, f, U ("xa hai unha máis nova (") + actual->version->texto() + U ("); non se baixa á ") + nova->texto());
                return;
            }
            if (*actual->version == *nova)
            {
                informe.engadir (Accion::Tipo::omitido, b.plugin, f, U ("xa está na ") + nova->texto());
                return;
            }
        }
        else if (! nova)
        {
            informe.engadir (Accion::Tipo::omitido, b.plugin, f, U ("non se sabe a versión do novo; non se substitúe sen --forzar"));
            return;
        }
    }

    // limpar a cuarentena e comprobar a firma ANTES de tocar o instalado
    if (so == SO::mac && soActual() == SO::mac)
    {
        quitarCuarentena (b.ficheiro);
        juce::String aviso;
        auto r = asegurarFirma (b.ficheiro, aviso);
        if (r.failed()) { informe.engadir (Accion::Tipo::erro, b.plugin, f, r.getErrorMessage()); return; }
        if (aviso.isNotEmpty()) informe.engadir (Accion::Tipo::aviso, b.plugin, f, aviso);
    }
    if (f == Formato::app && b.ficheiro.existsAsFile()) b.ficheiro.setExecutePermission (true);

    // 1) o novo, ao lado do destino cun nome provisional (pode vir doutro disco: copia)
    destino.getParentDirectory().createDirectory();
    const auto provisional = destino.getParentDirectory().getChildFile ("." + destino.getFileName() + ".novo-" + juce::String::toHexString (juce::Random::getSystemRandom().nextInt()));
    auto r = mover (b.ficheiro, provisional);
    if (r.failed()) { informe.engadir (Accion::Tipo::erro, b.plugin, f, r.getErrorMessage()); return; }

    // 2) o vello, ao Lixo (se precisa permiso, pídese); antes, unha copia para poder volver a el
    if (existe)
    {
        if (copias != nullptr && actual->version && (! nova || *actual->version != *nova))
        {
            auto rc = copias->gardar (p, f, destino, *actual->version);
            if (rc.failed()) informe.engadir (Accion::Tipo::aviso, b.plugin, f, U ("non se puido gardar a copia para volver atrás: ") + rc.getErrorMessage());
        }
        Informe sub;
        botarVarios ({ { destino, b.plugin, f, false } }, sub, U ("Barullo Xestor quere substituír ") + destino.getFileName() + ".");
        informe.conAdmin += sub.conAdmin;
        informe.cancelouAdmin = informe.cancelouAdmin || sub.cancelouAdmin;
        if (destino.exists())
        {
            for (auto& a : sub.accions) if (a.tipo != Accion::Tipo::aoLixo) informe.accions.push_back (a);
            provisional.isDirectory() ? provisional.deleteRecursively() : provisional.deleteFile();   // é a copia temporal do novo
            informe.engadir (Accion::Tipo::erro, b.plugin, f, U ("non se puido quitar a versión instalada: queda como estaba"));
            return;
        }
    }

    // 3) o novo, ao seu sitio (o mesmo cartafol: só un cambio de nome)
    r = mover (provisional, destino);
    if (r.failed())
    {
        informe.engadir (Accion::Tipo::erro, b.plugin, f, r.getErrorMessage() + U (" (o novo quedou en ") + provisional.getFullPathName() + ")");
        return;
    }
    if (so == SO::mac && soActual() == SO::mac) quitarCuarentena (destino);
    const auto vn = nova ? nova->texto() : juce::String ("?");
    if (rexistro != nullptr && nova) rexistro->anotar (destino, vn);
    if (existe)
        informe.engadir (Accion::Tipo::actualizado, b.plugin, f, texto (actual->version) + U (" → ") + vn + U (" (o vello, no Lixo)"));
    else
        informe.engadir (Accion::Tipo::instalado, b.plugin, f, vn + " en " + destino.getParentDirectory().getFullPathName());

    // unha copia nun cartafol do sistema pode tapar esta
    for (auto& s : rutas.sistema)
    {
        if (s.cartafoles.count (f) == 0) continue;
        auto c = esc.ler (b.plugin, f, s, binario);
        if (! c) continue;
        if (quitarVellasDoSistema && (! c->version || ! nova || *c->version < *nova))
            vellasSistema.push_back ({ c->ficheiro, b.plugin, f, true });
        else
            informe.engadir (Accion::Tipo::aviso, b.plugin, f, U ("tamén hai unha copia (") + texto (c->version) + ") en " + c->ficheiro.getParentDirectory().getFullPathName());
    }
}

// ============================================================== ao Lixo (con permiso se fai falla)
void Instalador::botarVarios (const std::vector<ParaOLixo>& lista, Informe& informe, const juce::String& motivo)
{
    std::vector<ParaOLixo> conPermiso;
    for (auto& x : lista)
    {
        if (! x.ficheiro.exists()) continue;
        const Ubicacion* u = nullptr;
        for (auto* c : rutas.todas())
            if (x.formato && x.ficheiro.isAChildOf (c->cartafol (*x.formato))) u = c;
        if (x.sistema && simularAdmin) { conPermiso.push_back (x); continue; }
        juce::File onde;
        auto r = papeleira.botar (x.ficheiro, &onde);
        if (r.wasOk())
            informe.engadir (Accion::Tipo::aoLixo, x.plugin, x.formato, x.ficheiro.getFullPathName() + (onde != juce::File() ? U (" → ") + onde.getFullPathName() : juce::String()));
        else if (x.ficheiro.exists() && (x.sistema || (u != nullptr && u->eSistema()) || ! x.ficheiro.getParentDirectory().hasWriteAccess()
                                         || ! x.ficheiro.hasWriteAccess()))
            conPermiso.push_back (x);   // sen permisos: pídese o de administrador
        else
            informe.engadir (Accion::Tipo::erro, x.plugin, x.formato, r.getErrorMessage());
    }
    if (conPermiso.empty()) return;

    if (elevador == nullptr)
    {
        for (auto& x : conPermiso)
            informe.engadir (Accion::Tipo::omitido, x.plugin, x.formato, x.ficheiro.getFullPathName() + " precisa permiso de administrador");
        return;
    }
    // un cartafol temporal do usuario onde o paso con permiso deixa o que saca (e llo devolve ao usuario)
    const auto saida = temporal.getChildFile ("saida-" + juce::Uuid().toString());
    saida.createDirectory();
    std::vector<MovementoAdmin> ms;
    for (auto& x : conPermiso) ms.push_back ({ x.ficheiro, nomeLibre (saida, x.ficheiro.getFileName()) });
    auto r = elevador->mover (ms, motivo);
    if (r.failed())
    {
        if (foiCancelado (r)) informe.cancelouAdmin = true;
        for (auto& x : conPermiso)
            informe.engadir (foiCancelado (r) ? Accion::Tipo::omitido : Accion::Tipo::erro, x.plugin, x.formato,
                             (foiCancelado (r) ? U ("sen permiso de administrador: segue en ") : r.getErrorMessage() + U (": segue en "))
                                 + x.ficheiro.getFullPathName());
        saida.deleteRecursively();   // está baleiro
        return;
    }
    for (size_t i = 0; i < ms.size(); ++i)
    {
        const auto& x = conPermiso[i];
        if (! ms[i].a.exists() || x.ficheiro.exists())
        {
            informe.engadir (Accion::Tipo::erro, x.plugin, x.formato, U ("o paso con permiso non o moveu: segue en ") + x.ficheiro.getFullPathName());
            continue;
        }
        ++informe.conAdmin;
        juce::File onde;
        auto rb = papeleira.botar (ms[i].a, &onde);
        if (rb.wasOk())
            informe.engadir (Accion::Tipo::aoLixo, x.plugin, x.formato, x.ficheiro.getFullPathName() + U (" → ")
                             + (onde != juce::File() ? onde.getFullPathName() : juce::String ("Lixo")) + U (" (con permiso de administrador)"));
        else
            informe.engadir (Accion::Tipo::aviso, x.plugin, x.formato, U ("saíu do cartafol do sistema pero non foi ao Lixo: está en ") + ms[i].a.getFullPathName());
    }
    if (saida.getNumberOfChildFiles (juce::File::findFilesAndDirectories) == 0) saida.deleteRecursively();
}

Informe Instalador::desinstalar (const Produto& p, const std::vector<Formato>& formatos, bool incluirSistema)
{
    Informe informe;
    std::vector<ParaOLixo> lista;
    const Escaner esc (rutas, rexistro);
    for (auto& i : esc.instalados (p))
    {
        if (std::find (formatos.begin(), formatos.end(), i.formato) == formatos.end()) continue;
        if (i.ubicacion.eSistema() && ! incluirSistema)
        {
            informe.engadir (Accion::Tipo::omitido, i.plugin, i.formato, "a copia de " + i.ficheiro.getParentDirectory().getFullPathName() + U (" queda (--incluir-sistema)"));
            continue;
        }
        lista.push_back ({ i.ficheiro, i.plugin, i.formato, i.ubicacion.eSistema() });
    }
    botarVarios (lista, informe, U ("Barullo Xestor quere levar ao Lixo ") + p.nome + U (", que está no cartafol do sistema."));
    if (informe.accions.empty()) informe.engadir (Accion::Tipo::omitido, p.nome, std::nullopt, U ("non había nada instalado nestes formatos"));
    return informe;
}

Informe Instalador::botar (const std::vector<Instalado>& copiasDe)
{
    Informe informe;
    std::vector<ParaOLixo> lista;
    juce::StringArray nomes;
    for (auto& c : copiasDe)
    {
        lista.push_back ({ c.ficheiro, c.plugin, c.formato, c.ubicacion.eSistema() });
        nomes.addIfNotAlreadyThere (c.ficheiro.getFileName());
    }
    botarVarios (lista, informe, U ("Barullo Xestor quere levar ao Lixo a copia vella de ") + nomes.joinIntoString (", ")
                                     + U (" que está no cartafol do sistema."));
    return informe;
}

// ============================================================== cuarentena e firma (Mac)
void Instalador::quitarCuarentena (const juce::File& u)
{
   #if JUCE_MAC
    removexattr (u.getFullPathName().toRawUTF8(), "com.apple.quarantine", XATTR_NOFOLLOW);
    if (u.isDirectory())
        for (const auto& e : juce::RangedDirectoryIterator (u, true, "*", juce::File::findFilesAndDirectories))
            removexattr (e.getFile().getFullPathName().toRawUTF8(), "com.apple.quarantine", XATTR_NOFOLLOW);
   #else
    juce::ignoreUnused (u);
   #endif
}

bool Instalador::temCuarentena (const juce::File& u)
{
   #if JUCE_MAC
    return getxattr (u.getFullPathName().toRawUTF8(), "com.apple.quarantine", nullptr, 0, 0, XATTR_NOFOLLOW) >= 0;
   #else
    juce::ignoreUnused (u);
    return false;
   #endif
}

bool Instalador::firmaValida (const juce::File& u)
{
    return executar ({ "/usr/bin/codesign", "--verify", "--deep", "--strict", u.getFullPathName() }).ok();
}

juce::Result Instalador::asegurarFirma (const juce::File& u, juce::String& aviso)
{
    // se a firma non vale (o Live rexeita o VST3: «a sealed resource is missing or invalid»), asínase «ad hoc»
    if (firmaValida (u)) return juce::Result::ok();
    auto r = executar ({ "/usr/bin/codesign", "--force", "--deep", "-s", "-", u.getFullPathName() });
    if (! r.ok() || ! firmaValida (u))
        return juce::Result::fail (U ("a firma non vale e non se puido asinar de novo: ") + r.saida.trim());
    aviso = U ("a firma non valía: asinado de novo «ad hoc»");
    return juce::Result::ok();
}
} // namespace xestor

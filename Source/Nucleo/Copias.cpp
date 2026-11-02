#include "Copias.h"
#include "Descubrimento.h"
#include "Proceso.h"
#include "Papeleira.h"

#if ! JUCE_WINDOWS
 #include <unistd.h>
#endif

namespace xestor
{
juce::File Copias::estandar() { return cartafolDatos().getChildFile ("copias"); }

juce::File Copias::cartafolDe (const Produto& p, const Version& v) const
{
    return cartafol.getChildFile (juce::File::createLegalFileName (p.curto)).getChildFile (v.texto());
}

juce::Result Copias::copiar (const juce::File& de, const juce::File& a)
{
    if (! de.exists()) return juce::Result::fail ("Non existe " + de.getFullPathName());
    a.getParentDirectory().createDirectory();
   #if JUCE_MAC
    if (de.isDirectory())
    {
        auto r = executar ({ "/usr/bin/ditto", de.getFullPathName(), a.getFullPathName() });
        return r.ok() ? juce::Result::ok() : juce::Result::fail ("Non se puido copiar " + de.getFullPathName() + ": " + r.saida.trim());
    }
   #endif
    const bool ok = de.isDirectory() ? de.copyDirectoryTo (a) : de.copyFileTo (a);
   #if ! JUCE_WINDOWS
    // un executable (a app de Linux) segue séndoo
    if (ok && de.existsAsFile() && access (de.getFullPathName().toRawUTF8(), X_OK) == 0) a.setExecutePermission (true);
   #endif
    return ok ? juce::Result::ok() : juce::Result::fail ("Non se puido copiar " + de.getFullPathName());
}

juce::Result Copias::gardar (const Produto& p, Formato f, const juce::File& bundle, const Version& v)
{
    const auto dir = cartafolDe (p, v).getChildFile (clave (f));
    const auto destino = dir.getChildFile (bundle.getFileName());
    // se xa hai unha copia desta versión, substitúese (é a mesma)
    if (destino.isDirectory()) destino.deleteRecursively();
    else destino.deleteFile();
    auto r = copiar (bundle, destino);
    if (r.wasOk()) podar (p);
    return r;
}

std::vector<Version> Copias::versions (const Produto& p) const
{
    std::vector<Version> r;
    const auto dir = cartafol.getChildFile (juce::File::createLegalFileName (p.curto));
    for (auto& d : dir.findChildFiles (juce::File::findDirectories, false))
        if (auto v = Version::ler (d.getFileName()))
            if (d.getNumberOfChildFiles (juce::File::findFilesAndDirectories) > 0) r.push_back (*v);
    std::sort (r.begin(), r.end(), [] (const Version& a, const Version& b) { return a > b; });
    return r;
}

void Copias::podar (const Produto& p) const
{
    const auto v = versions (p);
    for (size_t i = (size_t) juce::jmax (1, maximo); i < v.size(); ++i)
        cartafolDe (p, v[i]).deleteRecursively();
}

// ============================================================== presets
namespace presets
{
juce::File documentos() { return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory); }

juce::Array<juce::File> cartafoles (const Produto& p, const juce::File& docs)
{
    juce::Array<juce::File> r;
    juce::StringArray nomes (p.plugins);
    nomes.addIfNotAlreadyThere (p.nome);
    for (auto& n : nomes)
    {
        const auto c = docs.getChildFile (n).getChildFile ("Presets");
        if (c.isDirectory()) r.addIfNotAlreadyThere (c);
    }
    return r;
}

juce::File cartafolCopias (const juce::File& docs) { return docs.getChildFile ("Barullo Xestor").getChildFile ("Copias dos presets"); }

juce::Result copiaDeSeguridade (const Produto& p, const juce::File& destino, juce::File& zip, const juce::File& docs, juce::Time cando)
{
    const auto cs = cartafoles (p, docs);
    juce::ZipFile::Builder zb;
    int n = 0;
    for (auto& c : cs)
        for (const auto& e : juce::RangedDirectoryIterator (c, true, "*", juce::File::findFiles))
        {
            if (e.getFile().getFileName().startsWith (".")) continue;
            // dentro do .zip: «Barullo FM/Presets/Cálido.bfm»
            zb.addFile (e.getFile(), 6, e.getFile().getRelativePathFrom (docs).replaceCharacter ('\\', '/'));
            ++n;
        }
    if (n == 0) return juce::Result::fail (U ("Non hai presets de ") + p.nome);
    destino.createDirectory();
    zip = nomeLibre (destino, juce::File::createLegalFileName (p.nome + " presets " + cando.formatted ("%Y-%m-%d %H.%M")) + ".zip");
    {
        juce::FileOutputStream out (zip);
        if (! out.openedOk() || ! zb.writeToStream (out, nullptr))
            return juce::Result::fail ("Non se puido escribir " + zip.getFullPathName());
    }
    return juce::Result::ok();
}
} // namespace presets
} // namespace xestor

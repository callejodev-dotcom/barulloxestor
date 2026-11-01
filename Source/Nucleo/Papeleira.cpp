#include "Papeleira.h"

namespace xestor
{
juce::Result mover (const juce::File& de, const juce::File& a)
{
    if (! de.exists()) return juce::Result::fail ("Non existe " + de.getFullPathName());
    if (a.exists()) return juce::Result::fail ("Xa existe " + a.getFullPathName());
    a.getParentDirectory().createDirectory();
    if (de.moveFileTo (a)) return juce::Result::ok();
    // outro disco: copiar e logo quitar o orixinal (só se a copia saíu enteira)
    const bool copiado = de.isDirectory() ? de.copyDirectoryTo (a) : de.copyFileTo (a);
    if (! copiado)
    {
        if (a.isDirectory()) a.deleteRecursively(); else a.deleteFile();
        return juce::Result::fail ("Non se puido mover " + de.getFullPathName() + " (permisos?)");
    }
    if (! (de.isDirectory() ? de.deleteRecursively() : de.deleteFile()))
        return juce::Result::fail ("Copiado, pero non se puido quitar " + de.getFullPathName() + " (permisos?)");
    return juce::Result::ok();
}

juce::File nomeLibre (const juce::File& cartafol, const juce::String& nome)
{
    auto f = cartafol.getChildFile (nome);
    const auto base = juce::File (nome).getFileNameWithoutExtension(), ext = juce::File (nome).getFileExtension();
    for (int n = 2; f.exists(); ++n) f = cartafol.getChildFile (base + " " + juce::String (n) + ext);
    return f;
}

juce::Result CartafolLixo::botar (const juce::File& f, juce::File* onde) const
{
    cartafol.createDirectory();
    const auto d = nomeLibre (cartafol, f.getFileName());
    auto r = mover (f, d);
    if (r.wasOk() && onde != nullptr) *onde = d;
    return r;
}

juce::Result LixoDoSistema::botar (const juce::File& f, juce::File* onde) const
{
    if (! f.exists()) return juce::Result::fail ("Non existe " + f.getFullPathName());
   #if JUCE_MAC
    return botarNoLixoMac (f, onde);
   #elif JUCE_WINDOWS
    // a Papeleira de reciclaxe (SHFileOperation con FOF_ALLOWUNDO)
    if (f.moveToTrash() && ! f.exists()) { if (onde != nullptr) *onde = juce::File(); return juce::Result::ok(); }
    return juce::Result::fail ("Non se puido levar " + f.getFullPathName() + juce::String::fromUTF8 (" á Papeleira de reciclaxe"));
   #else
    // freedesktop.org: $XDG_DATA_HOME/Trash/files/<nome> e info/<nome>.trashinfo (para «Restaurar»)
    auto xdg = juce::SystemStats::getEnvironmentVariable ("XDG_DATA_HOME", {});
    const auto lixo = (xdg.isNotEmpty() ? juce::File (xdg) : juce::File ("~/.local/share")).getChildFile ("Trash");
    const auto files = lixo.getChildFile ("files"), info = lixo.getChildFile ("info");
    files.createDirectory();
    info.createDirectory();
    const auto d = nomeLibre (files, f.getFileName());
    juce::String ti;
    ti << "[Trash Info]\nPath=" << juce::URL::addEscapeChars (f.getFullPathName(), false).replace ("%2F", "/")
       << "\nDeletionDate=" << juce::Time::getCurrentTime().formatted ("%Y-%m-%dT%H:%M:%S") << "\n";
    const auto fi = info.getChildFile (d.getFileName() + ".trashinfo");
    if (! fi.replaceWithText (ti)) return juce::Result::fail ("Non se pode escribir no Lixo (" + info.getFullPathName() + ")");
    auto r = mover (f, d);
    if (r.failed()) { fi.deleteFile(); return r; }
    if (onde != nullptr) *onde = d;
    return r;
   #endif
}
} // namespace xestor

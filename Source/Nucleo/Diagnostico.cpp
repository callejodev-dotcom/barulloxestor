#include "Diagnostico.h"

namespace xestor::diagnostico
{
juce::String curta (const juce::File& f, const juce::File& casa)
{
    const auto r = f.getFullPathName(), c = casa.getFullPathName();
    if (c.isNotEmpty() && (r == c || r.startsWith (c + juce::File::getSeparatorString()))) return "~" + r.substring (c.length());
    return r;
}

juce::String informe (const DatosInforme& d)
{
    juce::StringArray l;
    l.add ("Barullo Xestor " + d.versionXestor + U (" · ") + d.so + (d.idioma.isNotEmpty() ? U (" · idioma ") + d.idioma : juce::String()));
    l.add (U ("Data: ") + juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H:%M"));
    juce::String gh = d.senRede ? U ("sen conexión (o gardado)") : juce::String ("con conexión");
    if (d.dataCatalogo != juce::Time()) gh << U (", catálogo das ") << d.dataCatalogo.formatted ("%H:%M");
    if (d.restantesAPI >= 0) gh << ", quedan " << d.restantesAPI << " preguntas";
    l.add ("GitHub: " + gh);
    l.add (U ("DAW abertos: ") + (d.daws.isEmpty() ? juce::String ("ningún") : d.daws.joinIntoString (", ")));

    l.add ({});
    l.add ("CARTAFOLES");
    for (auto* u : d.rutas.todas())
    {
        juce::StringArray cs;
        for (auto& [f, c] : u->cartafoles)
            cs.add (etiqueta (f) + " " + curta (c) + (c.isDirectory() ? juce::String() : U (" (non existe)")));
        l.add ("  " + u->etiqueta() + ": " + cs.joinIntoString (U (" · ")));
    }

    int duplicados = 0;
    l.add ({});
    l.add ("PLUGINS");
    for (auto& s : d.situacions)
    {
        const auto& p = s.produto;
        juce::String cab = "  " + p.nome + U (" — ") + etiqueta (s.estado());
        if (p.ultima && p.ultima->version()) cab << U (" · publicada ") << p.ultima->version()->texto();
        if (p.versionLocal) cab << " · local " << p.versionLocal->texto();
        if (p.repo.isNotEmpty()) cab << " · " << p.repo;
        l.add (cab);
        const auto dup = s.duplicados();
        for (auto& i : s.instalados)
        {
            const bool eDup = std::any_of (dup.begin(), dup.end(), [&] (const Instalado& x) { return x.ficheiro == i.ficheiro; });
            juce::String t = "      " + etiqueta (i.formato) + " " + i.ubicacion.etiqueta() + " "
                             + (i.textoVersion.isNotEmpty() ? i.textoVersion : juce::String ("?")) + "  " + curta (i.ficheiro);
            if (d.firma)
                if (auto f = d.firma (i.ficheiro); f.isNotEmpty()) t << "  [firma: " << f << "]";
            if (eDup) { t << "  [DUPLICADA]"; ++duplicados; }
            if (i.precisaAdmin()) t << "  [precisa admin]";
            l.add (t);
        }
        if (! s.faltan().isEmpty()) l.add ("      faltan: " + s.faltan().joinIntoString (", "));
        if (d.copias != nullptr)
        {
            juce::StringArray vs;
            for (auto& v : d.copias->versions (p)) vs.add (v.texto());
            if (! vs.isEmpty()) l.add (U ("      copias para volver atrás: ") + vs.joinIntoString (", "));
        }
    }
    if (! d.orfos.empty())
    {
        l.add ({});
        l.add (U ("OUTROS DE ESCOITA (sen repo coñecido)"));
        for (auto& i : d.orfos)
            l.add ("  " + i.plugin + " " + etiqueta (i.formato) + " " + (i.textoVersion.isNotEmpty() ? i.textoVersion : juce::String ("?")) + "  " + curta (i.ficheiro));
    }
    l.add ({});
    l.add ("Copias duplicadas (usuario e sistema): " + juce::String (duplicados));
    if (! d.avisos.isEmpty())
    {
        l.add ("AVISOS");
        for (auto& a : d.avisos) l.add ("  " + a);
    }
    return l.joinIntoString ("\n") + "\n";
}
} // namespace xestor::diagnostico

import Foundation

/// Un bundle instalado no disco.
public struct Instalado: Hashable, Sendable {
    public var plugin: String
    public var formato: Formato
    public var ubicacion: Ubicacion
    public var url: URL
    public var version: Version?
    public var textoVersion: String?
    public var identificador: String?

    public var precisaAdmin: Bool { ubicacion.clase == .sistema && !ubicacion.escribible(formato) }
}

/// Le as versións dos bundles (Contents/Info.plist → CFBundleShortVersionString).
public enum InfoBundle {
    public static func info(_ bundle: URL) -> [String: Any]? {
        let p = bundle.appendingPathComponent("Contents/Info.plist")
        guard let d = try? Data(contentsOf: p),
              let o = try? PropertyListSerialization.propertyList(from: d, format: nil) as? [String: Any] else { return nil }
        return o
    }

    public static func version(_ bundle: URL) -> String? {
        let i = info(bundle)
        return (i?["CFBundleShortVersionString"] as? String) ?? (i?["CFBundleVersion"] as? String)
    }

    public static func identificador(_ bundle: URL) -> String? { info(bundle)?["CFBundleIdentifier"] as? String }
}

public struct Escaner: Sendable {
    public var rutas: Rutas
    /// Os bundles de Escoita levan este prefixo no identificador.
    public static let prefixoIdentificador = "link.escoita."

    public init(rutas: Rutas) { self.rutas = rutas }

    private func ler(plugin: String, formato: Formato, ubicacion: Ubicacion) -> Instalado? {
        let u = ubicacion.ruta(plugin: plugin, formato: formato)
        var dir: ObjCBool = false
        guard FileManager.default.fileExists(atPath: u.path, isDirectory: &dir), dir.boolValue else { return nil }
        let t = InfoBundle.version(u)
        return Instalado(plugin: plugin, formato: formato, ubicacion: ubicacion, url: u,
                         version: t.flatMap(Version.init), textoVersion: t, identificador: InfoBundle.identificador(u))
    }

    /// Todas as copias dos plugins dun produto (en todos os formatos e ubicacións).
    public func instalados(_ p: Produto) -> [Instalado] {
        var r: [Instalado] = []
        for plugin in p.plugins {
            for u in rutas.todas {
                for f in Formato.allCases {
                    if let i = ler(plugin: plugin, formato: f, ubicacion: u) { r.append(i) }
                }
            }
        }
        return r
    }

    /// Os bundles de Escoita que non son de ningún produto coñecido (p. ex. un plugin compilado na casa).
    public func orfos(coñecidos: [Produto]) -> [Instalado] {
        let nomes = Set(coñecidos.flatMap(\.plugins))
        var r: [Instalado] = []
        for u in rutas.todas {
            for f in Formato.allCases {
                let c = u.cartafol(f)
                guard let lista = try? FileManager.default.contentsOfDirectory(atPath: c.path) else { continue }
                for n in lista.sorted() where n.hasSuffix(".\(f.extension_)") {
                    let plugin = String(n.dropLast(f.extension_.count + 1))
                    guard !nomes.contains(plugin), let i = ler(plugin: plugin, formato: f, ubicacion: u),
                          i.identificador?.hasPrefix(Escaner.prefixoIdentificador) == true else { continue }
                    // unha app de Escoita en /Applications só conta se é un plugin (Barullo...)
                    if f == .app, !plugin.hasPrefix("Barullo") || plugin == "Barullo Xestor" { continue }
                    r.append(i)
                }
            }
        }
        return r
    }

    /// Un produto «solto» para un bundle de Escoita sen repo coñecido (para poder desinstalalo).
    public static func produtoSolto(_ nome: String, rutas: Rutas) -> Produto? {
        for i in Escaner(rutas: rutas).orfos(coñecidos: []) {
            let p = Produto(nome: i.plugin, curto: i.plugin.replacingOccurrences(of: " ", with: ""),
                            descricion: "", plugins: [i.plugin], tipo: .descoñecido)
            if p.casa(nome) { return p }
        }
        return nil
    }
}

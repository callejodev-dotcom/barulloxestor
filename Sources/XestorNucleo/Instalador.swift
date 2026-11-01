import Darwin
import Foundation

/// O que pasou con cada bundle nunha instalación ou desinstalación.
public struct Accion: Sendable, CustomStringConvertible {
    public enum Tipo: String, Sendable { case instalado, actualizado, omitido, aoLixo, aviso, erro }
    public var tipo: Tipo
    public var plugin: String
    public var formato: Formato?
    public var mensaxe: String

    public var description: String {
        let f = formato.map { " (\($0.etiqueta))" } ?? ""
        return "\(tipo.rawValue): \(plugin)\(f) — \(mensaxe)"
    }
}

public struct Informe: Sendable {
    public var accions: [Accion] = []
    public var ok: Bool { !accions.contains { $0.tipo == .erro } }
    public var cambios: Int { accions.filter { [.instalado, .actualizado, .aoLixo].contains($0.tipo) }.count }
    mutating func engadir(_ t: Accion.Tipo, _ plugin: String, _ f: Formato?, _ m: String) {
        accions.append(Accion(tipo: t, plugin: plugin, formato: f, mensaxe: m))
    }
}

public enum ErroInstalacion: Error, LocalizedError {
    case senVersion(String)
    case senZip(String)
    case extraer(String)
    case baleiro

    public var errorDescription: String? {
        switch self {
        case .senVersion(let n): return "\(n) aínda non ten ningunha versión publicada"
        case .senZip(let n): return "A versión publicada de \(n) non ten o .zip de Mac"
        case .extraer(let e): return "Non se puido abrir o .zip: \(e)"
        case .baleiro: return "O .zip non leva ningún plugin deste produto"
        }
    }
}

/// Instala, actualiza e desinstala. Sempre no cartafol do usuario (sen admin); o que se substitúe ou
/// se quita vai ao Lixo, nunca se borra.
public struct Instalador: Sendable {
    public var rutas: Rutas
    public var papeleira: Papeleira
    /// Onde se descargan e se abren os .zip (un cartafol temporal neutro, fóra de ~/Downloads).
    public var temporal: URL

    public init(rutas: Rutas, papeleira: Papeleira = LixoDoSistema(),
                temporal: URL = FileManager.default.temporaryDirectory.appendingPathComponent("BarulloXestor", isDirectory: true)) {
        self.rutas = rutas; self.papeleira = papeleira; self.temporal = temporal
    }

    // MARK: - instalar

    /// Descarga o .zip de Mac da última versión, compróbao e instala os formatos pedidos.
    public func instalar(_ p: Produto, formatos: [Formato], forzar: Bool = false,
                         progreso: @escaping @Sendable (Double) -> Void = { _ in }) async throws -> Informe {
        guard let ultima = p.ultima else { throw ErroInstalacion.senVersion(p.nome) }
        guard let zip = ultima.zipMac else { throw ErroInstalacion.senZip(p.nome) }
        let traballo = temporal.appendingPathComponent(UUID().uuidString, isDirectory: true)
        try FileManager.default.createDirectory(at: traballo, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: traballo) }
        let local = try await Descarga.baixar(zip.url, a: traballo.appendingPathComponent(zip.nome), progreso: progreso)
        try Descarga.verificar(local, zip)
        return try instalarZip(local, produto: p, formatos: formatos, versionEsperada: ultima.version, forzar: forzar,
                               traballo: traballo)
    }

    /// Instala desde un .zip xa descargado (o de «á man»: un cartafol cos bundles).
    public func instalarZip(_ zip: URL, produto p: Produto, formatos: [Formato], versionEsperada: Version?,
                            forzar: Bool = false, traballo: URL? = nil) throws -> Informe {
        let fm = FileManager.default
        let base = traballo ?? temporal.appendingPathComponent(UUID().uuidString, isDirectory: true)
        let aberto = base.appendingPathComponent("aberto", isDirectory: true)
        try fm.createDirectory(at: aberto, withIntermediateDirectories: true)
        defer { if traballo == nil { try? fm.removeItem(at: base) } }
        let r = try Proceso.executar("/usr/bin/ditto", ["-x", "-k", zip.path, aberto.path])
        guard r.codigo == 0 else { throw ErroInstalacion.extraer(r.erro.trimmingCharacters(in: .whitespacesAndNewlines)) }

        var informe = Informe()
        let atopados = Instalador.bundles(en: aberto)
        let propios = atopados.filter { p.plugins.contains($0.plugin) }
        for a in atopados where !p.plugins.contains(a.plugin) {
            informe.engadir(.aviso, a.plugin, a.formato, "vén no .zip pero non é de \(p.nome): non se instala")
        }
        guard !propios.isEmpty else { throw ErroInstalacion.baleiro }

        for f in formatos {
            let destes = propios.filter { $0.formato == f }
            if destes.isEmpty {
                informe.engadir(.aviso, p.nome, f, "esta versión non trae \(f.etiqueta)")
                continue
            }
            for plugin in p.plugins where !destes.contains(where: { $0.plugin == plugin }) {
                informe.engadir(.aviso, plugin, f, "non vén no .zip")
            }
            for b in destes {
                instalarBundle(b.url, plugin: b.plugin, formato: f, versionEsperada: versionEsperada,
                               forzar: forzar, informe: &informe)
            }
        }
        return informe
    }

    /// Os bundles (.vst3, .component, .clap, .app) dun cartafol, sen entrar neles.
    public static func bundles(en dir: URL) -> [(plugin: String, formato: Formato, url: URL)] {
        var r: [(String, Formato, URL)] = []
        func mirar(_ d: URL, _ nivel: Int) {
            guard nivel <= 3, let lista = try? FileManager.default.contentsOfDirectory(
                at: d, includingPropertiesForKeys: [.isDirectoryKey], options: [.skipsHiddenFiles]) else { return }
            for u in lista.sorted(by: { $0.lastPathComponent < $1.lastPathComponent }) {
                guard (try? u.resourceValues(forKeys: [.isDirectoryKey]).isDirectory) == true else { continue }
                if u.lastPathComponent == "__MACOSX" { continue }
                if let f = Formato(extension_: u.pathExtension) {
                    r.append((u.deletingPathExtension().lastPathComponent, f, u))
                } else {
                    mirar(u, nivel + 1)
                }
            }
        }
        mirar(dir, 0)
        return r
    }

    private func instalarBundle(_ novo: URL, plugin: String, formato f: Formato, versionEsperada: Version?,
                                forzar: Bool, informe: inout Informe) {
        let fm = FileManager.default
        let textoNova = InfoBundle.version(novo)
        let nova = textoNova.flatMap(Version.init)
        if let e = versionEsperada, let n = nova, n != e {
            informe.engadir(.aviso, plugin, f, "o bundle di \(n) e a versión publicada é \(e)")
        }
        let destino = rutas.usuario.ruta(plugin: plugin, formato: f)
        let existe = fm.fileExists(atPath: destino.path)
        let vella = existe ? InfoBundle.version(destino).flatMap(Version.init) : nil

        // a comprobación de versión: nunca se pisa nada sen mirar
        if existe, !forzar {
            if let v = vella, let n = nova {
                if v > n { informe.engadir(.omitido, plugin, f, "xa hai unha máis nova (\(v)); non se baixa á \(n)"); return }
                if v == n { informe.engadir(.omitido, plugin, f, "xa está na \(n)"); return }
            } else if nova == nil {
                informe.engadir(.omitido, plugin, f, "non se sabe a versión do novo; non se substitúe sen --forzar"); return
            }
        }

        // limpar a cuarentena e comprobar a firma ANTES de tocar o instalado
        Instalador.quitarCuarentena(novo)
        do {
            if let aviso = try Instalador.asegurarFirma(novo) { informe.engadir(.aviso, plugin, f, aviso) }
        } catch {
            informe.engadir(.erro, plugin, f, error.localizedDescription); return
        }

        do {
            try fm.createDirectory(at: destino.deletingLastPathComponent(), withIntermediateDirectories: true)
            var noLixo: URL?
            if existe { noLixo = try papeleira.botar(destino) }
            do {
                try fm.moveItem(at: novo, to: destino)
            } catch {
                // devolver o vello ao seu sitio
                if let l = noLixo { try? fm.moveItem(at: l, to: destino) }
                throw error
            }
            Instalador.quitarCuarentena(destino)
            let vn = nova.map { "\($0)" } ?? (textoNova ?? "?")
            if existe {
                informe.engadir(.actualizado, plugin, f, "\(vella.map { "\($0)" } ?? "?") → \(vn) (o vello, no Lixo)")
            } else {
                informe.engadir(.instalado, plugin, f, "\(vn) en \(destino.deletingLastPathComponent().path)")
            }
        } catch {
            informe.engadir(.erro, plugin, f, error.localizedDescription)
        }

        // unha copia no cartafol do sistema pode tapar esta
        if let s = rutas.sistema {
            let u = s.ruta(plugin: plugin, formato: f)
            if fm.fileExists(atPath: u.path) {
                let v = InfoBundle.version(u) ?? "?"
                let admin = s.escribible(f) ? "" : " (para quitala fai falla admin)"
                informe.engadir(.aviso, plugin, f, "tamén hai unha copia (\(v)) en \(u.deletingLastPathComponent().path)\(admin)")
            }
        }
    }

    // MARK: - cuarentena e firma

    /// Quita `com.apple.quarantine` do bundle e de todo o que leva dentro.
    public static func quitarCuarentena(_ u: URL) {
        removexattr(u.path, "com.apple.quarantine", XATTR_NOFOLLOW)
        guard let e = FileManager.default.enumerator(atPath: u.path) else { return }
        for case let r as String in e {
            removexattr(u.appendingPathComponent(r).path, "com.apple.quarantine", XATTR_NOFOLLOW)
        }
    }

    public static func temCuarentena(_ u: URL) -> Bool {
        getxattr(u.path, "com.apple.quarantine", nil, 0, 0, XATTR_NOFOLLOW) >= 0
    }

    public static func firmaValida(_ u: URL) -> Bool {
        (try? Proceso.executar("/usr/bin/codesign", ["--verify", "--deep", "--strict", u.path]).codigo) == 0
    }

    /// Se a firma non vale (o Live rexeita o VST3: «a sealed resource is missing or invalid»),
    /// asínase «ad hoc». Devolve un aviso se houbo que asinar; lanza se nin así vale.
    public static func asegurarFirma(_ u: URL) throws -> String? {
        if firmaValida(u) { return nil }
        let r = try Proceso.executar("/usr/bin/codesign", ["--force", "--deep", "-s", "-", u.path])
        guard r.codigo == 0, firmaValida(u) else {
            throw NSError(domain: "BarulloXestor", code: 1, userInfo: [NSLocalizedDescriptionKey:
                "a firma non vale e non se puido asinar de novo: \(r.erro.trimmingCharacters(in: .whitespacesAndNewlines))"])
        }
        return "a firma non valía: asinado de novo «ad hoc»"
    }

    // MARK: - desinstalar

    /// Leva ao Lixo as copias dos formatos pedidos. As do sistema só se `incluirSistema` e se se pode
    /// escribir alí sen admin; se non, avísase.
    public func desinstalar(_ p: Produto, formatos: [Formato], incluirSistema: Bool = false) -> Informe {
        var informe = Informe()
        let fm = FileManager.default
        for plugin in p.plugins {
            for f in formatos {
                for u in rutas.todas {
                    let b = u.ruta(plugin: plugin, formato: f)
                    guard fm.fileExists(atPath: b.path) else { continue }
                    if u.clase == .sistema {
                        if !incluirSistema { informe.engadir(.omitido, plugin, f, "a copia de \(b.deletingLastPathComponent().path) queda"); continue }
                        if !u.escribible(f) { informe.engadir(.omitido, plugin, f, "\(b.path) precisa admin: quítaa no Finder"); continue }
                    }
                    do {
                        let l = try papeleira.botar(b)
                        informe.engadir(.aoLixo, plugin, f, "\(b.path) → \(l.path)")
                    } catch {
                        informe.engadir(.erro, plugin, f, error.localizedDescription)
                    }
                }
            }
        }
        if informe.accions.isEmpty { informe.engadir(.omitido, p.nome, nil, "non había nada instalado nestes formatos") }
        return informe
    }

    /// Leva ao Lixo unhas copias concretas (p. ex. a vella de /Library cando a do usuario xa está ao día).
    public func botar(_ copias: [Instalado]) -> Informe {
        var informe = Informe()
        for c in copias {
            guard FileManager.default.fileExists(atPath: c.url.path) else { continue }
            if !c.ubicacion.escribible(c.formato) {
                informe.engadir(.omitido, c.plugin, c.formato, "\(c.url.path) precisa admin: quítaa no Finder"); continue
            }
            do {
                let l = try papeleira.botar(c.url)
                informe.engadir(.aoLixo, c.plugin, c.formato, "\(c.url.path) → \(l.path)")
            } catch {
                informe.engadir(.erro, c.plugin, c.formato, error.localizedDescription)
            }
        }
        return informe
    }
}

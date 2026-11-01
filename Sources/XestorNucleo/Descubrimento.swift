import Foundation

/// Os axustes (gárdanse en ~/Library/Application Support/Barullo Xestor/axustes.json; todos opcionais).
public struct Axustes: Codable, Sendable {
    /// A conta de GitHub onde están os plugins.
    public var propietario = "callejodev-dotcom"
    /// Repos doutras contas ou que se queiran engadir á man («propietario/nome»).
    public var reposExtra: [String] = []
    /// Repos que non se queren ver.
    public var reposOcultos: [String] = []
    /// Cartafoles onde buscar repositorios locais con `packaging/produto.env` (un nivel por debaixo).
    public var cartafolesLocais: [String] = ["~/Downloads"]
    /// Os formatos que se instalan e se miran (por defecto, só VST3).
    public var formatos: [Formato] = [.vst3]
    /// Usar `gh auth token` (se o gh ten sesión) para non toparse co límite de GitHub.
    public var usarGh = true

    public init() {}

    public static var ficheiro: URL {
        FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("Barullo Xestor/axustes.json")
    }

    public static func cargar(_ u: URL = ficheiro) -> Axustes {
        guard let d = try? Data(contentsOf: u) else { return Axustes() }
        // as chaves que falten quedan co valor por defecto
        var a = Axustes()
        if let o = try? JSONSerialization.jsonObject(with: d) as? [String: Any] {
            if let v = o["propietario"] as? String { a.propietario = v }
            if let v = o["reposExtra"] as? [String] { a.reposExtra = v }
            if let v = o["reposOcultos"] as? [String] { a.reposOcultos = v }
            if let v = o["cartafolesLocais"] as? [String] { a.cartafolesLocais = v }
            if let v = o["formatos"] as? [String] { let f = v.compactMap(Formato.init(rawValue:)); if !f.isEmpty { a.formatos = f } }
            if let v = o["usarGh"] as? Bool { a.usarGh = v }
        }
        return a
    }

    public func gardar(_ u: URL = ficheiro) throws {
        try FileManager.default.createDirectory(at: u.deletingLastPathComponent(), withIntermediateDirectories: true)
        let e = JSONEncoder(); e.outputFormatting = [.prettyPrinted, .sortedKeys]
        try e.encode(self).write(to: u, options: .atomic)
    }

    public var urlsLocais: [URL] {
        cartafolesLocais.map { URL(fileURLWithPath: ($0 as NSString).expandingTildeInPath, isDirectory: true) }
    }
}

/// O que se sabe dos plugins: os produtos (de GitHub e locais) e os avisos que houbo polo camiño.
public struct Catalogo: Sendable {
    public var produtos: [Produto]
    public var avisos: [String]
    public var data: Date

    public init(produtos: [Produto], avisos: [String], data: Date = Date()) {
        self.produtos = produtos; self.avisos = avisos; self.data = data
    }

    public func buscar(_ consulta: String) -> Produto? { produtos.first { $0.casa(consulta) } }
}

/// Descobre os plugins sen ter unha lista escrita no código: un repo é un plugin de Escoita se ten
/// `packaging/produto.env` (o ficheiro do núcleo). Así un plugin novo aparece só ao publicalo, e un
/// que se está a facer na casa (con ese ficheiro) aparece coma «só local».
public struct Descubridor: Sendable {
    public var github: GitHub?
    public var axustes: Axustes

    public init(github: GitHub?, axustes: Axustes) { self.github = github; self.axustes = axustes }

    public func descubrir(forzar: Bool = false) async -> Catalogo {
        var avisos: [String] = []
        var produtos: [Produto] = []

        if let gh = github {
            do {
                var repos = try await gh.repos(forzar: forzar).filter { !$0.privado && !$0.arquivado && !$0.bifurcacion }
                for extra in axustes.reposExtra where !repos.contains(where: { $0.nomeCompleto.lowercased() == extra.lowercased() }) {
                    if let r = try? await gh.repo(extra, forzar: forzar) { repos.append(r) }
                    else { avisos.append("Non se atopa o repo \(extra)") }
                }
                let ocultos = Set(axustes.reposOcultos.map { $0.lowercased() })
                repos = repos.filter { !ocultos.contains($0.nomeCompleto.lowercased()) && !ocultos.contains($0.nomeCompleto.split(separator: "/").last.map { String($0).lowercased() } ?? "") }

                produtos = await withTaskGroup(of: (Produto?, String?).self) { g in
                    for r in repos {
                        g.addTask {
                            do {
                                guard let env = try await gh.produtoEnv(r, forzar: forzar),
                                      var p = Produto(produtoEnv: env, repo: r.nomeCompleto, descricionRepo: r.descricion)
                                else { return (nil, nil) }
                                p.ultima = try await gh.ultimoLanzamento(r.nomeCompleto, forzar: forzar)
                                p.lanzamentoConsultado = true
                                return (p, nil)
                            } catch {
                                return (nil, "\(r.nomeCompleto): \(error.localizedDescription)")
                            }
                        }
                    }
                    var r: [Produto] = []
                    for await (p, aviso) in g {
                        if let p { r.append(p) }
                        if let aviso { avisos.append(aviso) }
                    }
                    return r
                }
                if gh.usouCacheVella { avisos.append("Sen conexión con GitHub: amósase o último que se soubo.") }
            } catch {
                avisos.append("Non se puido preguntar a GitHub: \(error.localizedDescription)")
            }
        }

        // os repositorios locais
        for local in Descubridor.locais(en: axustes.urlsLocais) {
            if let i = produtos.firstIndex(where: { $0.curto.lowercased() == local.curto.lowercased() }) {
                if produtos[i].rutaLocal == nil {
                    produtos[i].rutaLocal = local.rutaLocal
                    produtos[i].versionLocal = local.versionLocal
                }
            } else if !produtos.contains(where: { $0.nome == local.nome }) {
                produtos.append(local)
            }
        }
        produtos.sort { ($0.tipo.orde, $0.nome.lowercased()) < ($1.tipo.orde, $1.nome.lowercased()) }
        return Catalogo(produtos: produtos, avisos: avisos.sorted())
    }

    /// Os subcartafoles con `packaging/produto.env`.
    public static func locais(en cartafoles: [URL]) -> [Produto] {
        var r: [Produto] = []
        let fm = FileManager.default
        for c in cartafoles {
            guard let subs = try? fm.contentsOfDirectory(at: c, includingPropertiesForKeys: [.isDirectoryKey],
                                                          options: [.skipsHiddenFiles]) else { continue }
            for s in subs.sorted(by: { $0.path < $1.path }) {
                let env = s.appendingPathComponent("packaging/produto.env")
                guard let t = try? String(contentsOf: env, encoding: .utf8), var p = Produto(produtoEnv: t) else { continue }
                // o núcleo leva un produto.env de exemplo? non é un plugin
                if s.lastPathComponent == "escoita-nucleo" { continue }
                p.rutaLocal = s
                p.versionLocal = versionCMake(s.appendingPathComponent("CMakeLists.txt"))
                if !r.contains(where: { $0.curto == p.curto }) { r.append(p) }
            }
        }
        return r
    }

    /// A versión do `project(... VERSION x.y.z)` do CMakeLists.
    public static func versionCMake(_ u: URL) -> Version? {
        guard let t = try? String(contentsOf: u, encoding: .utf8) else { return nil }
        let re = try! NSRegularExpression(pattern: #"project\s*\([^)]*?VERSION\s+([0-9][0-9.]*)"#, options: [.caseInsensitive])
        guard let m = re.firstMatch(in: t, range: NSRange(t.startIndex..., in: t)),
              let rg = Range(m.range(at: 1), in: t) else { return nil }
        return Version(String(t[rg]))
    }
}

extension TipoProduto {
    var orde: Int {
        switch self {
        case .instrumento: return 0
        case .efecto: return 1
        case .paquete: return 2
        case .descoñecido: return 3
        }
    }
}

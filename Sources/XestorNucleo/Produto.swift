import Foundation

/// Os formatos que se poden instalar. O usuario escolle cales; por defecto, só VST3.
public enum Formato: String, CaseIterable, Codable, Hashable, Sendable {
    case vst3, au, clap, app

    public var extension_: String {
        switch self {
        case .vst3: return "vst3"
        case .au: return "component"
        case .clap: return "clap"
        case .app: return "app"
        }
    }

    /// Subcartafol dentro de «Plug-Ins» (a app vai a «Applications»).
    public var subcartafol: String? {
        switch self {
        case .vst3: return "VST3"
        case .au: return "Components"
        case .clap: return "CLAP"
        case .app: return nil
        }
    }

    public var etiqueta: String {
        switch self {
        case .vst3: return "VST3"
        case .au: return "AU"
        case .clap: return "CLAP"
        case .app: return "App"
        }
    }

    public init?(extension_ ext: String) {
        guard let f = Formato.allCases.first(where: { $0.extension_ == ext.lowercased() }) else { return nil }
        self = f
    }

    /// «vst3,clap» → [.vst3, .clap]
    public static func lista(_ texto: String) -> [Formato]? {
        var r: [Formato] = []
        for a in texto.lowercased().split(separator: ",") {
            let t = a.trimmingCharacters(in: .whitespaces)
            let f: Formato? = t == "component" || t == "components" ? .au : (Formato(rawValue: t) ?? (t == "standalone" ? .app : nil))
            guard let f else { return nil }
            if !r.contains(f) { r.append(f) }
        }
        return r.isEmpty ? nil : r
    }
}

public enum TipoProduto: String, Codable, Sendable {
    case instrumento, efecto, paquete, descoñecido

    public var titulo: String {
        switch self {
        case .instrumento: return "Instrumentos"
        case .efecto: return "Efectos"
        case .paquete: return "Paquetes"
        case .descoñecido: return "Outros"
        }
    }
}

/// Un ficheiro dunha versión publicada en GitHub.
public struct Activo: Codable, Hashable, Sendable {
    public var nome: String
    public var url: URL
    public var tamano: Int64
    public var sha256: String?

    public init(nome: String, url: URL, tamano: Int64, sha256: String?) {
        self.nome = nome; self.url = url; self.tamano = tamano; self.sha256 = sha256
    }
}

/// A última versión publicada dun produto.
public struct Lanzamento: Codable, Hashable, Sendable {
    public var tag: String
    public var nome: String
    public var paxina: URL?
    public var publicado: Date?
    public var activos: [Activo]

    public var version: Version? { Version(tag) }
    /// O .zip de Mac «á man» (bundles soltos): é o que instala o xestor.
    public var zipMac: Activo? { activos.first { $0.nome.lowercased().hasSuffix("-mac.zip") } }
    public var pkgMac: Activo? { activos.first { $0.nome.lowercased().hasSuffix("-mac.pkg") } }

    public init(tag: String, nome: String, paxina: URL?, publicado: Date?, activos: [Activo]) {
        self.tag = tag; self.nome = nome; self.paxina = paxina; self.publicado = publicado; self.activos = activos
    }
}

/// Un produto de Escoita: un plugin ou unha serie (varios plugins nun instalador, coma Enxebres).
public struct Produto: Identifiable, Hashable, Sendable {
    public var id: String { curto }
    public var nome: String          // «Barullo Tune» (o do paquete, nunha serie)
    public var curto: String         // «BarulloTune»: prefixo dos ficheiros publicados
    public var descricion: String
    public var plugins: [String]     // os nomes dos bundles, sen tiles
    public var tipo: TipoProduto
    public var temAU: Bool
    public var repo: String?         // «propietario/nome» en GitHub (nil: só local)
    public var rutaLocal: URL?       // o repositorio na máquina, se se atopou
    public var versionLocal: Version? // a do CMakeLists local
    public var ultima: Lanzamento?
    /// Preguntouse a GitHub pola última versión (se `ultima` é nil, é que non hai ningunha)
    public var lanzamentoConsultado = false

    public var eSerie: Bool { tipo == .paquete }
    public var paxinaRepo: URL? { repo.flatMap { URL(string: "https://github.com/\($0)") } }

    public init(nome: String, curto: String, descricion: String, plugins: [String], tipo: TipoProduto,
                temAU: Bool = true, repo: String? = nil, rutaLocal: URL? = nil, versionLocal: Version? = nil,
                ultima: Lanzamento? = nil) {
        self.nome = nome; self.curto = curto; self.descricion = descricion; self.plugins = plugins
        self.tipo = tipo; self.temAU = temAU; self.repo = repo; self.rutaLocal = rutaLocal
        self.versionLocal = versionLocal; self.ultima = ultima
    }

    /// Crea o produto a partir do `packaging/produto.env` (o formato do núcleo de Escoita).
    public init?(produtoEnv texto: String, repo: String? = nil, descricionRepo: String? = nil) {
        let v = ProdutoEnv.ler(texto)
        guard let nome = v["NOME"], !nome.isEmpty else { return nil }
        let curto = v["CURTO"].flatMap { $0.isEmpty ? nil : $0 } ?? nome.replacingOccurrences(of: " ", with: "")
        let au = v["AU"] ?? ""
        var plugins: [String] = []
        var tipo: TipoProduto
        var temAU = !au.isEmpty
        if let lista = v["PLUGINS"], !lista.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
            for linha in lista.split(whereSeparator: \.isNewline) {
                let campos = linha.split(separator: "|", omittingEmptySubsequences: false)
                    .map { $0.trimmingCharacters(in: .whitespaces) }
                if let n = campos.first, !n.isEmpty { plugins.append(n) }
                if campos.count > 2, !campos[2].isEmpty { temAU = true }
            }
            tipo = .paquete
        } else {
            plugins = [nome]
            if au.hasPrefix("aumu") || au.hasPrefix("aumi") { tipo = .instrumento }
            else if au.hasPrefix("aufx") || au.hasPrefix("aumf") { tipo = .efecto }
            else { tipo = .descoñecido }
        }
        let d = v["DESCRICION"].flatMap { $0.isEmpty ? nil : $0 } ?? descricionRepo ?? ""
        self.init(nome: nome, curto: curto, descricion: d, plugins: plugins, tipo: tipo, temAU: temAU, repo: repo)
    }

    /// Para buscar un produto polo nome que escriba alguén: «tune», «Barullo Tune», «barullotune», «enxebres»...
    public func casa(_ consulta: String) -> Bool {
        func n(_ s: String) -> String {
            s.folding(options: [.diacriticInsensitive, .caseInsensitive], locale: nil)
                .filter { $0.isLetter || $0.isNumber }
        }
        let q = n(consulta)
        guard !q.isEmpty else { return false }
        var nomes = [nome, curto, nome.replacingOccurrences(of: "Barullo ", with: "")]
        if let repo { nomes.append(String(repo.split(separator: "/").last ?? "")) }
        return nomes.contains { n($0) == q }
    }
}

/// Le un `produto.env` (sintaxe de shell sinxela: CHAVE="valor", que pode ocupar varias liñas, e # comentarios).
public enum ProdutoEnv {
    public static func ler(_ texto: String) -> [String: String] {
        var r: [String: String] = [:]
        let c = Array(texto)
        var i = 0
        func saltarLinha() { while i < c.count, c[i] != "\n" { i += 1 } }
        while i < c.count {
            // inicio de liña: espazos, comentario ou CHAVE=
            while i < c.count, c[i] == " " || c[i] == "\t" { i += 1 }
            if i >= c.count { break }
            if c[i] == "\n" { i += 1; continue }
            if c[i] == "#" { saltarLinha(); continue }
            var chave = ""
            while i < c.count, c[i].isLetter || c[i].isNumber || c[i] == "_" { chave.append(c[i]); i += 1 }
            if chave == "export" { continue }
            guard i < c.count, c[i] == "=", !chave.isEmpty else { saltarLinha(); continue }
            i += 1
            var valor = ""
            while i < c.count, c[i] != "\n" {
                let ch = c[i]
                if ch == "\"" || ch == "'" {
                    i += 1
                    while i < c.count, c[i] != ch {
                        if ch == "\"", c[i] == "\\", i + 1 < c.count { i += 1 }
                        valor.append(c[i]); i += 1
                    }
                    i += 1
                } else if ch == " " || ch == "\t" {
                    // fóra das comiñas, un espazo remata o valor (o resto é un comentario)
                    saltarLinha()
                } else {
                    valor.append(ch); i += 1
                }
            }
            r[chave] = valor
        }
        return r
    }
}

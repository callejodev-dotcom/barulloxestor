import Foundation

public struct RepoInfo: Sendable, Hashable {
    public var nomeCompleto: String   // «propietario/nome»
    public var descricion: String?
    public var rama: String
    public var privado: Bool
    public var arquivado: Bool
    public var bifurcacion: Bool
}

public enum ErroGitHub: Error, LocalizedError {
    case limite(reinicio: Date?)
    case http(Int, String)
    case formato(String)

    public var errorDescription: String? {
        switch self {
        case .limite(let d):
            let h = d.map { DateFormatter.localizedString(from: $0, dateStyle: .none, timeStyle: .short) } ?? "dentro dun pouco"
            return "GitHub di que xa se lle preguntou de máis; volve probar ás \(h)."
        case .http(let c, let u): return "GitHub respondeu \(c) a \(u)"
        case .formato(let u): return "Non se entende a resposta de \(u)"
        }
    }
}

/// Cliente pequeno da API de GitHub, con caché no disco.
/// Sen sesión, GitHub deixa 60 preguntas por hora: por iso garda as respostas (e o seu ETag, para
/// preguntar «cambiou algo?», que se o non cambiou non gasta) e non volve preguntar ata pasado `vixencia`.
public final class GitHub: @unchecked Sendable {
    public let propietario: String
    public let cache: URL
    public var token: String?
    public var vixencia: TimeInterval
    public var soCache = false           // sen rede: só o que estea gardado
    private let sesion: URLSession
    private let bloqueo = NSLock()
    private var _restantes: Int?
    private var _reinicio: Date?
    private var _usouCache = false

    public var restantes: Int? { bloqueo.lock(); defer { bloqueo.unlock() }; return _restantes }
    public var reinicio: Date? { bloqueo.lock(); defer { bloqueo.unlock() }; return _reinicio }
    /// Algunha resposta saíu da caché vella porque a rede fallou?
    public var usouCacheVella: Bool { bloqueo.lock(); defer { bloqueo.unlock() }; return _usouCache }

    public init(propietario: String, cache: URL, token: String? = nil, vixencia: TimeInterval = 15 * 60,
                sesion: URLSession = .shared) {
        self.propietario = propietario; self.cache = cache; self.token = token
        self.vixencia = vixencia; self.sesion = sesion
    }

    public static var cacheEstandar: URL {
        FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("link.escoita.barulloxestor", isDirectory: true)
    }

    // MARK: - token (opcional)

    /// GITHUB_TOKEN / GH_TOKEN, ou o de `gh auth token` se o gh está instalado e ten sesión.
    public static func tokenDispoñible() -> String? {
        let env = ProcessInfo.processInfo.environment
        for k in ["GITHUB_TOKEN", "GH_TOKEN"] { if let t = env[k], !t.isEmpty { return t } }
        for gh in ["/opt/homebrew/bin/gh", "/usr/local/bin/gh", "/usr/bin/gh"] where FileManager.default.isExecutableFile(atPath: gh) {
            if let r = try? Proceso.executar(gh, ["auth", "token"], tempoMaximo: 5), r.codigo == 0 {
                let t = r.saida.trimmingCharacters(in: .whitespacesAndNewlines)
                if !t.isEmpty { return t }
            }
        }
        return nil
    }

    // MARK: - caché

    private struct Gardado: Codable {
        var url: String
        var etag: String?
        var data: Date
        var estado: Int
        var corpo: Data
    }

    private func ficheiro(_ url: URL) -> URL {
        // un nome estable e curto para cada URL
        var h: UInt64 = 1469598103934665603
        for b in url.absoluteString.utf8 { h = (h ^ UInt64(b)) &* 1099511628211 }
        return cache.appendingPathComponent(String(format: "%016llx.json", h))
    }

    private func lerCache(_ url: URL) -> Gardado? {
        guard let d = try? Data(contentsOf: ficheiro(url)) else { return nil }
        let dec = JSONDecoder(); dec.dateDecodingStrategy = .iso8601
        return try? dec.decode(Gardado.self, from: d)
    }

    private func gardar(_ g: Gardado, _ url: URL) {
        try? FileManager.default.createDirectory(at: cache, withIntermediateDirectories: true)
        let enc = JSONEncoder(); enc.dateEncodingStrategy = .iso8601
        if let d = try? enc.encode(g) { try? d.write(to: ficheiro(url), options: .atomic) }
    }

    public func baleirarCache() { try? FileManager.default.removeItem(at: cache) }

    /// Pide un URL (con caché). Devolve o estado HTTP e o corpo; 404 tamén se garda (non hai versión...).
    public func pedir(_ url: URL, forzar: Bool = false) async throws -> (Int, Data) {
        let vello = lerCache(url)
        if let v = vello, soCache || (!forzar && Date().timeIntervalSince(v.data) < vixencia) { return (v.estado, v.corpo) }
        if soCache { throw ErroGitHub.http(0, url.absoluteString) }
        var req = URLRequest(url: url, cachePolicy: .reloadIgnoringLocalCacheData, timeoutInterval: 20)
        req.setValue("BarulloXestor", forHTTPHeaderField: "User-Agent")
        if url.host == "api.github.com" {
            req.setValue("application/vnd.github+json", forHTTPHeaderField: "Accept")
            req.setValue("2022-11-28", forHTTPHeaderField: "X-GitHub-Api-Version")
            if let token { req.setValue("Bearer \(token)", forHTTPHeaderField: "Authorization") }
        }
        if let e = vello?.etag { req.setValue(e, forHTTPHeaderField: "If-None-Match") }
        do {
            let (d, r) = try await sesion.data(for: req)
            guard let h = r as? HTTPURLResponse else { throw ErroGitHub.formato(url.absoluteString) }
            if url.host == "api.github.com" { anotarLimite(h) }
            switch h.statusCode {
            case 304:
                if var v = vello { v.data = Date(); gardar(v, url); return (v.estado, v.corpo) }
                throw ErroGitHub.http(304, url.absoluteString)
            case 200, 404:
                gardar(Gardado(url: url.absoluteString, etag: h.value(forHTTPHeaderField: "Etag"), data: Date(),
                               estado: h.statusCode, corpo: d), url)
                return (h.statusCode, d)
            case 403, 429:
                if let v = vello { marcarVella(); return (v.estado, v.corpo) }
                throw ErroGitHub.limite(reinicio: reinicio)
            default:
                if let v = vello { marcarVella(); return (v.estado, v.corpo) }
                throw ErroGitHub.http(h.statusCode, url.absoluteString)
            }
        } catch let e as ErroGitHub {
            throw e
        } catch {
            if let v = vello { marcarVella(); return (v.estado, v.corpo) }
            throw error
        }
    }

    private func anotarLimite(_ h: HTTPURLResponse) {
        bloqueo.lock(); defer { bloqueo.unlock() }
        if let s = h.value(forHTTPHeaderField: "x-ratelimit-remaining"), let n = Int(s) { _restantes = n }
        if let s = h.value(forHTTPHeaderField: "x-ratelimit-reset"), let n = Double(s) { _reinicio = Date(timeIntervalSince1970: n) }
    }

    private func marcarVella() { bloqueo.lock(); _usouCache = true; bloqueo.unlock() }

    private func json(_ url: URL, forzar: Bool) async throws -> Any? {
        let (e, d) = try await pedir(url, forzar: forzar)
        if e == 404 { return nil }
        return try JSONSerialization.jsonObject(with: d)
    }

    // MARK: - o que se lle pregunta

    /// Os repositorios públicos do propietario.
    public func repos(forzar: Bool = false) async throws -> [RepoInfo] {
        var r: [RepoInfo] = []
        var paxina = 1
        while true {
            let u = URL(string: "https://api.github.com/users/\(propietario)/repos?per_page=100&type=owner&page=\(paxina)")!
            guard let lista = try await json(u, forzar: forzar) as? [[String: Any]] else {
                throw ErroGitHub.formato(u.absoluteString)
            }
            for o in lista {
                guard let n = o["full_name"] as? String else { continue }
                r.append(RepoInfo(nomeCompleto: n, descricion: o["description"] as? String,
                                  rama: o["default_branch"] as? String ?? "main",
                                  privado: o["private"] as? Bool ?? false, arquivado: o["archived"] as? Bool ?? false,
                                  bifurcacion: o["fork"] as? Bool ?? false))
            }
            if lista.count < 100 { break }
            paxina += 1
        }
        return r
    }

    /// Un repositorio concreto (para os que se engaden á man).
    public func repo(_ nomeCompleto: String, forzar: Bool = false) async throws -> RepoInfo? {
        let u = URL(string: "https://api.github.com/repos/\(nomeCompleto)")!
        guard let o = try await json(u, forzar: forzar) as? [String: Any] else { return nil }
        return RepoInfo(nomeCompleto: o["full_name"] as? String ?? nomeCompleto, descricion: o["description"] as? String,
                        rama: o["default_branch"] as? String ?? "main", privado: o["private"] as? Bool ?? false,
                        arquivado: o["archived"] as? Bool ?? false, bifurcacion: o["fork"] as? Bool ?? false)
    }

    /// O `packaging/produto.env` dun repo (de raw.githubusercontent.com, que non gasta preguntas da API).
    /// nil: o repo non é un plugin de Escoita.
    public func produtoEnv(_ repo: RepoInfo, forzar: Bool = false) async throws -> String? {
        let u = URL(string: "https://raw.githubusercontent.com/\(repo.nomeCompleto)/\(repo.rama)/packaging/produto.env")!
        let (e, d) = try await pedir(u, forzar: forzar)
        guard e == 200 else { return nil }
        return String(data: d, encoding: .utf8)
    }

    /// A última versión publicada (nil se aínda non hai ningunha).
    public func ultimoLanzamento(_ nomeCompleto: String, forzar: Bool = false) async throws -> Lanzamento? {
        let u = URL(string: "https://api.github.com/repos/\(nomeCompleto)/releases/latest")!
        guard let o = try await json(u, forzar: forzar) as? [String: Any] else { return nil }
        return GitHub.lanzamento(o)
    }

    public static func lanzamento(_ o: [String: Any]) -> Lanzamento? {
        guard let tag = o["tag_name"] as? String else { return nil }
        let iso = ISO8601DateFormatter()
        let activos: [Activo] = (o["assets"] as? [[String: Any]] ?? []).compactMap { a in
            guard let n = a["name"] as? String, let s = a["browser_download_url"] as? String, let u = URL(string: s) else { return nil }
            var sha = a["digest"] as? String
            if let d = sha { sha = d.hasPrefix("sha256:") ? String(d.dropFirst(7)) : nil }
            return Activo(nome: n, url: u, tamano: (a["size"] as? NSNumber)?.int64Value ?? 0, sha256: sha)
        }
        return Lanzamento(tag: tag, nome: o["name"] as? String ?? tag,
                          paxina: (o["html_url"] as? String).flatMap(URL.init(string:)),
                          publicado: (o["published_at"] as? String).flatMap { iso.date(from: $0) }, activos: activos)
    }
}

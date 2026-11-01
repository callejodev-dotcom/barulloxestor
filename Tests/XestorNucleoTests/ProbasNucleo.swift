import Foundation
import Testing
@testable import XestorNucleo

// Todas as probas traballan nun cartafol temporal: nunca nos cartafoles de plugins de verdade.

/// Un cartafol temporal que se borra ao final.
final class Temporal {
    let url: URL
    init() {
        url = FileManager.default.temporaryDirectory
            .appendingPathComponent("probas-barulloxestor-\(UUID().uuidString)", isDirectory: true)
        try! FileManager.default.createDirectory(at: url, withIntermediateDirectories: true)
    }
    deinit { try? FileManager.default.removeItem(at: url) }
    func sub(_ s: String) -> URL { url.appendingPathComponent(s, isDirectory: true) }
}

/// Fai un bundle de proba (Info.plist coa versión e, se se pide, un executable asinado).
@discardableResult
func facerBundle(_ dir: URL, nome: String, ext: String, version: String, executable: Bool = true,
                 identificador: String? = nil) throws -> URL {
    let b = dir.appendingPathComponent("\(nome).\(ext)", isDirectory: true)
    let c = b.appendingPathComponent("Contents", isDirectory: true)
    try FileManager.default.createDirectory(at: c.appendingPathComponent("MacOS"), withIntermediateDirectories: true)
    var info: [String: Any] = [
        "CFBundleIdentifier": identificador ?? "link.escoita.\(nome.lowercased().replacingOccurrences(of: " ", with: ""))",
        "CFBundleShortVersionString": version, "CFBundleVersion": version,
        "CFBundleName": nome, "CFBundlePackageType": "BNDL",
    ]
    if executable {
        info["CFBundleExecutable"] = nome
        try FileManager.default.copyItem(at: URL(fileURLWithPath: "/usr/bin/true"),
                                         to: c.appendingPathComponent("MacOS/\(nome)"))
    }
    let d = try PropertyListSerialization.data(fromPropertyList: info, format: .xml, options: 0)
    try d.write(to: c.appendingPathComponent("Info.plist"))
    return b
}

func asinar(_ b: URL) { _ = try? Proceso.executar("/usr/bin/codesign", ["--force", "--deep", "-s", "-", b.path]) }

/// Un .zip coma os publicados: «<Curto>-<ver>-mac/» cos bundles dentro.
func facerZip(_ t: Temporal, curto: String, version: String,
              bundles: [(String, String)], asinados: Bool = true, cuarentena: Bool = false) throws -> URL {
    let pai = t.sub("fonte-\(UUID().uuidString)")
    let dir = pai.appendingPathComponent("\(curto)-\(version)-mac", isDirectory: true)
    try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
    for (nome, ext) in bundles {
        let b = try facerBundle(dir, nome: nome, ext: ext, version: version)
        if asinados { asinar(b) }
        if cuarentena {
            let v = "0083;00000000;Safari;"
            _ = v.withCString { setxattr(b.path, "com.apple.quarantine", $0, strlen($0), 0, 0) }
        }
    }
    try "le-me".write(to: dir.appendingPathComponent("LEEME.txt"), atomically: true, encoding: .utf8)
    let zip = t.url.appendingPathComponent("\(curto)-\(version)-mac.zip")
    try? FileManager.default.removeItem(at: zip)
    let r = try Proceso.executar("/usr/bin/ditto", ["-c", "-k", "--sequesterRsrc", "--keepParent", dir.path, zip.path])
    #expect(r.codigo == 0)
    return zip
}

let envTune = """
# Datos do produto para o núcleo de Escoita
NOME="Barullo Tune"            # nome dos bundles
CURTO="BarulloTune"            # destino de CMake
DESCRICION="Corrector de afinación"
AU="aumf Btun Esco"            # tipo, subtipo e fabricante
"""

let envSerie = """
NOME="Barullo Enxebres"
CURTO="BarulloEnxebres"
DESCRICION="Nove ferramentas dun só mando"
AU=""
# a serie
PLUGINS="Barullo Ambiente|BarulloAmbiente|aufx Bamb Esco
Barullo Brillo|BarulloBrillo|aufx Bbri Esco"
"""

func lanzamento(_ v: String, zip: URL? = nil, sha: String? = nil, tamano: Int64 = 0) -> Lanzamento {
    let a = zip.map { [Activo(nome: $0.lastPathComponent, url: $0, tamano: tamano, sha256: sha)] } ?? []
    return Lanzamento(tag: "v\(v)", nome: v, paxina: nil, publicado: nil, activos: a)
}

// MARK: - versións e produto.env

@Test func versions() {
    #expect(Version("v0.4.0")! == Version("0.4")!)
    #expect(Version("0.10.0")! > Version("0.9.9")!)
    #expect(Version("1.2.3-beta")! == Version("1.2.3")!)
    #expect(Version("0.5.1")! > Version("0.5.0")!)
    #expect(Version("") == nil)
    #expect(Version("abc") == nil)
    #expect(Version("v0.4.0")!.description == "0.4.0")
}

@Test func produtoEnvSinxelo() throws {
    let p = try #require(Produto(produtoEnv: envTune, repo: "x/barullotune"))
    #expect(p.nome == "Barullo Tune")
    #expect(p.curto == "BarulloTune")
    #expect(p.descricion == "Corrector de afinación")
    #expect(p.plugins == ["Barullo Tune"])
    #expect(p.tipo == .efecto)
    #expect(p.casa("tune") && p.casa("barullotune") && p.casa("Barullo Tune") && !p.casa("verb"))
}

@Test func produtoEnvSerie() throws {
    let p = try #require(Produto(produtoEnv: envSerie, repo: "x/enxebres"))
    #expect(p.tipo == .paquete)
    #expect(p.plugins == ["Barullo Ambiente", "Barullo Brillo"])
    #expect(p.casa("enxebres"))
    #expect(Produto(produtoEnv: "# nada\nFOO=1\n") == nil)
}

@Test func lanzamentoDeGitHub() throws {
    let json = """
    {"tag_name":"v0.1.1","name":"Barullo Ley v0.1.1","html_url":"https://github.com/a/b/releases/tag/v0.1.1",
     "published_at":"2026-10-30T10:26:28Z","assets":[
      {"name":"BarulloLey-0.1.1-mac.pkg","size":10,"browser_download_url":"https://x/p.pkg","digest":null},
      {"name":"BarulloLey-0.1.1-mac.zip","size":17797648,"browser_download_url":"https://x/z.zip",
       "digest":"sha256:4b5cdcf5c8e6381d6746c375d62cfb027a51c47ab4c790d44f248d6a4e3314bf"},
      {"name":"BarulloLey-0.1.1-windows-x64.zip","size":1,"browser_download_url":"https://x/w.zip"}]}
    """
    let o = try JSONSerialization.jsonObject(with: Data(json.utf8)) as! [String: Any]
    let l = try #require(GitHub.lanzamento(o))
    #expect(l.version == Version("0.1.1"))
    #expect(l.zipMac?.nome == "BarulloLey-0.1.1-mac.zip")
    #expect(l.zipMac?.sha256 == "4b5cdcf5c8e6381d6746c375d62cfb027a51c47ab4c790d44f248d6a4e3314bf")
    #expect(l.pkgMac?.sha256 == nil)
}

// MARK: - escaneo e estado

@Test func escaneoEEstado() throws {
    let t = Temporal()
    let usuario = t.sub("usuario"), sistema = t.sub("sistema")
    let rutas = Rutas(usuario: Ubicacion(clase: .usuario, raizPlugins: usuario, raizApps: usuario.appendingPathComponent("Applications")),
                      sistema: Ubicacion(clase: .sistema, raizPlugins: sistema, raizApps: sistema.appendingPathComponent("Applications")))
    var p = try #require(Produto(produtoEnv: envTune, repo: "x/barullotune"))
    p.ultima = lanzamento("0.4.0")
    let e = Escaner(rutas: rutas)

    #expect(Situacion(produto: p, instalados: e.instalados(p), formatos: [.vst3]).estado == .nonInstalado)

    try facerBundle(usuario.appendingPathComponent("VST3"), nome: "Barullo Tune", ext: "vst3", version: "0.3.0", executable: false)
    try facerBundle(usuario.appendingPathComponent("CLAP"), nome: "Barullo Tune", ext: "clap", version: "0.4.0", executable: false)
    var s = Situacion(produto: p, instalados: e.instalados(p), formatos: [.vst3])
    #expect(s.instalados.count == 2)
    #expect(s.estado == .actualizacion)
    #expect(Situacion(produto: p, instalados: e.instalados(p), formatos: [.clap]).estado == .alDia)

    // a de usuario ao día pero unha vella no sistema
    try FileManager.default.removeItem(at: usuario.appendingPathComponent("VST3/Barullo Tune.vst3"))
    try facerBundle(usuario.appendingPathComponent("VST3"), nome: "Barullo Tune", ext: "vst3", version: "0.4.0", executable: false)
    try facerBundle(sistema.appendingPathComponent("VST3"), nome: "Barullo Tune", ext: "vst3", version: "0.2.1", executable: false)
    s = Situacion(produto: p, instalados: e.instalados(p), formatos: [.vst3])
    #expect(s.estado == .copiaVella)
    #expect(s.duplicados.count == 2)

    // sen versión publicada
    p.ultima = nil
    #expect(Situacion(produto: p, instalados: [], formatos: [.vst3]).estado == .senDatos)
    p.lanzamentoConsultado = true
    #expect(Situacion(produto: p, instalados: [], formatos: [.vst3]).estado == .soLocal)
}

@Test func serieIncompletaEOrfos() throws {
    let t = Temporal()
    let rutas = Rutas.cartafol(t.url)
    var p = try #require(Produto(produtoEnv: envSerie))
    p.ultima = lanzamento("0.2.0")
    try facerBundle(t.sub("VST3"), nome: "Barullo Ambiente", ext: "vst3", version: "0.2.0", executable: false)
    let s = Situacion(produto: p, instalados: Escaner(rutas: rutas).instalados(p), formatos: [.vst3])
    #expect(s.estado == .incompleto)
    #expect(s.faltan == ["Barullo Brillo"])

    // un plugin de Escoita que non é de ningún produto coñecido; e un doutra marca, que non conta
    try facerBundle(t.sub("VST3"), nome: "Barullo ADT", ext: "vst3", version: "0.1.0", executable: false)
    try facerBundle(t.sub("VST3"), nome: "Outro", ext: "vst3", version: "1.0", executable: false, identificador: "com.outro.plugin")
    let o = Escaner(rutas: rutas).orfos(coñecidos: [p])
    #expect(o.map(\.plugin) == ["Barullo ADT"])
    #expect(Escaner.produtoSolto("adt", rutas: rutas)?.plugins == ["Barullo ADT"])
}

@Test func reposLocais() throws {
    let t = Temporal()
    let r = t.sub("barulloadt")
    try FileManager.default.createDirectory(at: r.appendingPathComponent("packaging"), withIntermediateDirectories: true)
    try "NOME=\"Barullo ADT\"\nCURTO=\"BarulloADT\"\nAU=\"aufx Badt Esco\"\n"
        .write(to: r.appendingPathComponent("packaging/produto.env"), atomically: true, encoding: .utf8)
    try "cmake_minimum_required(VERSION 3.22)\nproject(BarulloADT VERSION 0.1.0 LANGUAGES CXX)\n"
        .write(to: r.appendingPathComponent("CMakeLists.txt"), atomically: true, encoding: .utf8)
    try FileManager.default.createDirectory(at: t.sub("outra-cousa"), withIntermediateDirectories: true)
    let l = Descubridor.locais(en: [t.url])
    #expect(l.count == 1)
    #expect(l.first?.nome == "Barullo ADT")
    #expect(l.first?.versionLocal == Version("0.1.0"))
    let s = Situacion(produto: l[0], instalados: [], formatos: [.vst3])
    #expect(s.estado == .soLocal)
}

// MARK: - instalar, actualizar e desinstalar (nun cartafol temporal)

@Test func instalarDesdeZip() throws {
    let t = Temporal()
    let rutas = Rutas.cartafol(t.sub("plugins"))
    let lixo = t.sub("lixo")
    let inst = Instalador(rutas: rutas, papeleira: CartafolLixo(lixo), temporal: t.sub("tmp"))
    let p = try #require(Produto(produtoEnv: envTune))
    // un .zip sen asinar e con cuarentena: hai que limpalo e asinalo
    let zip = try facerZip(t, curto: "BarulloTune", version: "0.4.0",
                           bundles: [("Barullo Tune", "vst3"), ("Barullo Tune", "clap"), ("Barullo Tune", "app"), ("Outro Plugin", "vst3")],
                           asinados: false, cuarentena: true)
    let inf = try inst.instalarZip(zip, produto: p, formatos: [.vst3], versionEsperada: Version("0.4.0"))
    #expect(inf.ok)
    let vst3 = rutas.usuario.ruta(plugin: "Barullo Tune", formato: .vst3)
    #expect(InfoBundle.version(vst3) == "0.4.0")
    #expect(!Instalador.temCuarentena(vst3))
    #expect(Instalador.firmaValida(vst3))
    #expect(inf.accions.contains { $0.tipo == .aviso && $0.mensaxe.contains("ad hoc") })
    #expect(inf.accions.contains { $0.tipo == .aviso && $0.plugin == "Outro Plugin" })
    // só o formato pedido
    #expect(!FileManager.default.fileExists(atPath: rutas.usuario.ruta(plugin: "Barullo Tune", formato: .clap).path))
    #expect(!FileManager.default.fileExists(atPath: rutas.usuario.ruta(plugin: "Outro Plugin", formato: .vst3).path))
    #expect(!FileManager.default.fileExists(atPath: lixo.path))

    // a mesma versión: non se toca
    let igual = try inst.instalarZip(zip, produto: p, formatos: [.vst3], versionEsperada: nil)
    #expect(igual.accions.contains { $0.tipo == .omitido && $0.mensaxe.contains("xa está") })
    #expect(!FileManager.default.fileExists(atPath: lixo.path))

    // unha máis nova: actualiza e o vello vai ao Lixo
    let zip2 = try facerZip(t, curto: "BarulloTune", version: "0.5.0", bundles: [("Barullo Tune", "vst3"), ("Barullo Tune", "clap")])
    let act = try inst.instalarZip(zip2, produto: p, formatos: [.vst3, .clap], versionEsperada: Version("0.5.0"))
    #expect(act.ok)
    #expect(act.accions.contains { $0.tipo == .actualizado && $0.formato == .vst3 })
    #expect(act.accions.contains { $0.tipo == .instalado && $0.formato == .clap })
    #expect(InfoBundle.version(vst3) == "0.5.0")
    #expect(InfoBundle.version(lixo.appendingPathComponent("Barullo Tune.vst3")) == "0.4.0")
    #expect(Instalador.firmaValida(vst3))

    // unha máis vella: non se baixa de versión sen --forzar
    let baixar = try inst.instalarZip(zip, produto: p, formatos: [.vst3], versionEsperada: nil)
    #expect(baixar.accions.contains { $0.tipo == .omitido && $0.mensaxe.contains("máis nova") })
    #expect(InfoBundle.version(vst3) == "0.5.0")
    let forzado = try inst.instalarZip(zip, produto: p, formatos: [.vst3], versionEsperada: nil, forzar: true)
    #expect(forzado.ok)
    #expect(InfoBundle.version(vst3) == "0.4.0")
    #expect(FileManager.default.fileExists(atPath: lixo.appendingPathComponent("Barullo Tune 2.vst3").path))
}

@Test func instalarSerie() throws {
    let t = Temporal()
    let rutas = Rutas.cartafol(t.sub("plugins"))
    let inst = Instalador(rutas: rutas, papeleira: CartafolLixo(t.sub("lixo")), temporal: t.sub("tmp"))
    let p = try #require(Produto(produtoEnv: envSerie))
    let zip = try facerZip(t, curto: "BarulloEnxebres", version: "0.2.0",
                           bundles: [("Barullo Ambiente", "vst3"), ("Barullo Brillo", "vst3"), ("Barullo Ambiente", "clap")])
    let inf = try inst.instalarZip(zip, produto: p, formatos: [.vst3, .au], versionEsperada: Version("0.2.0"))
    #expect(inf.ok)
    #expect(inf.accions.filter { $0.tipo == .instalado }.count == 2)
    #expect(inf.accions.contains { $0.tipo == .aviso && $0.formato == .au })   // a serie non trae AU
    var pp = p; pp.ultima = lanzamento("0.2.0")
    #expect(Situacion(produto: pp, instalados: Escaner(rutas: rutas).instalados(pp), formatos: [.vst3]).estado == .alDia)
}

@Test func zipAlleo() throws {
    let t = Temporal()
    let inst = Instalador(rutas: Rutas.cartafol(t.sub("plugins")), papeleira: CartafolLixo(t.sub("lixo")), temporal: t.sub("tmp"))
    let p = try #require(Produto(produtoEnv: envTune))
    let zip = try facerZip(t, curto: "Outro", version: "1.0.0", bundles: [("Outro", "vst3")])
    #expect(throws: ErroInstalacion.self) { try inst.instalarZip(zip, produto: p, formatos: [.vst3], versionEsperada: nil) }
}

@Test func descargarEComprobarSuma() async throws {
    let t = Temporal()
    let rutas = Rutas.cartafol(t.sub("plugins"))
    let inst = Instalador(rutas: rutas, papeleira: CartafolLixo(t.sub("lixo")), temporal: t.sub("tmp"))
    var p = try #require(Produto(produtoEnv: envTune))
    let zip = try facerZip(t, curto: "BarulloTune", version: "0.4.0", bundles: [("Barullo Tune", "vst3")])
    let tam = (try FileManager.default.attributesOfItem(atPath: zip.path)[.size] as! NSNumber).int64Value
    let sha = try Descarga.sha256(zip)

    // suma mala: non se instala nada
    p.ultima = lanzamento("0.4.0", zip: zip, sha: String(repeating: "0", count: 64), tamano: tam)
    await #expect(throws: ErroDescarga.self) { _ = try await inst.instalar(p, formatos: [.vst3]) }
    #expect(!FileManager.default.fileExists(atPath: rutas.usuario.ruta(plugin: "Barullo Tune", formato: .vst3).path))

    // tamaño malo
    p.ultima = lanzamento("0.4.0", zip: zip, sha: sha, tamano: tam + 1)
    await #expect(throws: ErroDescarga.self) { _ = try await inst.instalar(p, formatos: [.vst3]) }

    // ben
    p.ultima = lanzamento("0.4.0", zip: zip, sha: sha, tamano: tam)
    let inf = try await inst.instalar(p, formatos: [.vst3])
    #expect(inf.ok)
    #expect(InfoBundle.version(rutas.usuario.ruta(plugin: "Barullo Tune", formato: .vst3)) == "0.4.0")
    // e o temporal queda limpo
    #expect((try? FileManager.default.contentsOfDirectory(atPath: t.sub("tmp").path))?.isEmpty ?? true)
}

@Test func desinstalarAoLixo() throws {
    let t = Temporal()
    let usuario = t.sub("usuario"), sistema = t.sub("sistema")
    let rutas = Rutas(usuario: Ubicacion(clase: .usuario, raizPlugins: usuario, raizApps: usuario.appendingPathComponent("Applications")),
                      sistema: Ubicacion(clase: .sistema, raizPlugins: sistema, raizApps: sistema.appendingPathComponent("Applications")))
    let lixo = t.sub("lixo")
    let inst = Instalador(rutas: rutas, papeleira: CartafolLixo(lixo), temporal: t.sub("tmp"))
    let p = try #require(Produto(produtoEnv: envTune))
    try facerBundle(usuario.appendingPathComponent("VST3"), nome: "Barullo Tune", ext: "vst3", version: "0.4.0", executable: false)
    try facerBundle(usuario.appendingPathComponent("CLAP"), nome: "Barullo Tune", ext: "clap", version: "0.4.0", executable: false)
    try facerBundle(sistema.appendingPathComponent("VST3"), nome: "Barullo Tune", ext: "vst3", version: "0.2.0", executable: false)

    let inf = inst.desinstalar(p, formatos: [.vst3])
    #expect(inf.ok)
    #expect(inf.accions.filter { $0.tipo == .aoLixo }.count == 1)
    #expect(!FileManager.default.fileExists(atPath: usuario.appendingPathComponent("VST3/Barullo Tune.vst3").path))
    #expect(FileManager.default.fileExists(atPath: usuario.appendingPathComponent("CLAP/Barullo Tune.clap").path))  // outro formato: queda
    #expect(FileManager.default.fileExists(atPath: sistema.appendingPathComponent("VST3/Barullo Tune.vst3").path))   // sistema: só se se pide
    #expect(InfoBundle.version(lixo.appendingPathComponent("Barullo Tune.vst3")) == "0.4.0")

    let inf2 = inst.desinstalar(p, formatos: [.vst3], incluirSistema: true)
    #expect(inf2.accions.filter { $0.tipo == .aoLixo }.count == 1)
    #expect(!FileManager.default.fileExists(atPath: sistema.appendingPathComponent("VST3/Barullo Tune.vst3").path))
}

@Test func cuarentena() throws {
    let t = Temporal()
    let b = try facerBundle(t.url, nome: "Barullo Proba", ext: "vst3", version: "1.0", executable: false)
    let f = b.appendingPathComponent("Contents/Info.plist")
    for u in [b, f] { _ = "0083;0;x;".withCString { setxattr(u.path, "com.apple.quarantine", $0, strlen($0), 0, 0) } }
    #expect(Instalador.temCuarentena(b) && Instalador.temCuarentena(f))
    Instalador.quitarCuarentena(b)
    #expect(!Instalador.temCuarentena(b) && !Instalador.temCuarentena(f))
}

// MARK: - caché de GitHub (sen rede: un URLProtocol falso)

final class GitHubFalso: URLProtocol {
    nonisolated(unsafe) static var pedidos: [URLRequest] = []
    nonisolated(unsafe) static var etag = "\"v1\""
    override class func canInit(with request: URLRequest) -> Bool { true }
    override class func canonicalRequest(for request: URLRequest) -> URLRequest { request }
    override func startLoading() {
        GitHubFalso.pedidos.append(request)
        let u = request.url!
        var estado = 200
        var corpo = Data()
        if u.path.hasSuffix("/repos") {
            corpo = Data(#"[{"full_name":"eu/barullotune","default_branch":"main","description":"d"},{"full_name":"eu/outra","default_branch":"main"}]"#.utf8)
        } else if u.path.contains("barullotune/main/packaging/produto.env") {
            corpo = Data(envTune.utf8)
        } else if u.path.contains("/releases/latest") {
            corpo = Data(#"{"tag_name":"v0.4.0","assets":[]}"#.utf8)
        } else {
            estado = 404
        }
        if request.value(forHTTPHeaderField: "If-None-Match") == GitHubFalso.etag, estado == 200 { estado = 304; corpo = Data() }
        let r = HTTPURLResponse(url: u, statusCode: estado, httpVersion: "HTTP/1.1",
                                headerFields: ["Etag": GitHubFalso.etag, "x-ratelimit-remaining": "59"])!
        client?.urlProtocol(self, didReceive: r, cacheStoragePolicy: .notAllowed)
        client?.urlProtocol(self, didLoad: corpo)
        client?.urlProtocolDidFinishLoading(self)
    }
    override func stopLoading() {}
}

@Test func cacheDeGitHub() async throws {
    let t = Temporal()
    let c = URLSessionConfiguration.ephemeral
    c.protocolClasses = [GitHubFalso.self]
    let sesion = URLSession(configuration: c)
    GitHubFalso.pedidos = []
    let gh = GitHub(propietario: "eu", cache: t.sub("cache"), vixencia: 3600, sesion: sesion)
    var ax = Axustes(); ax.propietario = "eu"; ax.cartafolesLocais = []

    let cat = await Descubridor(github: gh, axustes: ax).descubrir()
    #expect(cat.produtos.map(\.nome) == ["Barullo Tune"])        // «outra» non ten produto.env
    #expect(cat.produtos.first?.ultima?.version == Version("0.4.0"))
    #expect(gh.restantes == 59)
    let primeiros = GitHubFalso.pedidos.count
    #expect(primeiros == 4)   // repos, 2 produto.env, 1 release

    // dentro da vixencia: ningunha pregunta
    _ = await Descubridor(github: gh, axustes: ax).descubrir()
    #expect(GitHubFalso.pedidos.count == primeiros)

    // forzando: pregúntase co ETag e a resposta 304 reutiliza o gardado
    let cat2 = await Descubridor(github: gh, axustes: ax).descubrir(forzar: true)
    #expect(GitHubFalso.pedidos.count == primeiros * 2)
    #expect(GitHubFalso.pedidos.suffix(primeiros).allSatisfy { $0.value(forHTTPHeaderField: "If-None-Match") != nil })
    #expect(cat2.produtos.first?.ultima?.version == Version("0.4.0"))

    // sen rede: o da caché
    let gh2 = GitHub(propietario: "eu", cache: t.sub("cache"), sesion: sesion)
    gh2.soCache = true
    let cat3 = await Descubridor(github: gh2, axustes: ax).descubrir(forzar: true)
    #expect(cat3.produtos.count == 1)
    #expect(GitHubFalso.pedidos.count == primeiros * 2)
}

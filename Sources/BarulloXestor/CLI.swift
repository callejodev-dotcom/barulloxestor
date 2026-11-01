import Foundation
import XestorNucleo

/// O modo sen xanela: `BarulloXestor --listar`, `--instalar <nome> --dir <cartafol>`...
enum CLI {
    static let axuda = """
    Barullo Xestor — os plugins Barullo de Escoita

    Uso (sen xanela):
      --listar                      lista os plugins, o instalado e a última versión
      --instalar <nome>             instala a última versión (nos formatos escollidos)
      --actualizar <nome>           o mesmo, se hai versión nova
      --actualizar-todo             actualiza todo o que estea vello ou incompleto
      --desinstalar <nome>          leva o instalado ao Lixo

    Opcións:
      --dir <cartafol>              en vez de ~/Library/Audio/Plug-Ins: <cartafol>/VST3, /Components, /CLAP
                                    e <cartafol>/Applications (sen mirar /Library)
      --formatos vst3,au,clap,app   formatos (por defecto os dos axustes: VST3)
      --papeleira <cartafol>        o que se substitúe ou quita vai alí en vez de ao Lixo
      --zip <ficheiro>              instalar desde un .zip xa descargado (con --instalar)
      --forzar                      instalar aínda que a versión instalada sexa igual ou máis nova
      --incluir-sistema             desinstalar tamén as copias de /Library (se se pode sen admin)
      --sen-rede                    só o que haxa na caché
      --refrescar                   preguntar a GitHub aínda que a caché sexa recente
      --json                        a lista en JSON
      --captura <ficheiro.png>      debuxa a xanela nun PNG (sen abrila; para revisar o deseño)
      --axuda

    O <nome> pode ser «tune», «Barullo Tune», «barullotune», «enxebres»...
    """

    static func eCLI(_ args: [String]) -> Bool {
        args.dropFirst().contains { $0.hasPrefix("--") }
    }

    struct Opcions {
        var orde = ""
        var nome: String?
        var dir: URL?
        var formatos: [Formato]?
        var papeleira: URL?
        var zip: URL?
        var forzar = false
        var incluirSistema = false
        var senRede = false
        var refrescar = false
        var json = false
        var captura: URL?
    }

    static func ler(_ args: [String]) throws -> Opcions {
        var o = Opcions()
        var i = 1
        func seguinte(_ n: String) throws -> String {
            i += 1
            guard i < args.count else { throw NSError(domain: "CLI", code: 2, userInfo: [NSLocalizedDescriptionKey: "\(n) precisa un valor"]) }
            return args[i]
        }
        func ruta(_ s: String) -> URL { URL(fileURLWithPath: (s as NSString).expandingTildeInPath).standardizedFileURL }
        while i < args.count {
            let a = args[i]
            switch a {
            case "--listar", "--actualizar-todo", "--axuda", "-h", "--help": o.orde = a
            case "--instalar", "--actualizar", "--desinstalar": o.orde = a; o.nome = try seguinte(a)
            case "--dir": o.dir = ruta(try seguinte(a))
            case "--papeleira": o.papeleira = ruta(try seguinte(a))
            case "--zip": o.zip = ruta(try seguinte(a))
            case "--formatos":
                let t = try seguinte(a)
                guard let f = Formato.lista(t) else { throw NSError(domain: "CLI", code: 2, userInfo: [NSLocalizedDescriptionKey: "formatos descoñecidos: \(t)"]) }
                o.formatos = f
            case "--forzar": o.forzar = true
            case "--incluir-sistema": o.incluirSistema = true
            case "--sen-rede": o.senRede = true
            case "--refrescar": o.refrescar = true
            case "--json": o.json = true
            case "--captura": o.orde = a; o.captura = ruta(try seguinte(a))
            default:
                if a.hasPrefix("-psn_") || a.hasPrefix("-NS") { break }
                throw NSError(domain: "CLI", code: 2, userInfo: [NSLocalizedDescriptionKey: "non se entende «\(a)» (--axuda)"])
            }
            i += 1
        }
        return o
    }

    static func executar(_ args: [String]) async -> Int32 {
        let o: Opcions
        do { o = try ler(args) } catch { eprint(error.localizedDescription); return 2 }
        if o.orde.isEmpty || o.orde == "--axuda" || o.orde == "-h" || o.orde == "--help" {
            print(axuda); return o.orde.isEmpty ? 2 : 0
        }

        if o.orde == "--captura", let u = o.captura {
            return await Captura.facer(u, formatos: o.formatos)
        }

        let axustes = Axustes.cargar()
        let formatos = o.formatos ?? axustes.formatos
        let rutas = o.dir.map(Rutas.cartafol) ?? Rutas.reais()
        let papeleira: Papeleira = o.papeleira.map(CartafolLixo.init) ?? LixoDoSistema()
        let gh = GitHub(propietario: axustes.propietario, cache: GitHub.cacheEstandar,
                        token: (axustes.usarGh && !o.senRede) ? GitHub.tokenDispoñible() : nil)
        gh.soCache = o.senRede
        let catalogo = await Descubridor(github: gh, axustes: axustes).descubrir(forzar: o.refrescar)
        for a in catalogo.avisos { eprint("aviso: \(a)") }
        let escaner = Escaner(rutas: rutas)
        let instalador = Instalador(rutas: rutas, papeleira: papeleira)

        switch o.orde {
        case "--listar":
            listar(catalogo, escaner: escaner, formatos: formatos, json: o.json, rutas: rutas, gh: gh)
            return 0

        case "--instalar", "--actualizar":
            guard let nome = o.nome, let p = catalogo.buscar(nome) else {
                eprint("Non se atopa «\(o.nome ?? "")». Hai: \(catalogo.produtos.map(\.nome).joined(separator: ", "))")
                return 1
            }
            let s = Situacion(produto: p, instalados: escaner.instalados(p), formatos: formatos)
            let nadaQueFacer = o.orde == "--actualizar" ? !s.estado.pideActualizar : s.estado == .alDia
            if nadaQueFacer, !o.forzar, o.zip == nil {
                print("\(p.nome): \(s.estado.etiqueta); nada que facer (--forzar para instalalo igual).")
                return 0
            }
            return await instalar(p, instalador: instalador, formatos: formatos, forzar: o.forzar, zip: o.zip)

        case "--actualizar-todo":
            var codigo: Int32 = 0
            var algo = false
            for p in catalogo.produtos {
                let s = Situacion(produto: p, instalados: escaner.instalados(p), formatos: formatos)
                guard s.estado.pideActualizar else { continue }
                algo = true
                if await instalar(p, instalador: instalador, formatos: formatos, forzar: false, zip: nil) != 0 { codigo = 1 }
            }
            if !algo { print("Todo ao día.") }
            return codigo

        case "--desinstalar":
            guard let nome = o.nome, let p = catalogo.buscar(nome) ?? Escaner.produtoSolto(nome, rutas: rutas) else {
                eprint("Non se atopa «\(o.nome ?? "")»"); return 1
            }
            let inf = instalador.desinstalar(p, formatos: formatos, incluirSistema: o.incluirSistema)
            inf.accions.forEach { print($0) }
            return inf.ok ? 0 : 1

        default:
            print(axuda); return 2
        }
    }

    static func instalar(_ p: Produto, instalador: Instalador, formatos: [Formato], forzar: Bool, zip: URL?) async -> Int32 {
        do {
            let inf: Informe
            if let zip {
                print("\(p.nome): instalando desde \(zip.path)")
                inf = try instalador.instalarZip(zip, produto: p, formatos: formatos, versionEsperada: nil, forzar: forzar)
            } else {
                let tam = p.ultima?.zipMac.map { ByteCountFormatter.string(fromByteCount: $0.tamano, countStyle: .file) } ?? "?"
                print("\(p.nome): descargando \(p.ultima?.zipMac?.nome ?? "?") (\(tam))")
                let ultimo = Ultimo()
                inf = try await instalador.instalar(p, formatos: formatos, forzar: forzar) { f in
                    let pc = Int(f * 10)
                    if ultimo.cambia(pc) { FileHandle.standardError.write("  \(pc * 10)%\n".data(using: .utf8)!) }
                }
            }
            inf.accions.forEach { print("  \($0)") }
            return inf.ok ? 0 : 1
        } catch {
            eprint("\(p.nome): \(error.localizedDescription)")
            return 1
        }
    }

    final class Ultimo: @unchecked Sendable {
        private var v = -1
        private let l = NSLock()
        func cambia(_ n: Int) -> Bool { l.lock(); defer { l.unlock() }; if n != v { v = n; return true }; return false }
    }

    static func listar(_ c: Catalogo, escaner: Escaner, formatos: [Formato], json: Bool, rutas: Rutas, gh: GitHub) {
        let orfos = escaner.orfos(coñecidos: c.produtos)
        if json {
            var lista: [[String: Any]] = []
            for p in c.produtos {
                let s = Situacion(produto: p, instalados: escaner.instalados(p), formatos: formatos)
                lista.append([
                    "nome": p.nome, "curto": p.curto, "tipo": p.tipo.rawValue, "descricion": p.descricion,
                    "repo": p.repo ?? NSNull(), "rutaLocal": p.rutaLocal?.path ?? NSNull(),
                    "ultima": p.ultima?.version?.description ?? NSNull(), "estado": s.estado.rawValue,
                    "plugins": p.plugins,
                    "instalados": s.instalados.map { ["plugin": $0.plugin, "formato": $0.formato.rawValue,
                                                      "ubicacion": $0.ubicacion.etiqueta, "version": $0.textoVersion ?? NSNull(),
                                                      "ruta": $0.url.path] as [String: Any] },
                ])
            }
            let o: [String: Any] = ["formatos": formatos.map(\.rawValue), "produtos": lista,
                                    "orfos": orfos.map { ["plugin": $0.plugin, "formato": $0.formato.rawValue, "version": $0.textoVersion ?? NSNull(), "ruta": $0.url.path] as [String: Any] }]
            if let d = try? JSONSerialization.data(withJSONObject: o, options: [.prettyPrinted, .sortedKeys]) {
                print(String(decoding: d, as: UTF8.self))
            }
            return
        }
        print("Formatos: \(formatos.map(\.etiqueta).joined(separator: ", "))   ·   cartafol: \(rutas.usuario.raizPlugins.path)\(rutas.sistema == nil ? "" : " e /Library")")
        var tipo: TipoProduto?
        for p in c.produtos {
            if p.tipo != tipo { tipo = p.tipo; print("\n\(p.tipo.titulo.uppercased())") }
            let s = Situacion(produto: p, instalados: escaner.instalados(p), formatos: formatos)
            let ultima = p.ultima?.version.map { "v\($0)" } ?? (p.versionLocal.map { "local \($0)" } ?? "—")
            print("  \(pad(p.nome, 20)) \(pad(ultima, 12)) \(pad(s.estado.etiqueta, 24)) \(p.descricion)")
            // as copias instaladas, agrupadas por formato e ubicación
            var grupos: [String: [Instalado]] = [:]
            for i in s.relevantes { grupos["\(i.formato.etiqueta) \(i.ubicacion.etiqueta)", default: []].append(i) }
            for (k, v) in grupos.sorted(by: { $0.key < $1.key }) {
                let vers = Set(v.map { $0.textoVersion ?? "?" }).sorted().joined(separator: "/")
                let cantos = p.eSerie ? " (\(v.count)/\(p.plugins.count))" : ""
                let admin = v.contains(where: \.precisaAdmin) ? " · precisa admin" : ""
                print("      \(k): \(vers)\(cantos)\(admin)")
            }
            if !s.faltan.isEmpty { print("      faltan: \(s.faltan.joined(separator: ", "))") }
            let outros = s.instalados.filter { !formatos.contains($0.formato) }
            if !outros.isEmpty {
                let r = Dictionary(grouping: outros, by: { "\($0.formato.etiqueta) \($0.ubicacion.etiqueta)" })
                    .map { "\($0.key) \(Set($0.value.map { $0.textoVersion ?? "?" }).sorted().joined(separator: "/"))" }.sorted()
                print("      (noutros formatos: \(r.joined(separator: ", ")))")
            }
        }
        if !orfos.isEmpty {
            print("\nOUTROS DE ESCOITA INSTALADOS (sen repo coñecido)")
            for i in orfos { print("  \(pad(i.plugin, 20)) \(pad(i.textoVersion ?? "?", 12)) \(i.formato.etiqueta) \(i.ubicacion.etiqueta)") }
        }
        if let r = gh.restantes { print("\nGitHub: quedan \(r) preguntas nesta hora.") }
    }

    static func pad(_ s: String, _ n: Int) -> String { s.count >= n ? s : s + String(repeating: " ", count: n - s.count) }
    static func eprint(_ s: String) { FileHandle.standardError.write((s + "\n").data(using: .utf8)!) }
}

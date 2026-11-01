import AppKit
import Foundation
import XestorNucleo

/// O estado da xanela. A lóxica está no núcleo (XestorNucleo); isto só a chama e garda o resultado.
@MainActor
final class Modelo: ObservableObject {
    @Published var axustes: Axustes
    @Published private(set) var catalogo: Catalogo?
    @Published private(set) var situacions: [String: Situacion] = [:]
    @Published private(set) var orfos: [Instalado] = []
    @Published private(set) var cargando = false
    /// Os produtos cun traballo en marcha: o progreso (0...1) ou nil se non se sabe.
    @Published private(set) var traballos: [String: Double?] = [:]
    @Published var mensaxe = ""
    @Published var erro: String?
    @Published private(set) var restantes: Int?

    let rutas = Rutas.reais()
    private var github: GitHub

    init() {
        let a = Axustes.cargar()
        axustes = a
        github = GitHub(propietario: a.propietario, cache: GitHub.cacheEstandar)
    }

    var formatos: [Formato] { axustes.formatos }

    func cambiarFormato(_ f: Formato) {
        var l = axustes.formatos
        if let i = l.firstIndex(of: f) { if l.count > 1 { l.remove(at: i) } } else { l.append(f) }
        axustes.formatos = Formato.allCases.filter(l.contains)
        try? axustes.gardar()
        reescanear()
    }

    // MARK: - cargar

    func cargar(forzar: Bool = false) async {
        guard !cargando else { return }
        cargando = true
        mensaxe = "Preguntando a GitHub…"
        let ax = axustes
        let gh = github
        let c: Catalogo = await Task.detached {
            if ax.usarGh, gh.token == nil { gh.token = GitHub.tokenDispoñible() }
            return await Descubridor(github: gh, axustes: ax).descubrir(forzar: forzar)
        }.value
        catalogo = c
        restantes = gh.restantes
        reescanear()
        cargando = false
        let f = DateFormatter.localizedString(from: c.data, dateStyle: .none, timeStyle: .short)
        mensaxe = c.avisos.first ?? "Ao día ás \(f)."
    }

    /// Volver ler o disco (sen preguntar a GitHub).
    func reescanear() {
        guard let c = catalogo else { return }
        let e = Escaner(rutas: rutas)
        var s: [String: Situacion] = [:]
        for p in c.produtos { s[p.id] = Situacion(produto: p, instalados: e.instalados(p), formatos: formatos) }
        situacions = s
        orfos = e.orfos(coñecidos: c.produtos).filter { formatos.contains($0.formato) }
    }

    var produtos: [Produto] { catalogo?.produtos ?? [] }

    func situacion(_ p: Produto) -> Situacion {
        situacions[p.id] ?? Situacion(produto: p, instalados: [], formatos: formatos)
    }

    var porActualizar: [Produto] { produtos.filter { situacion($0).estado.pideActualizar } }

    // MARK: - accións

    private var instalador: Instalador { Instalador(rutas: rutas) }

    func instalar(_ p: Produto) async {
        guard traballos[p.id] == nil else { return }
        traballos[p.id] = .some(0)
        mensaxe = "\(p.nome): descargando…"
        let inst = instalador, f = formatos
        let id = p.id
        let paso = Paso()
        do {
            let inf = try await Task.detached {
                try await inst.instalar(p, formatos: f) { v in
                    guard paso.novo(v) else { return }   // non asoballar a xanela: un aviso por cada 1 %
                    Task { @MainActor [weak self] in
                        if self?.traballos[id] != nil { self?.traballos[id] = .some(v) }
                    }
                }
            }.value
            rematar(p, inf)
        } catch {
            traballos[p.id] = nil
            erro = "\(p.nome): \(error.localizedDescription)"
            mensaxe = erro ?? ""
        }
    }

    func actualizarTodo() async {
        for p in porActualizar { await instalar(p) }
    }

    /// As copias que levaría ao Lixo «Desinstalar».
    func copiasParaQuitar(_ p: Produto) -> [Instalado] { situacion(p).relevantes }

    func desinstalar(_ p: Produto, incluirSistema: Bool) async {
        traballos[p.id] = .some(nil)
        let inst = instalador, f = formatos
        let inf = await Task.detached { inst.desinstalar(p, formatos: f, incluirSistema: incluirSistema) }.value
        rematar(p, inf)
    }

    /// Quita as copias vellas de /Library (cando a do usuario xa está ao día).
    func quitarCopiasVellas(_ p: Produto) async {
        let vellas = situacion(p).vellas.filter { $0.ubicacion.clase == .sistema }
        traballos[p.id] = .some(nil)
        let inst = instalador
        let inf = await Task.detached { inst.botar(vellas) }.value
        rematar(p, inf)
    }

    func desinstalarOrfo(_ i: Instalado) async {
        let inst = instalador
        let inf = await Task.detached { inst.botar([i]) }.value
        mensaxe = inf.accions.map(\.description).joined(separator: " · ")
        if !inf.ok { erro = mensaxe }
        reescanear()
    }

    private func rematar(_ p: Produto, _ inf: Informe) {
        traballos[p.id] = nil
        reescanear()
        let erros = inf.accions.filter { $0.tipo == .erro }
        let feitos = inf.accions.filter { [.instalado, .actualizado, .aoLixo].contains($0.tipo) }
        if !erros.isEmpty {
            erro = "\(p.nome):\n" + erros.map(\.description).joined(separator: "\n")
        }
        let omitidos = inf.accions.filter { $0.tipo == .omitido }
        if feitos.isEmpty, let o = omitidos.first {
            mensaxe = "\(p.nome): \(o.mensaxe)"
        } else {
            let avisos = inf.accions.filter { $0.tipo == .aviso && $0.mensaxe.contains("ad hoc") }.count
            mensaxe = "\(p.nome): \(feitos.count) cambio\(feitos.count == 1 ? "" : "s")"
                + (avisos > 0 ? " (\(avisos) asinado\(avisos == 1 ? "" : "s") de novo)" : "")
                + (feitos.contains { $0.tipo != .instalado } ? "; o vello, no Lixo." : ".")
        }
    }

    func amosarNoFinder(_ urls: [URL]) { NSWorkspace.shared.activateFileViewerSelecting(urls) }
    func abrir(_ u: URL?) { if let u { NSWorkspace.shared.open(u) } }
}

/// Deixa pasar o progreso só cando avanza un 1 %.
final class Paso: @unchecked Sendable {
    private var ultimo = -1
    private let l = NSLock()
    func novo(_ v: Double) -> Bool {
        let n = Int(v * 100)
        l.lock(); defer { l.unlock() }
        if n == ultimo { return false }
        ultimo = n; return true
    }
}

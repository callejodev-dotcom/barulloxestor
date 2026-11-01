import Foundation

/// Un sitio onde poden estar os plugins: o do usuario (~/Library) ou o do sistema (/Library, precisa admin).
public struct Ubicacion: Hashable, Sendable {
    public enum Clase: String, Sendable { case usuario, sistema }
    public var clase: Clase
    public var raizPlugins: URL   // ten dentro VST3, Components e CLAP
    public var raizApps: URL      // onde van as apps independentes

    public init(clase: Clase, raizPlugins: URL, raizApps: URL) {
        self.clase = clase; self.raizPlugins = raizPlugins; self.raizApps = raizApps
    }

    public func cartafol(_ f: Formato) -> URL {
        if let sub = f.subcartafol { return raizPlugins.appendingPathComponent(sub, isDirectory: true) }
        return raizApps
    }

    public func ruta(plugin: String, formato f: Formato) -> URL {
        cartafol(f).appendingPathComponent("\(plugin).\(f.extension_)", isDirectory: true)
    }

    /// Pódese escribir sen admin?
    public func escribible(_ f: Formato) -> Bool {
        let c = cartafol(f)
        if FileManager.default.fileExists(atPath: c.path) { return FileManager.default.isWritableFile(atPath: c.path) }
        return FileManager.default.isWritableFile(atPath: c.deletingLastPathComponent().path)
    }

    public var etiqueta: String { clase == .usuario ? "usuario" : "sistema" }
}

/// Onde mirar e onde instalar. A instalación vai sempre á ubicación do usuario.
public struct Rutas: Sendable {
    public var usuario: Ubicacion
    public var sistema: Ubicacion?

    public var todas: [Ubicacion] { [usuario] + (sistema.map { [$0] } ?? []) }

    public init(usuario: Ubicacion, sistema: Ubicacion?) { self.usuario = usuario; self.sistema = sistema }

    /// As de verdade: ~/Library/Audio/Plug-Ins (+ ~/Applications) e /Library/Audio/Plug-Ins (+ /Applications).
    public static func reais() -> Rutas {
        let casa = FileManager.default.homeDirectoryForCurrentUser
        return Rutas(
            usuario: Ubicacion(clase: .usuario,
                               raizPlugins: casa.appendingPathComponent("Library/Audio/Plug-Ins", isDirectory: true),
                               raizApps: casa.appendingPathComponent("Applications", isDirectory: true)),
            sistema: Ubicacion(clase: .sistema,
                               raizPlugins: URL(fileURLWithPath: "/Library/Audio/Plug-Ins", isDirectory: true),
                               raizApps: URL(fileURLWithPath: "/Applications", isDirectory: true)))
    }

    /// Un cartafol calquera (probas, ou `--dir`): <dir>/VST3, <dir>/Components, <dir>/CLAP e <dir>/Applications.
    /// Sen ubicación do sistema.
    public static func cartafol(_ dir: URL) -> Rutas {
        Rutas(usuario: Ubicacion(clase: .usuario, raizPlugins: dir,
                                 raizApps: dir.appendingPathComponent("Applications", isDirectory: true)),
              sistema: nil)
    }
}

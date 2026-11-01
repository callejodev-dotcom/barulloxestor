import Foundation

/// Onde van os bundles que se quitan: nunca se borran, van ao Lixo.
public protocol Papeleira: Sendable {
    /// Move o elemento e devolve onde quedou.
    @discardableResult func botar(_ url: URL) throws -> URL
}

/// O Lixo de verdade (Finder → «Volver poñer» funciona).
public struct LixoDoSistema: Papeleira {
    public init() {}
    public func botar(_ url: URL) throws -> URL {
        var resultado: NSURL?
        try FileManager.default.trashItem(at: url, resultingItemURL: &resultado)
        return (resultado as URL?) ?? url
    }
}

/// Un cartafol calquera feito Lixo (para as probas e para `--papeleira`).
public struct CartafolLixo: Papeleira {
    public let cartafol: URL
    public init(_ cartafol: URL) { self.cartafol = cartafol }
    public func botar(_ url: URL) throws -> URL {
        try FileManager.default.createDirectory(at: cartafol, withIntermediateDirectories: true)
        var destino = cartafol.appendingPathComponent(url.lastPathComponent)
        var n = 2
        while FileManager.default.fileExists(atPath: destino.path) {
            let base = url.deletingPathExtension().lastPathComponent, ext = url.pathExtension
            destino = cartafol.appendingPathComponent("\(base) \(n)" + (ext.isEmpty ? "" : ".\(ext)"))
            n += 1
        }
        try FileManager.default.moveItem(at: url, to: destino)
        return destino
    }
}

import CryptoKit
import Foundation

public enum ErroDescarga: Error, LocalizedError {
    case http(Int)
    case tamano(esperado: Int64, real: Int64)
    case suma(esperada: String, real: String)

    public var errorDescription: String? {
        switch self {
        case .http(let c): return "A descarga fallou (HTTP \(c))"
        case .tamano(let e, let r): return "O ficheiro descargado non mide o que debe (\(r) bytes, e son \(e))"
        case .suma(let e, let r): return "A suma SHA-256 do ficheiro non coincide (\(r.prefix(12))…, e é \(e.prefix(12))…)"
        }
    }
}

/// Descarga un ficheiro a disco avisando do progreso (0...1).
public final class Descarga: NSObject, URLSessionDownloadDelegate, @unchecked Sendable {
    private var continuacion: CheckedContinuation<URL, Error>?
    private let destino: URL
    private let progreso: @Sendable (Double) -> Void
    private var movido: URL?
    private var erroMover: Error?

    private init(destino: URL, progreso: @escaping @Sendable (Double) -> Void) {
        self.destino = destino; self.progreso = progreso
    }

    /// Descarga `url` en `destino`. Os URL file:// cópianse (para as probas).
    public static func baixar(_ url: URL, a destino: URL, progreso: @escaping @Sendable (Double) -> Void = { _ in }) async throws -> URL {
        try? FileManager.default.removeItem(at: destino)
        if url.isFileURL {
            try FileManager.default.copyItem(at: url, to: destino)
            progreso(1)
            return destino
        }
        let d = Descarga(destino: destino, progreso: progreso)
        let config = URLSessionConfiguration.ephemeral
        config.timeoutIntervalForRequest = 60
        let sesion = URLSession(configuration: config, delegate: d, delegateQueue: nil)
        defer { sesion.finishTasksAndInvalidate() }
        return try await withCheckedThrowingContinuation { c in
            d.continuacion = c
            var req = URLRequest(url: url)
            req.setValue("BarulloXestor", forHTTPHeaderField: "User-Agent")
            sesion.downloadTask(with: req).resume()
        }
    }

    public func urlSession(_ s: URLSession, downloadTask t: URLSessionDownloadTask, didWriteData _: Int64,
                           totalBytesWritten escrito: Int64, totalBytesExpectedToWrite total: Int64) {
        if total > 0 { progreso(Double(escrito) / Double(total)) }
    }

    public func urlSession(_ s: URLSession, downloadTask t: URLSessionDownloadTask, didFinishDownloadingTo u: URL) {
        // o ficheiro temporal desaparece ao saír desta función: hai que movelo xa
        if let h = t.response as? HTTPURLResponse, h.statusCode != 200 {
            erroMover = ErroDescarga.http(h.statusCode); return
        }
        do { try FileManager.default.moveItem(at: u, to: destino); movido = destino } catch { erroMover = error }
    }

    public func urlSession(_ s: URLSession, task: URLSessionTask, didCompleteWithError error: Error?) {
        let c = continuacion; continuacion = nil
        if let error { c?.resume(throwing: error) }
        else if let erroMover { c?.resume(throwing: erroMover) }
        else if let movido { progreso(1); c?.resume(returning: movido) }
        else { c?.resume(throwing: ErroDescarga.http(0)) }
    }

    /// SHA-256 dun ficheiro, lido a anacos.
    public static func sha256(_ u: URL) throws -> String {
        let h = try FileHandle(forReadingFrom: u)
        defer { try? h.close() }
        var s = SHA256()
        while let d = try h.read(upToCount: 1 << 20), !d.isEmpty { s.update(data: d) }
        return s.finalize().map { String(format: "%02x", $0) }.joined()
    }

    /// Comproba tamaño e suma contra o que di GitHub.
    public static func verificar(_ u: URL, _ a: Activo) throws {
        let t = (try FileManager.default.attributesOfItem(atPath: u.path)[.size] as? NSNumber)?.int64Value ?? -1
        if a.tamano > 0, t != a.tamano { throw ErroDescarga.tamano(esperado: a.tamano, real: t) }
        if let e = a.sha256?.lowercased() {
            let r = try sha256(u)
            if r != e { throw ErroDescarga.suma(esperada: e, real: r) }
        }
    }
}

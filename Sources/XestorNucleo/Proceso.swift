import Foundation

/// Executa unha orde (ditto, codesign...) e recolle a saída.
public enum Proceso {
    public struct Resultado: Sendable {
        public var codigo: Int32
        public var saida: String
        public var erro: String
    }

    @discardableResult
    public static func executar(_ programa: String, _ argumentos: [String], tempoMaximo: TimeInterval = 600) throws -> Resultado {
        let p = Process()
        p.executableURL = URL(fileURLWithPath: programa)
        p.arguments = argumentos
        let so = Pipe(), se = Pipe()
        p.standardOutput = so; p.standardError = se
        p.standardInput = FileHandle.nullDevice
        // lense ao mesmo tempo para que unha saída longa non bloquee o proceso
        var dOut = Data(), dErr = Data()
        let g = DispatchGroup()
        g.enter(); DispatchQueue.global().async { dOut = so.fileHandleForReading.readDataToEndOfFile(); g.leave() }
        g.enter(); DispatchQueue.global().async { dErr = se.fileHandleForReading.readDataToEndOfFile(); g.leave() }
        try p.run()
        let limite = Date().addingTimeInterval(tempoMaximo)
        while p.isRunning {
            if Date() > limite { p.terminate(); break }
            Thread.sleep(forTimeInterval: 0.02)
        }
        p.waitUntilExit()
        g.wait()
        return Resultado(codigo: p.terminationStatus,
                         saida: String(decoding: dOut, as: UTF8.self), erro: String(decoding: dErr, as: UTF8.self))
    }
}

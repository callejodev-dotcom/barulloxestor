import Foundation

/// Unha versión «maior.menor.parche» (acepta «v0.4.0», «0.4» ou «1.2.3-beta»).
/// Compárase coma no núcleo de Escoita: número a número, os que faltan valen 0.
public struct Version: Comparable, Hashable, CustomStringConvertible, Sendable {
    public let partes: [Int]

    public init?(_ texto: String) {
        var s = texto.trimmingCharacters(in: .whitespacesAndNewlines)
        if s.hasPrefix("v") || s.hasPrefix("V") { s.removeFirst() }
        // o que vaia tras un «-» ou «+» (beta, compilación...) non conta
        if let corte = s.firstIndex(where: { $0 == "-" || $0 == "+" }) { s = String(s[..<corte]) }
        let anacos = s.split(separator: ".", omittingEmptySubsequences: false)
        guard !anacos.isEmpty else { return nil }
        var n: [Int] = []
        for a in anacos {
            let dixitos = a.prefix(while: { $0.isNumber })
            guard let v = Int(dixitos) else { return nil }
            n.append(v)
        }
        partes = n
    }

    private func normalizada() -> [Int] {
        var p = partes
        while p.count < 3 { p.append(0) }
        return p
    }

    public static func < (a: Version, b: Version) -> Bool {
        let x = a.normalizada(), y = b.normalizada()
        for i in 0..<max(x.count, y.count) {
            let u = i < x.count ? x[i] : 0, v = i < y.count ? y[i] : 0
            if u != v { return u < v }
        }
        return false
    }

    public static func == (a: Version, b: Version) -> Bool { !(a < b) && !(b < a) }

    public func hash(into h: inout Hasher) {
        var p = normalizada()
        while p.count > 3, p.last == 0 { p.removeLast() }
        h.combine(p)
    }

    public var description: String { partes.map(String.init).joined(separator: ".") }
}

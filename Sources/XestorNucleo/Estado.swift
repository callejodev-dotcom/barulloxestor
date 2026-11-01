import Foundation

/// Como está un produto nesta máquina, mirando só os formatos escollidos.
public enum Estado: String, Sendable {
    case alDia, actualizacion, copiaVella, incompleto, nonInstalado, soLocal, senDatos

    public var etiqueta: String {
        switch self {
        case .alDia: return "ao día"
        case .actualizacion: return "actualización"
        case .copiaVella: return "copia vella no sistema"
        case .incompleto: return "incompleto"
        case .nonInstalado: return "non instalado"
        case .soLocal: return "só local"
        case .senDatos: return "sen datos"
        }
    }

    /// Hai algo que facer con «Actualizar todo»?
    public var pideActualizar: Bool { self == .actualizacion || self == .incompleto }
}

public struct Situacion: Sendable {
    public var produto: Produto
    public var instalados: [Instalado]   // todas as copias, en todos os formatos
    public var formatos: [Formato]

    public init(produto: Produto, instalados: [Instalado], formatos: [Formato]) {
        self.produto = produto; self.instalados = instalados; self.formatos = formatos
    }

    /// As copias nos formatos escollidos.
    public var relevantes: [Instalado] { instalados.filter { formatos.contains($0.formato) } }

    /// As copias máis vellas cá última publicada.
    public var vellas: [Instalado] {
        guard let u = produto.ultima?.version else { return [] }
        return relevantes.filter { ($0.version ?? Version("0")!) < u }
    }

    /// Plugins dunha serie que non teñen ningunha copia nos formatos escollidos (se outros si a teñen).
    public var faltan: [String] {
        let con = Set(relevantes.map(\.plugin))
        return con.isEmpty ? [] : produto.plugins.filter { !con.contains($0) }
    }

    /// Plugins que están á vez no cartafol do usuario e no do sistema (no mesmo formato).
    public var duplicados: [Instalado] {
        let r = relevantes
        return r.filter { a in r.contains { b in b.plugin == a.plugin && b.formato == a.formato && b.ubicacion.clase != a.ubicacion.clase } }
    }

    /// A versión máis baixa instalada (a que manda á hora de actualizar).
    public var versionInstalada: Version? { relevantes.compactMap(\.version).min() }

    public var estado: Estado {
        guard produto.ultima?.version != nil else {
            // sen versión publicada: ou non está en GitHub, ou aínda non ten ningunha (só local);
            // se non se puido preguntar, non se sabe
            return produto.repo == nil || produto.lanzamentoConsultado ? .soLocal : .senDatos
        }
        if relevantes.isEmpty { return .nonInstalado }
        let v = vellas
        if !v.isEmpty {
            // se o único vello é unha copia en /Library e no cartafol do usuario xa está a nova, non hai
            // que descargar nada: só quitar esa copia
            let u = Set(relevantes.filter { $0.ubicacion.clase == .usuario }.map { "\($0.plugin)|\($0.formato)" })
            let soSistema = v.allSatisfy { $0.ubicacion.clase == .sistema && u.contains("\($0.plugin)|\($0.formato)") }
            return soSistema ? .copiaVella : .actualizacion
        }
        if !faltan.isEmpty { return .incompleto }
        return .alDia
    }
}

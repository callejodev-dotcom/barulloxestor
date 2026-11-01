import SwiftUI
import XestorNucleo

struct Xanela: View {
    @EnvironmentObject var m: Modelo
    /// Para `--captura`: sen ScrollView, que ImageRenderer non debuxa.
    var captura = false

    var body: some View {
        VStack(spacing: 0) {
            Cabeceira()
            Divider().overlay(Cor.lina)
            if captura { lista } else { ScrollView { lista } }
            Divider().overlay(Cor.lina)
            Pe()
        }
        .background(LinearGradient(colors: [Cor.fondo1, Cor.fondo2], startPoint: .top, endPoint: .bottom))
        .task { if !captura { await m.cargar() } }
        .alert("Algo non foi ben", isPresented: Binding(get: { m.erro != nil }, set: { if !$0 { m.erro = nil } })) {
            Button("Vale") { m.erro = nil }
        } message: { Text(m.erro ?? "") }
    }

    private var lista: some View {
                VStack(alignment: .leading, spacing: 10) {
                    if m.catalogo == nil {
                        HStack { Spacer(); ProgressView().controlSize(.small); Text("Buscando os plugins…").foregroundColor(Cor.crema2); Spacer() }
                            .padding(.top, 60)
                    }
                    ForEach(seccions, id: \.0) { titulo, lista in
                        Text(titulo.uppercased())
                            .font(.system(size: 11, weight: .semibold)).tracking(1.6)
                            .foregroundColor(Cor.crema3)
                            .padding(.top, 10).padding(.leading, 4)
                        ForEach(lista) { p in Tarxeta(produto: p) }
                    }
                    if !m.orfos.isEmpty {
                        Text("OUTROS DE ESCOITA INSTALADOS")
                            .font(.system(size: 11, weight: .semibold)).tracking(1.6)
                            .foregroundColor(Cor.crema3).padding(.top, 10).padding(.leading, 4)
                        ForEach(m.orfos, id: \.url) { i in Orfo(instalado: i) }
                    }
                }
                .padding(16)
    }

    private var seccions: [(String, [Produto])] {
        var r: [(String, [Produto])] = []
        let publicados = m.produtos.filter { $0.repo != nil }
        for t in [TipoProduto.instrumento, .efecto, .paquete, .descoñecido] {
            let l = publicados.filter { $0.tipo == t }
            if !l.isEmpty { r.append((t.titulo, l)) }
        }
        let locais = m.produtos.filter { $0.repo == nil }
        if !locais.isEmpty { r.append(("Só en local (aínda sen publicar)", locais)) }
        return r
    }
}

// MARK: - cabeceira

struct Cabeceira: View {
    @EnvironmentObject var m: Modelo

    var body: some View {
        HStack(alignment: .center, spacing: 14) {
            Logo().frame(width: 34, height: 34)
            VStack(alignment: .leading, spacing: 1) {
                Text("Barullo Xestor").font(.system(size: 20, weight: .bold, design: .rounded)).foregroundColor(Cor.crema)
                Text("os plugins de Escoita").font(.system(size: 11)).foregroundColor(Cor.crema3)
            }
            Spacer()
            HStack(spacing: 4) {
                ForEach(Formato.allCases, id: \.self) { f in
                    let on = m.formatos.contains(f)
                    Button { m.cambiarFormato(f) } label: {
                        Text(f.etiqueta).font(.system(size: 11, weight: .semibold))
                            .padding(.horizontal, 9).padding(.vertical, 4)
                            .foregroundColor(on ? Cor.noite : Cor.crema2)
                            .background(Capsule().fill(on ? Cor.ouro : Cor.visor))
                            .overlay(Capsule().stroke(on ? Color.clear : Cor.lina))
                    }
                    .buttonStyle(.plain)
                    .help(on ? "Mirar e instalar \(f.etiqueta) (clic para quitalo)" : "Mirar e instalar tamén \(f.etiqueta)")
                }
            }
            Button { Task { await m.cargar(forzar: true) } } label: {
                Image(systemName: "arrow.clockwise").foregroundColor(Cor.crema2)
            }
            .buttonStyle(.plain).disabled(m.cargando).help("Preguntar a GitHub de novo")
            let n = m.porActualizar.count
            Button(n > 0 ? "Actualizar todo (\(n))" : "Todo ao día") { Task { await m.actualizarTodo() } }
                .buttonStyle(Boton(principal: n > 0))
                .disabled(n == 0 || !m.traballos.isEmpty)
        }
        .padding(.horizontal, 16).padding(.vertical, 12)
        .background(LinearGradient(colors: [Cor.panel1, Cor.panel2], startPoint: .top, endPoint: .bottom))
    }
}

/// Un mando coma os dos plugins.
struct Logo: View {
    var body: some View {
        ZStack {
            Circle().fill(RadialGradient(colors: [Cor.c(0x4a5577), Cor.c(0x232b45), Cor.c(0x0e1323)],
                                         center: UnitPoint(x: 0.35, y: 0.3), startRadius: 1, endRadius: 22))
            Circle().stroke(Cor.laranxa, lineWidth: 2.5).padding(1.5)
            Capsule().fill(Cor.crema).frame(width: 3, height: 11).offset(y: -8).rotationEffect(.degrees(40))
        }
    }
}

// MARK: - tarxeta dun produto

struct Tarxeta: View {
    @EnvironmentObject var m: Modelo
    let produto: Produto
    @State private var confirmar = false
    @State private var aberta = false

    var body: some View {
        let s = m.situacion(produto)
        let traballo = m.traballos[produto.id]
        VStack(alignment: .leading, spacing: 8) {
            HStack(alignment: .top, spacing: 12) {
                VStack(alignment: .leading, spacing: 4) {
                    HStack(spacing: 8) {
                        Text(produto.nome).font(.system(size: 15, weight: .semibold)).foregroundColor(Cor.crema)
                        Pastilla(estado: s.estado)
                    }
                    if !produto.descricion.isEmpty {
                        Text(produto.descricion).font(.system(size: 12)).foregroundColor(Cor.crema2)
                    }
                    Copias(situacion: s)
                }
                Spacer(minLength: 8)
                VStack(alignment: .trailing, spacing: 6) {
                    Versions(produto: produto, situacion: s)
                    Botons(produto: produto, situacion: s, confirmar: $confirmar)
                        .disabled(traballo != nil)
                }
            }
            if let t = traballo {
                if let v = t { ProgressView(value: v).tint(Cor.laranxa) }
                else { ProgressView().progressViewStyle(.linear).tint(Cor.laranxa) }
            }
            if produto.eSerie {
                DisclosureGroup(isExpanded: $aberta) {
                    VStack(alignment: .leading, spacing: 3) {
                        ForEach(produto.plugins, id: \.self) { pl in
                            let c = s.relevantes.filter { $0.plugin == pl }
                            HStack {
                                Text(pl.replacingOccurrences(of: "Barullo ", with: "")).foregroundColor(Cor.crema2)
                                    .frame(width: 110, alignment: .leading)
                                Text(c.isEmpty ? "non instalado" : c.map { "\($0.formato.etiqueta) \($0.textoVersion ?? "?")" }.joined(separator: " · "))
                                    .foregroundColor(c.isEmpty ? Cor.crema3 : Cor.crema2)
                            }
                            .font(.system(size: 11, design: .monospaced))
                        }
                    }
                    .padding(.top, 4)
                } label: {
                    Text("\(produto.plugins.count) plugins dun mando").font(.system(size: 11)).foregroundColor(Cor.crema3)
                }
                .tint(Cor.crema3)
            }
        }
        .padding(14)
        .background(RoundedRectangle(cornerRadius: 10)
            .fill(LinearGradient(colors: [Cor.panel1, Cor.panel2], startPoint: .top, endPoint: .bottom)))
        .overlay(RoundedRectangle(cornerRadius: 10).stroke(Cor.lina, lineWidth: 1))
        .confirmationDialog("Levar \(produto.nome) ao Lixo?", isPresented: $confirmar, titleVisibility: .visible) {
            let usuario = s.relevantes.filter { $0.ubicacion.clase == .usuario }
            let sistema = s.relevantes.filter { $0.ubicacion.clase == .sistema && !$0.precisaAdmin }
            if !usuario.isEmpty {
                Button("Levar ao Lixo (\(m.formatos.map(\.etiqueta).joined(separator: ", ")) do usuario)", role: .destructive) {
                    Task { await m.desinstalar(produto, incluirSistema: false) }
                }
            }
            if !sistema.isEmpty {
                Button("Levar ao Lixo, tamén as de /Library", role: .destructive) {
                    Task { await m.desinstalar(produto, incluirSistema: true) }
                }
            }
            Button("Cancelar", role: .cancel) {}
        } message: {
            Text(s.relevantes.map { $0.url.path }.joined(separator: "\n")
                 + (s.relevantes.contains(where: \.precisaAdmin) ? "\n\nAs de /Library que precisan admin quedan: quítaas no Finder." : "")
                 + "\n\nVan ao Lixo, non se borran.")
        }
    }
}

struct Pastilla: View {
    let estado: Estado
    var body: some View {
        let cor: Color = {
            switch estado {
            case .alDia: return Cor.verde
            case .actualizacion, .incompleto: return Cor.laranxa
            case .copiaVella: return Cor.ouro
            case .nonInstalado, .senDatos: return Cor.crema3
            case .soLocal: return Cor.c(0x8fb0e8)
            }
        }()
        Text(estado.etiqueta)
            .font(.system(size: 10, weight: .semibold))
            .padding(.horizontal, 7).padding(.vertical, 2)
            .foregroundColor(cor)
            .background(Capsule().fill(cor.opacity(0.13)))
            .overlay(Capsule().stroke(cor.opacity(0.45), lineWidth: 1))
    }
}

/// As copias instaladas: formato, versión e onde («usuario» ou «sistema»).
struct Copias: View {
    @EnvironmentObject var m: Modelo
    let situacion: Situacion

    var body: some View {
        let s = situacion
        VStack(alignment: .leading, spacing: 2) {
            ForEach(grupos(s.relevantes), id: \.0) { clave, lista in
                HStack(spacing: 6) {
                    Image(systemName: lista[0].ubicacion.clase == .usuario ? "person" : "lock")
                        .font(.system(size: 9)).foregroundColor(Cor.crema3)
                    Text(texto(clave, lista)).font(.system(size: 11, design: .monospaced)).foregroundColor(Cor.crema2)
                    if lista.contains(where: \.precisaAdmin) {
                        Text("precisa admin").font(.system(size: 10)).foregroundColor(Cor.crema3)
                        Button("Amosar") { m.amosarNoFinder(lista.map(\.url)) }
                            .buttonStyle(.link).font(.system(size: 10))
                    }
                }
            }
            let outros = s.instalados.filter { !m.formatos.contains($0.formato) }
            if !outros.isEmpty {
                Text("tamén: " + grupos(outros).map { texto($0.0, $0.1) }.joined(separator: " · "))
                    .font(.system(size: 10)).foregroundColor(Cor.crema3)
            }
            if !s.duplicados.isEmpty {
                Text("Está no cartafol do usuario e no do sistema: o DAW pode ver as dúas.")
                    .font(.system(size: 10)).foregroundColor(Cor.ouro.opacity(0.85))
            }
            if !s.faltan.isEmpty {
                Text("Faltan: " + s.faltan.map { $0.replacingOccurrences(of: "Barullo ", with: "") }.joined(separator: ", "))
                    .font(.system(size: 10)).foregroundColor(Cor.laranxa)
            }
            if let l = s.produto.rutaLocal {
                Text("repo local: " + (l.path as NSString).abbreviatingWithTildeInPath + (s.produto.versionLocal.map { " (\($0))" } ?? ""))
                    .font(.system(size: 10)).foregroundColor(Cor.crema3)
            }
        }
        .padding(.top, 2)
    }

    private func grupos(_ l: [Instalado]) -> [(String, [Instalado])] {
        Dictionary(grouping: l) { "\($0.formato.etiqueta) · \($0.ubicacion.etiqueta)" }
            .sorted { $0.key < $1.key }.map { ($0.key, $0.value) }
    }

    private func texto(_ clave: String, _ l: [Instalado]) -> String {
        let v = Set(l.map { $0.textoVersion ?? "?" }).sorted().joined(separator: "/")
        let n = situacion.produto.eSerie ? " (\(Set(l.map(\.plugin)).count)/\(situacion.produto.plugins.count))" : ""
        return "\(clave)  \(v)\(n)"
    }
}

struct Versions: View {
    let produto: Produto
    let situacion: Situacion
    var body: some View {
        VStack(alignment: .trailing, spacing: 1) {
            if let v = produto.ultima?.version {
                Text("v\(v.description)").font(.system(size: 15, weight: .semibold, design: .monospaced)).foregroundColor(Cor.ouro)
                Text(produto.ultima?.publicado.map { "publicada o " + DateFormatter.localizedString(from: $0, dateStyle: .medium, timeStyle: .none) } ?? "última publicada")
                    .font(.system(size: 9)).foregroundColor(Cor.crema3)
            } else if let v = produto.versionLocal {
                Text("\(v.description)").font(.system(size: 15, weight: .semibold, design: .monospaced)).foregroundColor(Cor.crema3)
                Text("só no repo local").font(.system(size: 9)).foregroundColor(Cor.crema3)
            } else {
                Text("—").foregroundColor(Cor.crema3)
                Text(produto.repo == nil ? "só local" : "sen versión publicada").font(.system(size: 9)).foregroundColor(Cor.crema3)
            }
        }
        .padding(.horizontal, 10).padding(.vertical, 5)
        .background(RoundedRectangle(cornerRadius: 6).fill(Cor.visor))
    }
}

struct Botons: View {
    @EnvironmentObject var m: Modelo
    let produto: Produto
    let situacion: Situacion
    @Binding var confirmar: Bool

    var body: some View {
        let e = situacion.estado
        HStack(spacing: 6) {
            if produto.ultima?.zipMac != nil {
                switch e {
                case .nonInstalado:
                    Button("Instalar") { Task { await m.instalar(produto) } }.buttonStyle(Boton(principal: true))
                case .actualizacion, .incompleto:
                    Button("Actualizar") { Task { await m.instalar(produto) } }.buttonStyle(Boton(principal: true))
                case .copiaVella:
                    let pode = situacion.vellas.contains { !$0.precisaAdmin }
                    Button("Quitar a copia vella") { Task { await m.quitarCopiasVellas(produto) } }
                        .buttonStyle(Boton(principal: true)).disabled(!pode)
                        .help(pode ? "Leva ao Lixo a copia vella de /Library" : "Esa copia precisa admin: quítaa no Finder")
                default: EmptyView()
                }
            }
            if !situacion.relevantes.isEmpty {
                Button("Desinstalar") { confirmar = true }.buttonStyle(Boton(principal: false))
            }
            if let u = produto.paxinaRepo {
                Button { m.abrir(produto.ultima?.paxina ?? u) } label: { Image(systemName: "arrow.up.right.square") }
                    .buttonStyle(Boton(principal: false)).help("Ver no GitHub")
            } else if let l = produto.rutaLocal {
                Button { m.amosarNoFinder([l]) } label: { Image(systemName: "folder") }
                    .buttonStyle(Boton(principal: false)).help("Amosar o repo local")
            }
        }
    }
}

struct Orfo: View {
    @EnvironmentObject var m: Modelo
    let instalado: Instalado
    @State private var confirmar = false
    var body: some View {
        HStack {
            Text(instalado.plugin).font(.system(size: 13, weight: .semibold)).foregroundColor(Cor.crema)
            Text("\(instalado.formato.etiqueta) · \(instalado.ubicacion.etiqueta)  \(instalado.textoVersion ?? "?")")
                .font(.system(size: 11, design: .monospaced)).foregroundColor(Cor.crema2)
            Text("sen repo coñecido").font(.system(size: 10)).foregroundColor(Cor.crema3)
            Spacer()
            Button { m.amosarNoFinder([instalado.url]) } label: { Image(systemName: "folder") }.buttonStyle(Boton(principal: false))
            Button("Desinstalar") { confirmar = true }.buttonStyle(Boton(principal: false)).disabled(instalado.precisaAdmin)
        }
        .padding(.horizontal, 14).padding(.vertical, 9)
        .background(RoundedRectangle(cornerRadius: 10).fill(Cor.panel2))
        .overlay(RoundedRectangle(cornerRadius: 10).stroke(Cor.lina, lineWidth: 1))
        .confirmationDialog("Levar \(instalado.plugin) ao Lixo?", isPresented: $confirmar) {
            Button("Levar ao Lixo", role: .destructive) { Task { await m.desinstalarOrfo(instalado) } }
            Button("Cancelar", role: .cancel) {}
        } message: { Text(instalado.url.path) }
    }
}

// MARK: - pé

struct Pe: View {
    @EnvironmentObject var m: Modelo
    var body: some View {
        HStack(spacing: 10) {
            if m.cargando { ProgressView().controlSize(.small) }
            Text(m.mensaxe).font(.system(size: 11)).foregroundColor(Cor.crema2).lineLimit(1).truncationMode(.middle)
            Spacer()
            if let r = m.restantes {
                Text("GitHub: \(r) preguntas máis nesta hora").font(.system(size: 10)).foregroundColor(Cor.crema3)
            }
        }
        .padding(.horizontal, 16).padding(.vertical, 8)
        .background(Cor.panel2)
    }
}

struct Boton: ButtonStyle {
    var principal: Bool
    @Environment(\.isEnabled) private var activo

    func makeBody(configuration c: Configuration) -> some View {
        c.label
            .font(.system(size: 12, weight: .semibold))
            .padding(.horizontal, 11).padding(.vertical, 5)
            .foregroundColor(principal ? Cor.noite : Cor.crema2)
            .background(RoundedRectangle(cornerRadius: 6).fill(principal ? Cor.laranxa : Cor.visor))
            .overlay(RoundedRectangle(cornerRadius: 6).stroke(principal ? Color.clear : Cor.lina))
            .opacity(!activo ? 0.4 : (c.isPressed ? 0.75 : 1))
    }
}

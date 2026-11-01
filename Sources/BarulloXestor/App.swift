import SwiftUI
import XestorNucleo

/// As cores dos editores dos plugins Barullo (PluginEditor.h: noite, crema, laranxa, ouro...).
enum Cor {
    static func c(_ h: UInt32) -> Color {
        Color(red: Double((h >> 16) & 0xff) / 255, green: Double((h >> 8) & 0xff) / 255, blue: Double(h & 0xff) / 255)
    }
    static let noite = c(0x0c111f)     // fondo (o da web de Escoita)
    static let fondo1 = c(0x161e33), fondo2 = c(0x0b101e)
    static let crema = c(0xfff7c1)     // texto
    static let crema2 = c(0xbdb792)    // texto secundario
    static let crema3 = c(0x857f67)    // rótulos discretos
    static let laranxa = c(0xe08b16)   // o activo
    static let ouro = c(0xedba23)      // os visores
    static let verde = c(0x9dc6a0)     // ao día
    static let panel1 = c(0x1c2540), panel2 = c(0x111830)
    static let visor = c(0x070b16)
    static let lina = c(0x2a3350)
    static let vermello = c(0xd9705a)
}

struct BarulloXestorApp: App {
    @NSApplicationDelegateAdaptor(Delegado.self) var delegado
    @StateObject private var modelo = Modelo()

    var body: some Scene {
        WindowGroup("Barullo Xestor") {
            Xanela()
                .environmentObject(modelo)
                .frame(minWidth: 760, minHeight: 520)
                .preferredColorScheme(.dark)
        }
        .defaultSize(width: 900, height: 720)
        .commands {
            CommandGroup(replacing: .newItem) {}
            CommandMenu("Plugins") {
                Button("Preguntar a GitHub de novo") { Task { await modelo.cargar(forzar: true) } }
                    .keyboardShortcut("r")
                Button("Actualizar todo") { Task { await modelo.actualizarTodo() } }
                    .keyboardShortcut("u")
                    .disabled(modelo.porActualizar.isEmpty)
                Divider()
                Button("Abrir o cartafol dos VST3") {
                    modelo.abrir(modelo.rutas.usuario.cartafol(.vst3))
                }
                Button("Abrir os axustes (axustes.json)") {
                    if !FileManager.default.fileExists(atPath: Axustes.ficheiro.path) { try? modelo.axustes.gardar() }
                    modelo.amosarNoFinder([Axustes.ficheiro])
                }
            }
        }
    }
}

final class Delegado: NSObject, NSApplicationDelegate {
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }
}

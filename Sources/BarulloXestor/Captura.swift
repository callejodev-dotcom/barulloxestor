import AppKit
import SwiftUI
import XestorNucleo

/// Debuxa a xanela nun PNG sen abrila (ImageRenderer), cos datos de verdade.
@MainActor
enum Captura {
    static func facer(_ u: URL, formatos: [Formato]?) async -> Int32 {
        let m = Modelo()
        if let f = formatos { m.axustes.formatos = f }   // só para esta captura: non se garda
        await m.cargar()
        let v = Xanela(captura: true).environmentObject(m).frame(width: 900).preferredColorScheme(.dark)
            .environment(\.colorScheme, .dark)
        let r = ImageRenderer(content: v)
        r.scale = 2
        guard let cg = r.cgImage else { FileHandle.standardError.write("Non se puido debuxar\n".data(using: .utf8)!); return 1 }
        let rep = NSBitmapImageRep(cgImage: cg)
        guard let d = rep.representation(using: .png, properties: [:]) else { return 1 }
        do { try d.write(to: u) } catch { return 1 }
        print("Captura: \(u.path) (\(cg.width)×\(cg.height))")
        return 0
    }
}

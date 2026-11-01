// Debuxa a icona (un mando coma os dos plugins sobre o azul noite) e fai Recursos/AppIcon.icns.
// Uso: swift ferramentas/icona.swift <cartafol de saída>
import AppKit

func cor(_ h: UInt32, _ a: CGFloat = 1) -> NSColor {
    NSColor(srgbRed: CGFloat((h >> 16) & 0xff) / 255, green: CGFloat((h >> 8) & 0xff) / 255, blue: CGFloat(h & 0xff) / 255, alpha: a)
}

func debuxar(_ t: Int) -> Data {
    let s = CGFloat(t)
    let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: t, pixelsHigh: t, bitsPerSample: 8, samplesPerPixel: 4,
                               hasAlpha: true, isPlanar: false, colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)!
    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
    // o fondo: o cadrado redondeado de macOS (marxe do 10 %)
    let m = s * 0.1, r = NSRect(x: m, y: m, width: s - 2 * m, height: s - 2 * m)
    let fondo = NSBezierPath(roundedRect: r, xRadius: s * 0.18, yRadius: s * 0.18)
    NSGradient(starting: cor(0x1c2540), ending: cor(0x0b101e))!.draw(in: fondo, angle: -90)
    cor(0x2a3350).setStroke(); fondo.lineWidth = s * 0.008; fondo.stroke()
    // o mando
    let c = NSPoint(x: s / 2, y: s / 2), kr = s * 0.25
    let arco = NSBezierPath()
    arco.appendArc(withCenter: c, radius: kr * 1.28, startAngle: -45, endAngle: 225)
    arco.lineWidth = s * 0.035; arco.lineCapStyle = .round
    cor(0x2a3350).setStroke(); arco.stroke()
    let activo = NSBezierPath()
    activo.appendArc(withCenter: c, radius: kr * 1.28, startAngle: 225, endAngle: 50, clockwise: true)
    activo.lineWidth = s * 0.035; activo.lineCapStyle = .round
    cor(0xe08b16).setStroke(); activo.stroke()
    let disco = NSBezierPath(ovalIn: NSRect(x: c.x - kr, y: c.y - kr, width: 2 * kr, height: 2 * kr))
    NSGradient(colors: [cor(0x4a5577), cor(0x232b45), cor(0x0e1323)], atLocations: [0, 0.55, 1], colorSpace: .sRGB)!
        .draw(in: disco, relativeCenterPosition: NSPoint(x: -0.3, y: 0.4))
    // o indicador
    let ang = 50.0 * Double.pi / 180
    let a = NSPoint(x: c.x + kr * 0.35 * CGFloat(cos(ang)), y: c.y + kr * 0.35 * CGFloat(sin(ang)))
    let b = NSPoint(x: c.x + kr * 0.85 * CGFloat(cos(ang)), y: c.y + kr * 0.85 * CGFloat(sin(ang)))
    let ind = NSBezierPath(); ind.move(to: a); ind.line(to: b)
    ind.lineWidth = s * 0.03; ind.lineCapStyle = .round
    cor(0xfff7c1).setStroke(); ind.stroke()
    NSGraphicsContext.restoreGraphicsState()
    return rep.representation(using: .png, properties: [:])!
}

let saida = URL(fileURLWithPath: CommandLine.arguments[1])
let set = saida.appendingPathComponent("AppIcon.iconset")
try? FileManager.default.createDirectory(at: set, withIntermediateDirectories: true)
for t in [16, 32, 128, 256, 512] {
    try! debuxar(t).write(to: set.appendingPathComponent("icon_\(t)x\(t).png"))
    try! debuxar(t * 2).write(to: set.appendingPathComponent("icon_\(t)x\(t)@2x.png"))
}

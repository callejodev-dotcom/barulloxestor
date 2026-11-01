// swift-tools-version:6.0
import PackageDescription

let package = Package(
    name: "BarulloXestor",
    platforms: [.macOS(.v13)],
    products: [
        .executable(name: "BarulloXestor", targets: ["BarulloXestor"]),
        .library(name: "XestorNucleo", targets: ["XestorNucleo"]),
    ],
    targets: [
        // a lóxica: descubrir, ler versións, descargar, instalar e desinstalar (sen interface)
        .target(name: "XestorNucleo"),
        // a app (SwiftUI) e o modo de liña de ordes
        .executableTarget(name: "BarulloXestor", dependencies: ["XestorNucleo"]),
        .testTarget(name: "XestorNucleoTests", dependencies: ["XestorNucleo"]),
    ],
    swiftLanguageModes: [.v5]
)

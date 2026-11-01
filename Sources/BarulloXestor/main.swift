import Foundation

// Con argumentos «--algo», modo de liña de ordes (sen xanela); se non, a app.
if CLI.eCLI(CommandLine.arguments) {
    setvbuf(stdout, nil, _IOLBF, 0)
    Task {
        let codigo = await CLI.executar(CommandLine.arguments)
        exit(codigo)
    }
    // o fío principal segue vivo e atende a cola principal (a captura de SwiftUI precisa o fío principal)
    while true { RunLoop.main.run(mode: .default, before: .distantFuture) }
} else {
    BarulloXestorApp.main()
}

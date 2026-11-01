# Barullo Xestor

Unha app de Mac (SwiftUI) para ter ao día os plugins Barullo de Escoita: que está instalado e en
que versión, cal é a última publicada, e instalar, actualizar ou desinstalar (ao Lixo, nunca se borra).

## Como sabe que plugins hai

- **GitHub**: mira os repos públicos de `callejodev-dotcom` e un repo é un plugin se ten
  `packaging/produto.env` (o ficheiro do núcleo de Escoita). De alí saen o nome, a descrición,
  o tipo (instrumento/efecto, polo código AU) e, nas series coma Enxebres, a lista de plugins.
  Un plugin novo aparece só ao publicar o repo; non hai lista no código.
- **Local**: os subcartafoles de `~/Downloads` con `packaging/produto.env` que non están en GitHub
  aparecen coma «só local» (coa versión do CMakeLists).
- **Instalado**: `~/Library/Audio/Plug-Ins/{VST3,Components,CLAP}` e `/Library/...` (e as apps
  en `~/Applications` e `/Applications`); a versión, do `CFBundleShortVersionString`.
- A última versión: `releases/latest`. Instala o `…-mac.zip` (bundles soltos), comproba tamaño e
  SHA-256 (o `digest` de GitHub), quita `com.apple.quarantine` e, se `codesign --verify --deep --strict`
  falla, asina «ad hoc». Instala sempre no cartafol do usuario (sen admin); o que substitúe vai ao Lixo.
- Caché en `~/Library/Caches/link.escoita.barulloxestor` (15 min, e logo pregunta co ETag). Se o `gh`
  ten sesión, usa o seu token (5000 preguntas/hora en vez de 60).

Axustes opcionais en `~/Library/Application Support/Barullo Xestor/axustes.json`:
`formatos`, `cartafolesLocais`, `reposExtra`, `reposOcultos`, `propietario`, `usarGh`.

## Liña de ordes

```sh
B="$HOME/Applications/Barullo Xestor.app/Contents/MacOS/BarulloXestor"
"$B" --listar                       # ou --json
"$B" --instalar tune --dir /tmp/proba --papeleira /tmp/lixo
"$B" --actualizar-todo
"$B" --axuda
```

## Compilar e probar

```sh
./probas.sh            # probas do núcleo (en cartafoles temporais)
./build.sh             # dist/Barullo Xestor.app
./build.sh --instalar  # e cópiaa a ~/Applications
```

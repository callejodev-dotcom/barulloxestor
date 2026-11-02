# Barullo Xestor

A app para ter ao día os plugins Barullo de [Escoita](https://www.escoita.link) en **Mac, Windows e Linux**:
que está instalado e en que versión, cal é a última publicada, e instalar, actualizar ou desinstalar
(ao Lixo, nunca se borra). Feita en JUCE coma os plugins, co seu mesmo aspecto (Montserrat e Inter,
o panel azul mariño, os visores de cristal, as teclas con LED) e o núcleo común `escoita-nucleo`.

## Como sabe que plugins hai

- **GitHub**: mira os repos públicos de `callejodev-dotcom`; un repo é un plugin se ten
  `packaging/produto.env` (o ficheiro do núcleo). De alí saen o nome, a descrición, o tipo
  (instrumento/efecto, polo código AU) e, nas series coma Enxebres, a lista `PLUGINS`.
  Un plugin novo aparece só ao publicar o repo; non hai lista no código.
- **Local**: os subcartafoles de `~/Downloads` con `packaging/produto.env` que non están en GitHub
  aparecen coma «só neste ordenador» (coa versión do CMakeLists).
- **Instalado**, por formato e sitio (o do usuario e o do sistema):

  | | usuario (sen admin) | sistema (con admin) |
  |---|---|---|
  | Mac | `~/Library/Audio/Plug-Ins/{VST3,Components,CLAP}`, `~/Applications` | `/Library/Audio/Plug-Ins/…`, `/Applications` |
  | Windows | `%LOCALAPPDATA%\Programs\Common\{VST3,CLAP}` | `C:\Program Files\Common Files\{VST3,CLAP}` |
  | Linux | `~/.vst3`, `~/.clap`, `~/.lv2`, `~/.local/bin` | `/usr/local/lib/…`, `/usr/lib/…` |

  A versión: `Contents/Info.plist` (Mac), `Contents/Resources/moduleinfo.json` (VST3 en todos) ou,
  para o que non a di (CLAP, apps de Windows e Linux), o rexistro do que instalou o xestor.
- **A última versión**: `releases/latest`. Instala o arquivo «á man» do sistema (`…-mac.zip`,
  `…-windows-x64.zip`, `…-linux-x86_64.tar.gz`), comproba tamaño e SHA-256 (o `digest` de GitHub)
  e, en Mac, quita `com.apple.quarantine` e asina «ad hoc» se `codesign --verify --deep --strict` falla.
- **Capturas**: as imaxes da interface, de raw.githubusercontent.com (non gastan preguntas da API):
  as de `CAPTURAS="…"` no `produto.env` (opcional), `docs/img/captura.png` e as imaxes que saian no
  `LEEME.md`, `README.md` ou `docs/MANUAL.md`, na súa orde. Sen ningunha, a tarxeta leva o nome na súa cor.
- Caché (15 min, e logo pregunta co ETag). Se o `gh` ten sesión, usa o seu token (5000 preguntas/hora en vez de 60).

## Estados

ao día · actualización · copia vella no sistema (no cartafol do usuario xa está a nova, pero queda
unha vella no do sistema que o DAW tamén ve) · incompleto (unha serie á que lle faltan plugins) ·
non instalado · só local.

## Instalar, actualizar e o permiso de administrador

Instálase **sempre no cartafol do usuario** (sen admin). O que se substitúe vai ao Lixo; nunca se
pisa unha versión máis nova sen `--forzar`. Ao actualizar, as copias máis vellas dos cartafoles
do sistema tamén van ao Lixo.

Para sacar algo dun cartafol do sistema pídese o permiso **unha vez** e o paso con privilexios é
mínimo: mover o bundle a un cartafol temporal do usuario (e devolverllo con `chown`); despois, xa
sen privilexios, vai ao Lixo coma calquera outro (así o root non toca nunca `~/.Trash`, `~/Downloads`
nin `~/Documents`, que en Mac protexe TCC, e «Volver poñer» funciona).

- Mac: `osascript` → `do shell script … with administrator privileges` (o diálogo do sistema, co motivo).
- Windows: o propio xestor con `--operacions <ficheiro>`, lanzado con `ShellExecuteEx` «runas» (UAC);
  só move desde os cartafoles do sistema ata o temporal.
- Linux: `pkexec /bin/sh -c …` (o diálogo de polkit).

Se se di que non, a copia queda onde estaba e avísase.

## Antes e despois de cambiar os plugins

- **DAW aberto**: antes de instalar, actualizar, volver atrás ou desinstalar mírase se hai un DAW
  en marcha (Ableton Live, Logic Pro, REAPER, Bitwig, Cubase, Nuendo, FL Studio, Studio One,
  Pro Tools, GarageBand, Ardour, Reason…; `ps` en Mac e Linux, `tasklist` en Windows). Se o hai,
  avísase: **agardar** a que se peche (faise só entón), **continuar igual** ou cancelar. Despois
  dun cambio, a faixa de arriba lembra volver escanear os plugins no DAW.
- **Volver atrás**: ao actualizar, a versión de antes cópiase en `<datos>/copias/<Curto>/<versión>/`
  (as dúas máis novas de cada plugin) antes de ir ao Lixo; o menú ••• ofrece «Volver á 0.5.0»,
  sen rede.
- **Notas da versión**: o texto da versión en GitHub (ou o `NOTAS-VERSION.md` da etiqueta), nun
  panel coma o das capturas, desde o menú ••• ou cun clic no visor «0.5.0 → 0.5.1».
- **Manual e presets**: «Abrir o manual» (`docs/MANUAL.md`, `LEEME.md` ou `README.md` en GitHub,
  ou no disco), o cartafol dos presets (`Documentos/<Plugin>/Presets`) e unha copia en .zip
  (`Documentos/Barullo Xestor/Copias dos presets`); opcional, antes de cada actualización.

## O propio xestor

- **Versión nova**: mira `callejodev-dotcom/barulloxestor`; «Hai Barullo Xestor X» no pé abre as
  notas e «Actualizar e reiniciar»: descarga (comprobando tamaño e SHA-256), en Mac e Linux pon a
  nova no sitio da de agora (a vella, ao Lixo) e en Windows abre o instalador en silencio; despois
  ábrese a nova.
- **Comprobar cada día**: no menú ⚙ da cabeceira, unha icona na barra de menús (Mac) ou na
  bandeja (Windows, Linux) que pregunta a GitHub unha vez ao día e avisa cunha notificación
  das versións novas (unha vez cada unha). Con ela, pechar a xanela deixa a app alí.
- **Idioma**: galego, castelán ou inglés (por defecto, o do sistema: gl → galego, es →
  castelán, se non inglés). A liña de ordes e o informe van en galego.
- **Copiar informe**: o SO, as versións do xestor e de cada plugin, as rutas, as copias
  duplicadas e (en Mac) a firma de cada bundle, para pegar ao pedir axuda (tamén `--informe`).
- **Teclado**: Tab / Maiús+Tab percorren as teclas (a lista desprázase soa), Intro preme, Esc
  pecha paneis, menús e o visor; Cmd/Ctrl+R ou F5 comproba.

## Liña de ordes

```sh
B="$HOME/Applications/Barullo Xestor.app/Contents/MacOS/Barullo Xestor"   # Windows: "Barullo Xestor.exe"; Linux: barullo-xestor
"$B" --listar                       # ou --json
"$B" --instalar tune --dir /tmp/proba --papeleira /tmp/lixo
"$B" --actualizar fm                # e a copia vella do sistema, ao Lixo (pide o permiso)
"$B" --actualizar-todo
"$B" --captura /tmp/x.png [--escena sen-rede|baleiro|traballando|ao-dia|demo|ficha|aberta|captura|sen-capturas|notas|daw|xestor|atallos] [--idioma gl|es|en] [--completa]
"$B" --informe                      # o informe para pedir axuda
"$B" --axuda
```

Axustes opcionais en `axustes.json` (Mac `~/Library/Application Support/Barullo Xestor`, Windows
`%APPDATA%\Barullo Xestor`, Linux `~/.config/Barullo Xestor`): `formatos`, `cartafolesLocais`,
`reposExtra`, `reposOcultos`, `propietario`, `usarGh`, `idioma`, `bandeja`, `copiaPresets`.

## Compilar e probar

Coma os plugins: JUCE 8.0.4 e `escoita-nucleo` nos cartafoles irmáns `../JUCE` e `../escoita-nucleo`
(ou `-DJUCE_DIR=… -DESCOITA_NUCLEO=…`).

```sh
./build.sh             # compila e pasa as probas (cartafoles temporais e un GitHub de mentira; sen rede)
./build.sh --instalar  # (Mac) e cópiaa a ~/Applications (a anterior, ao Lixo)
```

A CI (`.github/workflows/xestor.yml`) compila e proba en Mac (universal), Windows e Linux e, ao
etiquetar `v…`, publica o `.zip` de Mac (asinado coma os plugins, ou «ad hoc»), o `.zip` e o
instalador de Windows e o `.tar.gz` de Linux.

## Licenza

GNU Affero General Public License v3.0, coma os plugins. Montserrat e Inter: SIL Open Font License
(`Resources/*-OFL.txt`).

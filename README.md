# TK2000 microcomputer emulator

**About this repository**

The TK2000 emulator was created by **Fabio Belavenuto** (FBLabs), originally published at https://github.com/fbelavenuto/tk2000. This repository preserves his full history and continues the project, adding macOS support, a resizable window with a TK2000 monitor frame, a menu option to type BASIC files, 64K memory (RAM under the ROM), faster tape loading, recording the inserted tape as a WAV for a real TK2000, recording what programs save on tape, a macOS installer and the disk drive.

*In memory of Fabio Belavenuto.*

Emulates a TK2000 computer (Clone MPF-II)

This emulator was created to test my learning in C++. The required level is C++17.

I used the SDL2 graphics library to render the image and generate sound. For configuration I used a simple menu system rendering texts with a rasterized font on top of the TK2000 video itself.

# Download (macOS)

The macOS installer is on the [Releases page](https://github.com/ramurti/emulador-tk2000/releases/latest): download `TK2000-<version>.dmg`, open it and drag **TK2000** to **Applications**. It runs on Macs with Apple chips (M1 or newer) and macOS 15 or later.

The app is not signed with an Apple developer certificate, so the first time macOS refuses to open it: right-click the app and choose **Open**, or go to **System Settings > Privacy & Security** and click **Open Anyway**. After that it opens normally.

# Download (Linux)

The Linux version is also on the [Releases page](https://github.com/ramurti/emulador-tk2000/releases/latest): download `TK2000-<version>-x86_64.AppImage`. It is a single file with SDL included, so there is nothing to install. Make it executable and run it:

```sh
chmod +x TK2000-*-x86_64.AppImage
./TK2000-*-x86_64.AppImage
```

or, in the file manager, open **Properties > Permissions**, check **Allow executing file as program** and double-click it. It runs on 64-bit PCs with glibc 2.39 or newer (Ubuntu 24.04, Linux Mint 22, Debian 13, Fedora 40 or later). If it complains about FUSE, install `libfuse2` (`sudo apt install libfuse2t64` on Ubuntu 24.04 and Mint 22) or run it with `--appimage-extract-and-run`.

# Download (Windows)

The Windows version is on the [Releases page](https://github.com/ramurti/emulador-tk2000/releases/latest) too: download `TK2000-<version>-windows-x64.zip`, extract it anywhere (keep the files together) and double-click `tk2000.exe`. It runs on 64-bit Windows 10 and 11.

The program is not signed with a code-signing certificate, so the first time Windows shows "Windows protected your PC": click **More info** and then **Run anyway**. After that it opens normally.

# Usage

Download the latest release and unzip it to a folder, no installation required. To use SDL2 it is necessary that some libraries (supplied) are included with the binary.

Run the binary. To call up the menu, press F2 on the keyboard and use the up and down arrow keys to navigate. Use the ENTER key to accept the selected menu.

"Record tape (WAV)" saves the inserted tape (all its files, exactly the same blocks) as a WAV next to the `.ct2`, ready to load on a real TK2000 with `LOADT`. An existing WAV is never overwritten: the new one gets a `-2`, `-3`... suffix.

What a program saves on tape (`SAVET` in BASIC, or any program using the ROM's tape routines) is recorded: the blocks are appended to `recorded.ct2`, next to the inserted tape (or in the current directory if there is none). Insert `recorded.ct2` to load them back with `LOADT`.

"Insert disk" puts a disk image (`.dsk`, `.do`, `.po` or `.nib`, Apple II formats) in the drive of the TK2000 disk interface (the Microdigital cartridge, a copy of the Apple Disk II controller: registers at `$C090-$C09F`, boot ROM at `$C100-$C1FF` while a disk is inserted). Boot it with `CALL 49408` (`$C100`). What the TK2000 writes on the disk is saved in the image file (a read-only file is a write-protected disk). "Eject disk" removes it. A disk image can also be given on the command line instead of a tape. TKDOS 3.3 (the TK2000's DOS, from the TK2000 emulator at https://github.com/apple-II/tk2000-emulator, `Outros/TKDOS2000.dsk`) boots and reads and writes files.

"Scan lines" in the menu (or F7) cycles a light TV scan-line effect: off, the bottom part of each line of the picture 25% darker, then 35% darker, then off again. It is off when the emulator starts.

"Frame" in the menu (or F9) changes the picture around the screen: the TK2000 with its monitor (at start), the monitor and the TK2000 label only (without the keyboard), or none.

"Full screen" in the menu (or F8) fills the whole screen with the emulator, without the window title bar or the desktop panel; choose it again (or press F8) to go back to the window. On Linux, maximizing the window keeps the title bar and the panel, so use this instead.

# Development

I developed the emulator in MS Visual Studio Community 2022 (MSVC22). Make a clone of the repository and open the root directory in MSVC22 to be able to work on the sources.

## Building on macOS

Install CMake and SDL2 with Homebrew:

```sh
brew install cmake sdl2
```

From the repository root, configure and build the emulator:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Run it from the repository root so the tape browser starts in the project directory:

```sh
./build/src/tk2000
```

To create a disk image (`dist/TK2000-<version>.dmg`) with the app and SDL bundled, to share with other people:

```sh
packaging/macos/make-dmg.sh 1.1
```

The app runs on the same architecture and minimum macOS version as the Homebrew SDL libraries used to build it.

## Building on Linux

Install a C++17 compiler, CMake and the SDL2 development package:

```sh
sudo apt install g++ cmake libsdl2-dev        # Debian, Ubuntu, Mint
sudo dnf install gcc-c++ cmake SDL2-devel     # Fedora
sudo pacman -S gcc cmake sdl2                 # Arch
```

From the repository root, configure, build and run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/src/tk2000
```

The frame picture (`tk2000-frame.png`) is copied next to the binary, so the emulator can also be run from any other directory.

To create an AppImage (`dist/TK2000-<version>-x86_64.AppImage`) with SDL bundled, to share with other people (it also needs `python3-pil` and `curl`, and downloads [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) the first time):

```sh
packaging/linux/make-appimage.sh 1.1
```

The AppImage runs on distributions with the same or a newer glibc than the system used to build it, so build it on the oldest distribution you want to support.

## Building for Windows

The Windows version is cross-compiled on Linux with MinGW-w64, without a Windows PC. Install it, CMake, Pillow and zip:

```sh
sudo apt install g++-mingw-w64-x86-64 cmake python3-pil curl zip
```

Then create `dist/TK2000-<version>-windows-x64.zip` with `tk2000.exe`, `SDL2.dll` and the frame picture (it downloads the [SDL2 MinGW package](https://github.com/libsdl-org/SDL/releases) the first time):

```sh
packaging/windows/make-zip.sh 1.1
```

To test it on Linux with Wine: `wine dist/TK2000-1.1-windows-x64/tk2000.exe`.

# Known issues

There is a problem with sound generation, there may be some glitches during sound playback. This problem remains unresolved for now.



# Portuguese

Emula um computador TK2000 (Clone MPF-II)

Este emulador foi criado para testar meu aprendizado na linha C++. O nível requerido é o C++17.

Usei a biblioteca gráfica SDL2 para fazer a renderização da imagem e geração de som. Para a configuração utilizei um sistema de menu simples renderizando textos com uma fonte rasterizada em cima do próprio vídeo do TK2000.

# Download (macOS)

O instalador para macOS está na [página de Releases](https://github.com/ramurti/emulador-tk2000/releases/latest): baixe `TK2000-<versão>.dmg`, abra e arraste o **TK2000** para **Aplicativos**. Roda em Macs com chip da Apple (M1 ou mais novo) e macOS 15 ou posterior.

O app não é assinado com um certificado de desenvolvedor da Apple, então na primeira vez o macOS se recusa a abri-lo: clique com o botão direito no app e escolha **Abrir**, ou vá em **Ajustes do Sistema > Privacidade e Segurança** e clique em **Abrir Mesmo Assim**. Depois disso ele abre normalmente.

# Download (Linux)

A versão para Linux também está na [página de Releases](https://github.com/ramurti/emulador-tk2000/releases/latest): baixe `TK2000-<versão>-x86_64.AppImage`. É um único arquivo com o SDL embutido, então não há nada para instalar. Torne-o executável e execute:

```sh
chmod +x TK2000-*-x86_64.AppImage
./TK2000-*-x86_64.AppImage
```

ou, no gerenciador de arquivos, abra **Propriedades > Permissões**, marque **Permitir execução do arquivo como programa** e dê um duplo clique. Roda em PCs de 64 bits com glibc 2.39 ou mais nova (Ubuntu 24.04, Linux Mint 22, Debian 13, Fedora 40 ou posteriores). Se reclamar do FUSE, instale o `libfuse2` (`sudo apt install libfuse2t64` no Ubuntu 24.04 e no Mint 22) ou execute com `--appimage-extract-and-run`.

# Download (Windows)

A versão para Windows também está na [página de Releases](https://github.com/ramurti/emulador-tk2000/releases/latest): baixe `TK2000-<versão>-windows-x64.zip`, extraia em qualquer pasta (mantenha os arquivos juntos) e dê um duplo clique no `tk2000.exe`. Roda no Windows 10 e 11 de 64 bits.

O programa não é assinado com um certificado de assinatura de código, então na primeira vez o Windows mostra "O Windows protegeu o computador": clique em **Mais informações** e depois em **Executar assim mesmo**. Depois disso ele abre normalmente.

# Uso

Baixe o último release e descompacte em alguma pasta, não é necessário instalação. Por usar o SDL2 é necessário que algumas bibliotecas (fornecido) estejam junto ao binário.

Execute o binário. Para chamar o menu aperte F2 no teclado e use as setas direcionais para cima e para baixo para navegar. Utilize a tecla ENTER para aceitar o menu selecionado.

"Record tape (WAV)" grava a fita inserida (todos os arquivos, exatamente os mesmos blocos) num WAV ao lado do `.ct2`, pronto para carregar num TK2000 real com `LOADT`. Um WAV existente nunca é sobrescrito: o novo ganha o sufixo `-2`, `-3`...

O que um programa grava em fita (`SAVET` no BASIC, ou qualquer programa que use as rotinas de fita da ROM) é gravado: os blocos são acrescentados a `recorded.ct2`, ao lado da fita inserida (ou no diretório atual, se não houver fita). Insira `recorded.ct2` para carregá-los de volta com `LOADT`.

"Insert disk" coloca uma imagem de disco (`.dsk`, `.do`, `.po` ou `.nib`, formatos do Apple II) no drive da interface de disco do TK2000 (o cartucho da Microdigital, cópia da controladora Disk II da Apple: registradores em `$C090-$C09F`, ROM de boot em `$C100-$C1FF` enquanto há um disco inserido). Dê o boot com `CALL 49408` (`$C100`). O que o TK2000 grava no disco é salvo no arquivo da imagem (um arquivo somente leitura é um disco protegido). "Eject disk" retira o disco. Uma imagem de disco também pode ser passada na linha de comando no lugar de uma fita. O TKDOS 3.3 (o DOS do TK2000, do emulador em https://github.com/apple-II/tk2000-emulator, `Outros/TKDOS2000.dsk`) dá boot, lê e grava arquivos.

"Scan lines" no menu (ou F7) alterna um efeito leve de linhas de varredura de TV: desligado, a parte de baixo de cada linha da imagem 25% mais escura, depois 35% mais escura, depois desligado de novo. Começa desligado ao abrir o emulador.

"Frame" no menu (ou F9) muda a imagem em volta da tela: o TK2000 com o monitor (ao abrir), só o monitor e a faixa TK2000 (sem o teclado), ou nenhuma.

"Full screen" no menu (ou F8) ocupa a tela inteira com o emulador, sem a barra de título da janela nem o painel da área de trabalho; escolha de novo (ou aperte F8) para voltar à janela. No Linux, maximizar a janela mantém a barra de título e o painel, então use esta opção.

# Desenvolvimento

Desenvolvi o emulador no MS Visual Studio Community 2022 (MSVC22). Faça um clone do repositório e abra o diretório root no MSVC22 para poder trabalhar nos fontes.

## Compilação no macOS

Instale o CMake e o SDL2 pelo Homebrew:

```sh
brew install cmake sdl2
```

Na raiz do repositório, configure, compile e execute:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/src/tk2000
```

Para gerar uma imagem de disco (`dist/TK2000-<versão>.dmg`) com o app e o SDL embutidos, para enviar a outras pessoas:

```sh
packaging/macos/make-dmg.sh 1.1
```

O app roda na mesma arquitetura e versão mínima do macOS das bibliotecas SDL do Homebrew usadas na compilação.

## Compilação no Linux

Instale um compilador C++17, o CMake e o pacote de desenvolvimento do SDL2:

```sh
sudo apt install g++ cmake libsdl2-dev        # Debian, Ubuntu, Mint
sudo dnf install gcc-c++ cmake SDL2-devel     # Fedora
sudo pacman -S gcc cmake sdl2                 # Arch
```

Na raiz do repositório, configure, compile e execute:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/src/tk2000
```

A imagem da moldura (`tk2000-frame.png`) é copiada ao lado do binário, então o emulador também pode ser executado de qualquer outro diretório.

Para gerar um AppImage (`dist/TK2000-<versão>-x86_64.AppImage`) com o SDL embutido, para enviar a outras pessoas (também precisa do `python3-pil` e do `curl`, e baixa o [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) na primeira vez):

```sh
packaging/linux/make-appimage.sh 1.1
```

O AppImage roda em distribuições com a mesma glibc do sistema usado na compilação ou mais nova, então compile na distribuição mais antiga que quiser suportar.

## Compilação para Windows

A versão para Windows é compilada no Linux com o MinGW-w64 (compilação cruzada), sem precisar de um PC com Windows. Instale-o, o CMake, o Pillow e o zip:

```sh
sudo apt install g++-mingw-w64-x86-64 cmake python3-pil curl zip
```

Depois gere o `dist/TK2000-<versão>-windows-x64.zip` com o `tk2000.exe`, o `SDL2.dll` e a imagem da moldura (baixa o [pacote MinGW do SDL2](https://github.com/libsdl-org/SDL/releases) na primeira vez):

```sh
packaging/windows/make-zip.sh 1.1
```

Para testar no Linux com o Wine: `wine dist/TK2000-1.1-windows-x64/tk2000.exe`.

# Problemas conhecidos

Há um problema com a geração do som, pode haver alguns glitches durante a reprodução do som. Este problema continua sem solução por enquanto.

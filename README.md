# TK2000 microcomputer emulator

**About this repository**

The TK2000 emulator was created by **Fabio Belavenuto** (FBLabs), originally published at https://github.com/fbelavenuto/tk2000. This repository preserves his full history and continues the project, adding macOS support, a resizable window with a TK2000 monitor frame, a menu option to type BASIC files, 64K memory (RAM under the ROM), faster tape loading and a macOS installer.

*In memory of Fabio Belavenuto.*

Emulates a TK2000 computer (Clone MPF-II)

This emulator was created to test my learning in C++. The required level is C++17.

I used the SDL2 graphics library to render the image and generate sound. For configuration I used a simple menu system rendering texts with a rasterized font on top of the TK2000 video itself.

# Usage

Download the latest release and unzip it to a folder, no installation required. To use SDL2 it is necessary that some libraries (supplied) are included with the binary.

Run the binary. To call up the menu, press F2 on the keyboard and use the up and down arrow keys to navigate. Use the ENTER key to accept the selected menu.

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
packaging/macos/make-dmg.sh 1.0
```

The app runs on the same architecture and minimum macOS version as the Homebrew SDL libraries used to build it.

# Known issues

There is a problem with sound generation, there may be some glitches during sound playback. This problem remains unresolved for now.



# Portuguese

Emula um computador TK2000 (Clone MPF-II)

Este emulador foi criado para testar meu aprendizado na linha C++. O nível requerido é o C++17.

Usei a biblioteca gráfica SDL2 para fazer a renderização da imagem e geração de som. Para a configuração utilizei um sistema de menu simples renderizando textos com uma fonte rasterizada em cima do próprio vídeo do TK2000.

# Uso

Baixe o último release e descompacte em alguma pasta, não é necessário instalação. Por usar o SDL2 é necessário que algumas bibliotecas (fornecido) estejam junto ao binário.

Execute o binário. Para chamar o menu aperte F2 no teclado e use as setas direcionais para cima e para baixo para navegar. Utilize a tecla ENTER para aceitar o menu selecionado.

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
packaging/macos/make-dmg.sh 1.0
```

O app roda na mesma arquitetura e versão mínima do macOS das bibliotecas SDL do Homebrew usadas na compilação.

# Problemas conhecidos

Há um problema com a geração do som, pode haver alguns glitches durante a reprodução do som. Este problema continua sem solução por enquanto.

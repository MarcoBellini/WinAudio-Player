# WinAudio

![WinAudio_Screen2](https://github.com/MarcoBellini/WinAudio-Player/assets/66796124/fc62a3a3-9d23-4692-9ee6-781822558b2c)

WinAudio is a lightweight audio program developed in C and Win32, designed to cater to both audio enthusiasts and programming aficionados. The project is a result of my passion for computers and programming, developed during my spare time after work.

**Note: The project is currently in an alpha phase and under active development.**

## ⭐ Leave a Star

If you find WinAudio useful or interesting, please consider leaving a star! Your support lets me know that the project is appreciated, and it encourages further development.

## Features

- **Lightweight**: WinAudio is designed to be a lightweight audio program, ensuring a smooth and efficient user experience.

- **Extensible Architecture**: The program is built with an extensible architecture that allows easy integration of plugins. This enables users to expand the functionality according to their requirements.

- **Dark Mode Support**: WinAudio supports a Dark Mode theme, achieved through the use of undocumented Windows APIs. This enhances the visual experience of the application, especially in low-light environments.

- **Target Platform**: The project is developed to be opened in Visual Studio 2022 and targets Windows 10 and later versions.

## Getting Started

To get started with WinAudio, follow these steps:

1. Clone the repository: `git clone https://github.com/MarcoBellini/WinAudio-Player.git`
2. Download and install [CMake](https://cmake.org/download), if you haven't already.
3. Download the [mpg123 library source code](https://www.mpg123.de/) and extract it.
4. Create a build directory within the mpg123 source directory.
5. Generate the build files using CMake, specifying the appropriate generator for your system
6. Build the mpg123 library using the generated build files.
7. Copy the compiled mpg123 library and headers to the WinAudio project directory.
8. Download the [libsndfile library](https://libsndfile.github.io/libsndfile/) and extract it.
9. Link the static library `sndfile.lib` to the WinAudio_WaveForm project to enable support for WAV, AIFF, OGG, FLAC, and OPUS file formats.
10. Open the WinAudio project in Visual Studio 2022 and link libraries.
11. Build and run the application.

## Contributing

Contributions to WinAudio are more than welcome! If you're interested in enhancing the program, consider the following steps:

1. Fork the repository.
2. Create a new branch for your feature or improvement.
3. Make your changes and commit them.
4. Push your changes to your forked repository.
5. Open a pull request, and we'll review your contribution.

## License

This project is licensed under the [MIT License](LICENSE), allowing you to use, modify, and distribute the code as per the terms specified.

<meta name="google-site-verification" content="L7SwBgfD1TgnFhcHs2WoDIhnhsIWzb7FVuOH_aBUSqk" />

Happy coding!

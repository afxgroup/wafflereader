# Arduino Floppy Disk Reader (for AmigaOS4)

This project allows you to read floppy disks using an Arduino. It supports various disk formats and provides a simple interface for reading and writing data.

## Features

- Read and write floppy disks
- Supports multiple disk formats
- Simple and easy-to-use interface
- **Multi-language support** with AmigaOS catalog system (Italian, German, French, Dutch, Greek, Spanish, Polish)

## Requirements

- Arduino board
- Floppy disk drive
- Connecting cables

## Board project

More information about Arduino board and how to install and configure the firmware can be found [here](https://github.com/RobSmithDev/ArduinoFloppyDiskReader)

## Installation

### Installation on AmigaOS4

1. Clone the required libraries:
    ```sh
    git clone https://github.com/afxgroup/libftdi-0.20.git
    git clone https://github.com/afxgroup/libusb.git
    ```
2. Build and install the libraries following their respective instructions.

### Building the Project

1. Clone the main repository:
    ```sh
    git clone https://github.com/afxgroup/wafflereader.git
    ```
2. Navigate to the project directory:
    ```sh
    cd wafflereader
    ```
3. Use the provided Makefile to build the project. Specify `GUI=3D` if you want to create the Raylib GUI version:
    ```sh
    make GUI=3D
    ```  
    
    Specify `GUI=REACTION` if you want to create the Reaction GUI version:
    ```sh
    make GUI=REACTION 
    ```  

    Or, to build the command-line version without GUI:
    ```sh
    make
    ```

### Building Localization Catalogs

The project includes support for multiple languages through AmigaOS catalog files. To compile the catalog files:

1. Ensure you have `flexcat` installed on your system
2. Build all language catalogs:
    ```sh
    make catalogs
    ```

This will generate binary catalog files for all supported languages:
    - Italian (Locale/italian/waffle.catalog)
    - German (Locale/german/waffle.catalog)
    - French (Locale/french/waffle.catalog)
    - Dutch (Locale/dutch/waffle.catalog)
    - Greek (Locale/greek/waffle.catalog)
    - Spanish (Locale/spanish/waffle.catalog)
    - Polish (Locale/polish/waffle.catalog)

The catalogs are automatically used based on the system's locale settings. If a catalog is not available for the current locale, the program will fall back to English.

To clean up generated files including catalogs:
    ```sh
    make clean
    ```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements

- Original project by Rob Smith
- AmigaOS4 port and GUI by afxgroup (Andrea Palmatè)
- Raylib, libftdi, and libusb libraries ported on AmigaOS4 by afxgroup (Andrea Palmatè)

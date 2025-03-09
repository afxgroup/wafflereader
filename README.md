# Arduino Floppy Disk Reader (for AmigaOS4)

This project allows you to read floppy disks using an Arduino. It supports various disk formats and provides a simple interface for reading and writing data.

## Features

- Read and write floppy disks
- Supports multiple disk formats
- Simple and easy-to-use interface

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
3. Use the provided Makefile to build the project. Specify `GUI=TRUE` if you want to create the GUI version:
    ```sh
    make GUI=TRUE
    ```
    Or, to build the command-line version without GUI:
    ```sh
    make
    ```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements

- Original project by Rob Smith
- AmigaOS4 port and GUI by afxgroup (Andrea Palmatè)
- Raylib, libftdi, and libusb libraries ported on AmigaOS4 by afxgroup (Andrea Palmatè)

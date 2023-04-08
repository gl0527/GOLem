# GOLem

This is a C project that simulates Game of Life on an SDL texture.

## Dependencies

Install the following dependencies:
* SDL2
* SDL2_image
* OpenMP

## Installation

Execute the following command on top level:
```sh
$ make
```

## Usage

### Start

To run the debug version:
```sh
$ ./gol_dbg <path/to/8rgba-image>
```

To run the release version:
```sh
$ ./gol_rel <path/to/8rgba-image>
```

### Controls

* Use `space` to start/stop the simulation.
* Use `left mouse button` while moving the mouse to drag the texture (you have to click onto the texture).
* Use `mouse scroll up` to zoom into the texture.
* Use `mouse scoll down` to zoom out from the texture.
* Use `middle mouse button` to reset zoom.

## License

[MIT](LICENSE.md)

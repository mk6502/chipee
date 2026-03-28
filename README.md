# Chipee
A simple CHIP-8 emulator meant to be used as a learning tool for others.

# Goals

Chipee's goals are:

* To be a learning tool for others.
* Simplicity.
* Code readability.

# Building

## macOS

On macOS (including macOS 26), install the `sdl2` package from Homebrew.

Then, run `make`.

## Ubuntu

To build using Ubuntu (18.04 and 20.04 have been tested), install the `libsdl2-dev` package.

Then, run `make`.

## Dockerfile

Another option is to use the included Dockerfile. This image will have build prerequisites installed.
You will still need `libsdl2-dev` installed to run Chipee.

To build the Docker image, run:

    docker build --rm -f "Dockerfile" -t chipee:latest .

To execute `make` inside a container made from this image, run something like:

    docker run --rm -v `pwd`:/app chipee make

This will place a `chipee` executable in the current working directory.

# Running

Run the emulator with:

    ./chipee rom.ch8

# Testing

Build and run the test suite by running:

    make test

# Controls

The CHIP-8 keypad is a 16-key hexadecimal layout mapped to the keyboard as follows:

| CHIP-8 keypad | Keyboard |
|:---:|:---:|
| `1` `2` `3` `C` | `1` `2` `3` `4` |
| `4` `5` `6` `D` | `Q` `W` `E` `R` |
| `7` `8` `9` `E` | `A` `S` `D` `F` |
| `A` `0` `B` `F` | `Z` `X` `C` `V` |

Press `Esc` to quit.

# FAQ

Q: Why C? \
A: Simplicity.

Q: Does it support Super CHIP-8? \
A: No.

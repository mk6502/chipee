# Chipee
Adventures in CHIP-8.

# Goals

Chipee's goals are:

* Simplicity.
* Code readability.
* To be a learning tool for others.

# Building

## macOS

On macOS, install the `sdl2` package from Homebrew.

Then, run `make`.

## Ubuntu

To build using Ubuntu, install the `libsdl2-dev` package.

Then, run `make`.

## Dockerfile

Another option is to use the included Dockerfile. This image will have build prerequisites installed. You will still need `libsdl2-dev` installed to run Chipee.

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

# FAQ

Q: Why C?
A: Simplicity.

Q: Does it support Super CHIP-8?
A: No.

Q: Sound?
A: Not yet!

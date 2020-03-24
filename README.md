# On Simulating Neural Damage in Connectionist Networks

## Citation

Please cite the journal article that goes with this code, if you use any part of
this in your own work:
> Guest, O., Caso A. & Cooper, R. P. (2020). On Simulating Neural Damage in
Connectionist Networks.

## Requirements
You need to have a C compiler
([gcc](https://en.wikipedia.org/wiki/GNU_Compiler_Collection)) installed. This
comes installed by default on Linux. On Mac you need to install
[Xcode](https://apps.apple.com/in/app/xcode/id497799835). On Windows you
probably need [Cygwin](https://www.cygwin.com/) — although this has not been
tested.

### Linux
Install gtk using your package manager, e.g., for Debian-based systems:
```bash
sudo apt-get install gtk+2.0
```

### Mac
Install [Xcode](https://apps.apple.com/in/app/xcode/id497799835) and
[homebrew](https://brew.sh/) and then use it to install gtk:
```bash
brew install gtk+
```

## Compilation
To compile each model run the following in its respective directory, e.g.:
```bash
cd RogersEtal2004
make all
```

## Execution
Refer to each ```README.md``` file in each directory for instructions on how to
run each model.

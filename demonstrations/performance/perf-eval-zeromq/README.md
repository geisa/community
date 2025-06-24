# geisa-messaging-benchmarking
Contains work meant to evaluate messaging options for GEISA

## Intro

This project contains a few rust programs that are meant to provide some analysis for helping make technical decisions


## How to run locally
You'll need to have rust and cargo installed to easily run/debug locally.  Once you do:

To run zmqsub:
```
cd zmqsub
cargo run
```
To see command line options
```
cargo run -- --help
```
Ex: to enable latency analysis
```
cargo run -- -e
```

To run zmqpub:
```
cd zmqpub
cargo run
```

## How to target RPi Zero (original):
First, see:
https://rust-lang.github.io/rustup/cross-compilation.html


For rpi zero 2:
```
rustup target add aarch64-unknown-linux-gnu
```

Then there are some build chain dependencies that you'll need to install locally if you don't want to use the cross project (see below).  Once the build chain dependencies are installed, you should be able to do this:

```
cargo build --target aarch64-unknown-linux-gnu --release
```


I was able to cross compile directly for rpi zero 2, but not for the rpi zero (original).   I tried every possible thing under the sun almost.   Then, turned to the cross project, which worked.  
https://github.com/cross-rs/cross

Once installed and working, do this for each program:
```
cross build --release --target=arm-unknown-linux-gnueabihf
```

Then scp the binaries to your rpi zero


## How to use zmqsub and zmqpub

They each have prompts that let you choose input values
Basically zmqpub sends data to zmqsub

The data contains timestamps that allow for calculating throughput and latency
There is no multithreading or anything fancy.   Just one program sending messages to another.

To run the geisa zeromq benchmark programs:

1. SCP both zmqsub and zmqpub to any location on the rpi zero.  Examples:

```
scp /home/rick/Projects/geisa-messaging-benchmarking/zmqpub/target/aarch64-unknown-linux-gnu/release/zmqpub pi@10.112.1.154:~/Projects/zmqpub
scp /home/rick/Projects/geisa-messaging-benchmarking/zmqsub/target/aarch64-unknown-linux-gnu/release/zmqsub pi@10.112.1.154:~/Projects/zmqsub
```

You may need to chmod them:
```
chmod +x zmqsub
chmod +x zmqpub
```

2. To see command line options::
```
./zmqsub --help
./zmqpub --help
```

3. Start the subscriber program first:
```
./zmqsub -e
```


3. Then start the publisher program:
```
./zmqpub
```

4. When it's done, zmqsub will spit out an analysis.

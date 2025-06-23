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

2. Start the subscriber program first:
```
./zmqsub
```

        Choose the location for the socket (default seems to work every time)


3. Then start the publisher program:
```
./zmqpub
```

        Choose the same location for the socket

        Then choose the time period between messages.   The value 16,667 is basically the number of microseconds that gives us 60 messages per second.  (One message per cycle, basically)

        Then choose the message size in bytes (1525 for 128samplesX6channelsX16bit, etc)

        Then choose the number of messages to send.  Enough to watch the processor a while.  Not too much so you have to sit there for ever.


4. When it's done, zmqsub will spit out an analysis.
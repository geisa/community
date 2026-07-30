use std::{thread, time};

use chrono::{DateTime, Utc}; // 0.4.15
use std::time::SystemTime;
use serde::{Deserialize, Serialize};
use serde_json::Result;
use clap::Parser;
use zmq::{Context, PUB};


#[derive(Serialize, Deserialize, Debug, Clone)]
struct Message {
    message_type: String,
    timestamp_micros: i64,
    number_of_messages: u32,
    id: u32,
    content: String,
}

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    /// The location of the ZeroMQ socket to bind to
    #[arg(short, long, default_value = "/tmp/zmqpub0.sock")]
    socket_location: String,

    /// The period in microseconds between messages
    #[arg(short, long, default_value = "16667")]
    period: u64,

    /// The size of each message in bytes
    #[arg(short, long, default_value = "5000")]
    message_size: usize,

    /// The number of messages to send
    #[arg(short, long, default_value = "100")]
    number_of_messages: u32,
}


fn main() {
    let args: Args = Args::parse();

    println!("Socket location: {}", args.socket_location);
    println!("Period: {} microseconds", args.period);
    println!("Message size: {} bytes", args.message_size);
    println!("Number of messages: {}", args.number_of_messages);

    // Call the benchmark logic with the parsed arguments
    benchmark_logic(
        args.socket_location, 
        args.period, 
        args.message_size, 
        args.number_of_messages)
        .expect("Failed to run benchmark logic");
}


fn benchmark_logic(socket_location: String, period: u64, message_size: usize, number_of_messages: u32) -> Result<()> {
    let ctx = Context::new();
    let socket = ctx.socket(PUB).expect("Failed to create socket");
    
    let socketuri = format!("ipc://{}", socket_location);
    socket.bind(&socketuri).expect("Failed to bind socket");

    let content: String = "0".repeat(message_size);
    let message_delay = time::Duration::from_micros(period);

    // Send a few messages to allow the subscriber to connect
    let num_startup_messages: u32 = 5;
    let startup_delay = time::Duration::from_millis(100);
    for i in 0..num_startup_messages {
        thread::sleep(startup_delay);
        let message = Message {
            // startup message
            message_type: String::from("startup"),
            timestamp_micros: 0,
            number_of_messages: num_startup_messages,
            id: i as u32,
            content: content.clone(),
        };
        println!("Sending startup message {}", i);
        let serialized = serde_json::to_string(&message)?;
        socket.send(&serialized, 0).expect("Failed to send message");
    }

    // Now send the actual messages
    println!("Sending {} messages", number_of_messages);
    for i in 0..number_of_messages {
        thread::sleep(message_delay);

        let message_type: String;
        if i == 0 {
            // First message
            message_type = String::from("first");
        } else if i == number_of_messages - 1 {
            // Last message
            message_type = String::from("last");
        } else {
            // Intermediate message
            message_type = String::from("regular");
        }

        let now: SystemTime = SystemTime::now();
        let now: DateTime<Utc> = now.into();
        let now_micros: i64 = now.timestamp_micros();
        
        let message = Message {
            message_type: message_type,
            timestamp_micros: now_micros,
            number_of_messages: number_of_messages as u32,
            id: i as u32,
            content: content.clone(),
        };

        let serialized = serde_json::to_string(&message)?;
        socket.send(&serialized, 0).expect("Failed to send message");
    }
    println!("Sent {} messages", number_of_messages);
    Ok(())
}
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_benchmark_logic() {
        let socket_location = "/tmp/zmqpub0.sock".to_string();
        let period = 16_667;
        let message_size = 5_000;
        let number_of_messages = 100;

        let result = benchmark_logic(socket_location, period, message_size, number_of_messages);
        assert!(result.is_ok());
    }
}
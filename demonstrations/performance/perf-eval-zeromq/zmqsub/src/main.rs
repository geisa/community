
use chrono::{DateTime, Utc}; // 0.4.15
use std::time::SystemTime;
use serde::{Deserialize, Serialize};
use serde_json::Result;
use clap::Parser;

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
    /// The location of the ZeroMQ socket to connect to
    #[arg(short, long, default_value = "/tmp/zmqpub0.sock")]
    socket_location: String,

    /// Enable latency measurement
    #[arg(short, long, default_value = "false")]
    enable_latency_measurement: bool,
}

fn main() -> Result<()> {
    let args = Args::parse();

    println!("Socket location: {}", args.socket_location);
    println!("Enable latency measurement: {}", args.enable_latency_measurement);
    // Call the benchmark logic with the parsed arguments

    benchmark_logic(
        args.socket_location, 
        args.enable_latency_measurement)
        .expect("Failed to run benchmark logic");
    Ok(())
}


fn benchmark_logic(socket_location: String, enable_latency_measurement: bool) -> Result<()> {
    let ctx: zmq::Context = zmq::Context::new();
    let socket: zmq::Socket = ctx.socket(zmq::SUB).expect("Failed to create socket");

    let socketuri: String = format!("ipc://{}", socket_location);
    socket.connect(&socketuri).expect("Failed to connect socket");
    socket.set_subscribe(b"").expect("Failed to subscribe");

    // non mutable variables
    let number_of_messages: u32;
    let message_size: u32;
    let analysis_end_dt: DateTime<Utc>;

    // mutable variables
    let mut message_count: u32 = 0;
    let mut analysis_begin_dt: DateTime<Utc> = DateTime::from_timestamp(0, 0).expect("Failed to create DateTime from 0");

    // vectors
    let mut latency_measurements: Vec<i64> = Vec::new();

    println!("Waiting for messages...");

    loop {
        let serialized_zeromq_message: zmq::Message = socket.recv_msg(0).expect("Failed to receive message");
        let serialized_message = serialized_zeromq_message.as_str().expect("Failed to convert message to string");
        
        let deserialized_message: Message = serde_json::from_str(&serialized_message)?;

        match deserialized_message.message_type.as_str() {
            "startup" => {
                // startup message
                println!("Received startup message {}", deserialized_message.id);
            }
            "first" => {
                // First message received
                println!("Received first message {}", deserialized_message.id);
                message_count += 1;
                analysis_begin_dt = SystemTime::now().into();
            }
            "last" => {
                // Last message received
                println!("Received last message {}", deserialized_message.id);
                message_count += 1;
                number_of_messages = deserialized_message.number_of_messages;
                message_size = deserialized_message.content.len() as u32;
                analysis_end_dt = SystemTime::now().into();
                break;  // Magical compiler.  Break makes the variables not need to be mutable.
            }
            "regular" => {
                // Body message received
                message_count += 1;
                if enable_latency_measurement {
                    // // Compute latency.  Seems kind of expensive on a tiny device where we're taking precice measurements, so make it optional.
                    let new_now = SystemTime::now();
                    let new_now: DateTime<Utc> = new_now.into();
                    let parsed_timestamp_micros = deserialized_message.timestamp_micros;
                    let latency: i64 = new_now.timestamp_micros() - parsed_timestamp_micros;
                    latency_measurements.push(latency);
                }
            }
            _ => {
                // Unknown message type.  Copilot autogen came up with this - meh.
                println!("Received unknown message type {}", deserialized_message.id);
            }
        }
    }

    let analysis_duration: i64 = analysis_end_dt.timestamp_micros() - analysis_begin_dt.timestamp_micros();
    let total_data_bytes = message_count * message_size;
    let throughput_bytes_per_second = (message_count * message_size) as f64 / (analysis_duration as f64 / 1_000_000.0);
    let throughput_messages_per_second = message_count as f64 / (analysis_duration as f64 / 1_000_000.0);  



    println!("Analysis: ");

    println!("  Number of Messages Sent: {}", number_of_messages);
    println!("  Number of Messages Received: {}", message_count);
    println!("  Time from first to last message received: {} microseconds", analysis_duration);
    println!("  Message Size: {} bytes", message_size);
    println!("  Total Data Bytes: {} bytes", total_data_bytes);
    println!("  Throughput: {} bytes/second", throughput_bytes_per_second);
    println!("  Throughput: {} messages/second", throughput_messages_per_second);

    if enable_latency_measurement {
        let total_latency = latency_measurements.iter().sum::<i64>();
        let avg_latency = total_latency / (latency_measurements.len() as i64);

        println!("  Total Latency: {} microseconds", total_latency);
        println!("  Average Latency: {} microseconds", avg_latency);

    } else {
        println!("  Latency Measurement is disabled.");
    }






    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_benchmark_logic() {
        let socket_location = "/tmp/zmqpub0.sock".to_string();
        let enable_latency_measurement = true;
        let result = benchmark_logic(socket_location, enable_latency_measurement);
        assert!(result.is_ok());
    }
}
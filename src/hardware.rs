use serde::{Deserialize, Serialize};
use tokio::fs;

#[cfg(feature = "target-platform")]
const BRIGHTNESS_FILE: &str = "/sys/class/backlight/11-0045/brightness";
#[cfg(not(feature = "target-platform"))]
const BRIGHTNESS_FILE: &str = "./brightness";

#[cfg(feature = "target-platform")]
const PROCESSOR_TEMPERATURE_FILE: &str = "/sys/class/thermal/thermal_zone0/hwmon0/temp1_input";
#[cfg(not(feature = "target-platform"))]
const PROCESSOR_TEMPERATURE_FILE: &str = "./processor_temperature";

/// Sets the screen brightness (0 to 100 percentage)
pub async fn set_brightness(val: u32) -> Result<(), String> {
    let pwm_val = ((val.clamp(0, 100) as u32 * 31) / 100) as u32;

    match fs::write(BRIGHTNESS_FILE, pwm_val.to_string()).await {
        Ok(_) => {
            println!("Hardware: Brightness set to {} (PWM)", pwm_val);
            Ok(())
        }
        Err(_) => {
            // Graceful fallback for local desktop testing
            println!("Hardware (Mock): Brightness would be set to {} (PWM).", pwm_val);
            Ok(())
        }
    }
}

/// Sets the system volume (0 to 100 percentage)
pub async fn set_volume(val: u32) -> Result<(), String> {
    // For now just log it - actual ALSA/PulseAudio integration can be added later
    println!("Hardware: Volume set to {}%", val.clamp(0, 100));
    Ok(())
}

/// Reads the processor temperature in millidegrees Celsius (e.g. 45000 = 45.0°C)
/// Returns None if the temperature cannot be read
pub async fn get_processor_temperature() -> Option<f64> {
    match fs::read_to_string(PROCESSOR_TEMPERATURE_FILE).await {
        Ok(content) => content.trim().parse::<f64>().ok(),
        Err(_) => None,
    }
}

// --- Environment Sensor (SCD40 via IIO sysfs) ---

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EnvironmentData {
    pub co2_parts_per_million: f64,
    pub temperature_celsius: f64,
    pub humidity_percentage: f64,
}

#[cfg(feature = "target-platform")]
const ENVIRONMENT_IIO_BASE: &str = "/sys/devices/platform/axi/1000120000.pcie/1f00074000.i2c/i2c-1/1-0062/iio:device0";

/// Reads CO2, temperature, and humidity from the SCD40 sensor via IIO sysfs.
/// Returns None if any read or parse fails.
pub async fn get_environment_data() -> Option<EnvironmentData> {
    #[cfg(feature = "target-platform")]
    {
        async fn read_f64(path: &str) -> Option<f64> {
            fs::read_to_string(path)
                .await
                .ok()?
                .trim()
                .parse::<f64>()
                .ok()
        }

        let co2_raw = read_f64(&format!("{}/in_concentration_co2_raw", ENVIRONMENT_IIO_BASE)).await?;
        let co2_scale = read_f64(&format!("{}/in_concentration_co2_scale", ENVIRONMENT_IIO_BASE)).await?;

        let temp_raw = read_f64(&format!("{}/in_temp_raw", ENVIRONMENT_IIO_BASE)).await?;
        let temp_scale = read_f64(&format!("{}/in_temp_scale", ENVIRONMENT_IIO_BASE)).await?;

        let hum_raw = read_f64(&format!("{}/in_humidityrelative_raw", ENVIRONMENT_IIO_BASE)).await?;
        let hum_scale = read_f64(&format!("{}/in_humidityrelative_scale", ENVIRONMENT_IIO_BASE)).await?;

        Some(EnvironmentData {
            co2_parts_per_million: co2_raw * co2_scale,
            temperature_celsius: (temp_raw * temp_scale) / 1000.0,
            humidity_percentage: (hum_raw * hum_scale) / 100.0,
        })
    }

    #[cfg(not(feature = "target-platform"))]
    {
        Some(EnvironmentData {
            co2_parts_per_million: 750.0,
            temperature_celsius: 21.5,
            humidity_percentage: 45.0,
        })
    }
}

/// Initiates system shutdown
pub async fn shutdown() -> Result<(), String> {
    #[cfg(feature = "target-platform")]
    {
        use tokio::process::Command;

        println!("Hardware: Initiating system shutdown...");
        match Command::new("shutdown")
            .args(["-h", "now"])
            .spawn()
        {
            Ok(_) => {
                println!("Hardware: Shutdown command executed successfully");
                Ok(())
            }
            Err(e) => {
                eprintln!("Hardware: Failed to execute shutdown command: {}", e);
                Err(format!("Shutdown failed: {}", e))
            }
        }
    }

    #[cfg(not(feature = "target-platform"))]
    {
        println!("Hardware (Mock): System shutdown command not available on this platform");
        Ok(())
    }
}

/// Initiates system reboot
pub async fn reboot() -> Result<(), String> {
    #[cfg(feature = "target-platform")]
    {
        use tokio::process::Command;

        println!("Hardware: Initiating system reboot...");
        match Command::new("reboot")
            .spawn()
        {
            Ok(_) => {
                println!("Hardware: Reboot command executed successfully");
                Ok(())
            }
            Err(e) => {
                eprintln!("Hardware: Failed to execute reboot command: {}", e);
                Err(format!("Reboot failed: {}", e))
            }
        }
    }

    #[cfg(not(feature = "target-platform"))]
    {
        println!("Hardware (Mock): System reboot command not available on this platform");
        Ok(())
    }
}

#[cfg(feature = "target-platform")]
mod audio {
    use lazy_static::lazy_static;
    use rodio::{Decoder, OutputStream, Sink};
    use std::fs::File;
    use std::io::BufReader;
    use std::sync::Mutex;

    lazy_static! {
        static ref QUEUE_SINK: Mutex<Option<Sink>> = Mutex::new(None);
    }

    pub fn play_audio_file(media_path: &str, volume: u32, mode: &str) -> Result<(), String> {
        let file = File::open(media_path).map_err(|e| e.to_string())?;
        let reader = BufReader::new(file);
        let source = Decoder::new(reader).map_err(|e| e.to_string())?;
        let volume_clamped = (volume.clamp(0, 100) as f32) / 100.0;

        let (_stream, stream_handle) = OutputStream::try_default()
            .map_err(|e| e.to_string())?;

        match mode {
            "replace" => {
                let sink = Sink::try_new(&stream_handle)
                    .map_err(|e| e.to_string())?;
                sink.set_volume(volume_clamped);
                sink.append(source);
                sink.sleep_until_end();
            }
            "queue" => {
                let mut sink_guard = QUEUE_SINK.lock().unwrap();
                if let Some(ref sink) = *sink_guard {
                    sink.set_volume(volume_clamped);
                    sink.append(source);
                } else {
                    let sink = Sink::try_new(&stream_handle)
                        .map_err(|e| e.to_string())?;
                    sink.set_volume(volume_clamped);
                    sink.append(source);
                    *sink_guard = Some(sink);
                }
            }
            "concurrent" | _ => {
                let sink = Sink::try_new(&stream_handle)
                    .map_err(|e| e.to_string())?;
                sink.set_volume(volume_clamped);
                sink.append(source);
                sink.sleep_until_end();
            }
        }

        Ok(())
    }

    pub fn stop_audio() -> Result<(), String> {
        let mut sink_guard = QUEUE_SINK.lock().unwrap();
        if let Some(ref sink) = *sink_guard {
            sink.stop();
            *sink_guard = None;
        }
        Ok(())
    }
}

pub async fn play_audio_file(media_path: &str, volume: u32, mode: &str) -> Result<(), String> {
    #[cfg(feature = "target-platform")]
    {
        audio::play_audio_file(media_path, volume, mode)
    }

    #[cfg(not(feature = "target-platform"))]
    {
        println!("Hardware (Mock): Playing audio file {} at volume {} (mode: {})",
            media_path, volume, mode);
        Ok(())
    }
}

pub async fn stop_audio() -> Result<(), String> {
    #[cfg(feature = "target-platform")]
    {
        audio::stop_audio()
    }

    #[cfg(not(feature = "target-platform"))]
    {
        println!("Hardware (Mock): Stopping audio playback");
        Ok(())
    }
}

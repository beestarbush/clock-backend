use tokio::fs;

#[cfg(feature = "target-platform")]
const BRIGHTNESS_FILE: &str = "/sys/class/backlight/11-0045/brightness";
#[cfg(not(feature = "target-platform"))]
const BRIGHTNESS_FILE: &str = "./brightness";

#[cfg(feature = "target-platform")]
const TEMPERATURE_FILE: &str = "/sys/class/thermal/thermal_zone0/hwmon0/temp1_input";
#[cfg(not(feature = "target-platform"))]
const TEMPERATURE_FILE: &str = "./processor_temperature";

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
pub async fn get_temperature() -> Option<f64> {
    match fs::read_to_string(TEMPERATURE_FILE).await {
        Ok(content) => content.trim().parse::<f64>().ok(),
        Err(_) => None,
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

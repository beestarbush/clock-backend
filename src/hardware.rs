use tokio::fs;

///const BRIGHTNESS_FILE: &str = "/sys/class/backlight/backlight_0/brightness";
const BRIGHTNESS_FILE: &str = "./brightness";

/// Sets the screen brightness (0.0 to 1.0)
pub async fn set_brightness(val: f32) -> Result<(), String> {
    let pwm_val = (val.clamp(0.0, 1.0) * 255.0) as u32;
    
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

/// Sets the system volume (0.0 to 1.0)
pub async fn set_volume(val: f32) -> Result<(), String> {
    // Placeholder for actual ALSA/PulseAudio system calls
    println!("Hardware: Volume set to {:.2}", val);
    Ok(())
}

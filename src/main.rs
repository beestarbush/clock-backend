mod hardware;

use axum::{
    extract::{ws::{Message, WebSocket, WebSocketUpgrade}, State},
    routing::get,
    Router,
};
use axum::extract::Multipart;
use axum::response::Json;
use axum::http::StatusCode;
use serde::{Deserialize, Serialize};
use serde_json::{json, Value};
use std::{fs, sync::Arc};
use tokio::sync::{broadcast, Mutex};
use axum::http::{header, Method};
use tower_http::cors::{Any, CorsLayer};
use tower_http::services::ServeDir;

// --- Data Contract ---
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AppConfig {
    pub active_app_id: String,
    #[serde(rename = "system-configuration")]
    pub system_configuration: SystemConfig,
    pub applications: Vec<Value>, // Keeping apps flexible for now
    pub device_id: String,
    pub last_modified: String,
    pub version: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SystemConfig {
    pub brightness: f32,
    pub volume: f32,
    #[serde(rename = "pendulum-bob-color")]
    pub pendulum_bob_color: String,
    #[serde(rename = "pendulum-rod-color")]
    pub pendulum_rod_color: String,
    #[serde(rename = "pendulum-background-color")]
    pub pendulum_background_color: String,
    #[serde(rename = "base-color")]
    pub base_color: String,
    #[serde(rename = "accent-color")]
    pub accent_color: String,
}

// --- App State ---
struct AppState {
    config: Mutex<AppConfig>,
    tx: broadcast::Sender<String>,
}

#[tokio::main]
async fn main() {
    // 1. Load initial JSON (Ensure you have a configuration.json in the folder)
    let raw_json = fs::read_to_string("configuration.json")
        .expect("Failed to read configuration.json! Please create it.");
    let config: AppConfig = serde_json::from_str(&raw_json).expect("Invalid JSON schema");
    
    // Create media directory.
    let _ = fs::create_dir_all("media");

    // 2. Setup Broadcast Channel for real-time sync
    let (tx, _rx) = broadcast::channel(100);
    let state = Arc::new(AppState {
        config: Mutex::new(config),
        tx,
    });
    
    // Create a robust CORS layer
    let cors = CorsLayer::new()
        .allow_origin(Any) // For development, allow any origin
        .allow_methods([Method::GET, Method::POST, Method::OPTIONS])
        .allow_headers([header::CONTENT_TYPE]);

    // 3. Start Axum WebServer, websockets and REST API.
    let app = Router::new()
        .route("/ws", get(ws_handler))
        .route("/api/media", get(list_media).post(upload_media))
        .nest_service("/media", ServeDir::new("media"))
        .layer(cors)
        .with_state(state);

    println!("clock-backend running on:\n
    		ws://127.0.0.1:5000/ws\n
    		http://127.0.0.1:5000/api/media\n
    		http://127.0.0.1:5000/media");
    let listener = tokio::net::TcpListener::bind("0.0.0.0:5000").await.unwrap();
    axum::serve(listener, app).await.unwrap();
}

// --- WebSocket Setup ---
async fn ws_handler(ws: WebSocketUpgrade, State(state): State<Arc<AppState>>) -> impl axum::response::IntoResponse {
    ws.on_upgrade(|socket| handle_socket(socket, state))
}

// --- Core Logic & Routing ---
async fn handle_socket(mut socket: WebSocket, state: Arc<AppState>) {
    let mut rx = state.tx.subscribe();

    loop {
        tokio::select! {
            // 1. Listen for incoming UI commands
            Some(Ok(Message::Text(text))) = socket.recv() => {
                if let Ok(request) = serde_json::from_str::<Value>(&text) {
                    let method = request["method"].as_str().unwrap_or("");
                    let params = &request["params"];
                    let id = request["id"].clone();

                    let mut config_changed = false;

                    match method {
                        "getConfig" => {
                            let config = state.config.lock().await;
                            let response = json!({ "jsonrpc": "2.0", "result": *config, "id": id });
                            let _ = socket.send(Message::Text(response.to_string())).await;
                        }
                        "setBrightness" => {
                            if let Some(val) = params["value"].as_f64() {
                                let v = val as f32;
                                let _ = hardware::set_brightness(v).await;
                                state.config.lock().await.system_configuration.brightness = v;
                                config_changed = true;
                                send_success(&mut socket, id).await;
                            }
                        }
                        "setVolume" => {
                            if let Some(val) = params["value"].as_f64() {
                                let v = val as f32;
                                let _ = hardware::set_volume(v).await;
                                state.config.lock().await.system_configuration.volume = v;
                                config_changed = true;
                                send_success(&mut socket, id).await;
                            }
                        }
                        "setActiveApp" => {
                            if let Some(app_id) = params["app_id"].as_str() {
                                state.config.lock().await.active_app_id = app_id.to_string();
                                config_changed = true;
                                send_success(&mut socket, id).await;
                            }
                        }
                        "addApp" => {
                            // The frontend sends a complete new App object as the params
                            let new_app = params.clone();
                            state.config.lock().await.applications.push(new_app);
                            config_changed = true;
                            send_success(&mut socket, id).await;
                        }
                        "updateApp" => {
                            // The frontend sends the fully updated App object (must include its "id")
                            if let Some(app_id) = params["id"].as_str() {
                            let mut config = state.config.lock().await;
                            // Find the app by ID and replace it with the new incoming payload
                            if let Some(app) = config.applications.iter_mut().find(|a| a["id"] == app_id) {
                                *app = params.clone();
                                config_changed = true;
                                send_success(&mut socket, id).await;
                            }
                            }
                        }
                        "removeApp" => {
                            // The frontend just sends the ID to delete: {"id": "clock"}
                            if let Some(app_id) = params["id"].as_str() {
                            let mut config = state.config.lock().await;
                            // retain() keeps everything that DOES NOT match the ID we want to delete
                            config.applications.retain(|a| a["id"] != app_id);
                            config_changed = true;
                            send_success(&mut socket, id).await;
                            }
                        }
                        _ => {
                            // Catch unsupported commands
                            let error = json!({ "jsonrpc": "2.0", "error": "Method not found", "id": id });
                            let _ = socket.send(Message::Text(error.to_string())).await;
                        }
                    }

                    // If a mutation command was successful, save to disk and broadcast
                    if config_changed {
                        let current_config = state.config.lock().await.clone();
                        save_config_to_disk(&current_config).await;
                        
                        let broadcast_msg = json!({ "jsonrpc": "2.0", "method": "stateChanged", "params": current_config });
                        let _ = state.tx.send(broadcast_msg.to_string());
                    }
                }
            }
            // 2. Listen for broadcasts (Push updates from other clients to this socket)
            Ok(msg) = rx.recv() => {
                let _ = socket.send(Message::Text(msg)).await;
            }
        }
    }
}

// --- Helpers ---
async fn send_success(socket: &mut WebSocket, id: Value) {
    let ack = json!({ "jsonrpc": "2.0", "result": "success", "id": id });
    let _ = socket.send(Message::Text(ack.to_string())).await;
}

/// Atomically writes the configuration to disk to prevent corruption on power loss
async fn save_config_to_disk(config: &AppConfig) {
    if let Ok(updated_json) = serde_json::to_string_pretty(config) {
        let temp_path = "configuration.json.tmp";
        let final_path = "configuration.json";

        if let Err(e) = tokio::fs::write(temp_path, &updated_json).await {
            eprintln!("Failed to write temp config: {}", e);
        } else if let Err(e) = tokio::fs::rename(temp_path, final_path).await {
            eprintln!("Failed to atomically save config: {}", e);
        } else {
            println!("Config safely saved to disk.");
        }
    }
}

// --- Media HTTP API ---

/// Returns a JSON list of all files in the ./media folder
async fn list_media() -> Json<Vec<String>> {
    let mut files = Vec::new();
    if let Ok(mut entries) = tokio::fs::read_dir("media").await {
        while let Ok(Some(entry)) = entries.next_entry().await {
            if let Ok(name) = entry.file_name().into_string() {
                files.push(name);
            }
        }
    }
    Json(files)
}

/// Receives a file upload and saves it to the ./media folder
async fn upload_media(mut multipart: Multipart) -> Result<Json<Value>, StatusCode> {
    while let Ok(Some(field)) = multipart.next_field().await {
        let file_name = field.file_name().unwrap_or("unknown_file").to_string();
        if !file_name.is_empty() {
            let file_path = format!("media/{}", file_name);
            if let Ok(data) = field.bytes().await {
                if let Ok(_) = tokio::fs::write(&file_path, data).await {
                    return Ok(Json(json!({ "status": "success", "file": file_name })));
                }
            }
        }
    }
    Err(StatusCode::BAD_REQUEST)
}

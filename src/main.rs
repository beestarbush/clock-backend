mod hardware;

use axum::{
    extract::{ws::{Message, WebSocket, WebSocketUpgrade}, State},
    routing::{get, post},
    Router,
};
use serde::{Deserialize, Serialize};
use serde_json::{json, Value};
use std::{collections::HashSet, fs, sync::Arc};
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

// --- App Status (received from app via publish) ---
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct AppStatus {
    pub version: Option<String>,
    pub uptime: Option<f64>,
}

// --- App State ---
struct AppState {
    config: Mutex<AppConfig>,
    app_status: Mutex<AppStatus>,
    tx: broadcast::Sender<String>,
}

#[tokio::main]
async fn main() {
    // 1. Load initial JSON
    let raw_json = fs::read_to_string("configuration.json")
        .expect("Failed to read configuration.json! Please create it.");
    let config: AppConfig = serde_json::from_str(&raw_json).expect("Invalid JSON schema");
    
    // Create media directory
    let _ = fs::create_dir_all("media");

    // 2. Setup Broadcast Channel for real-time sync
    let (tx, _rx) = broadcast::channel(100);
    let state = Arc::new(AppState {
        config: Mutex::new(config),
        app_status: Mutex::new(AppStatus::default()),
        tx,
    });

    // 3. Start media directory watcher
    let watcher_state = state.clone();
    tokio::spawn(async move {
        watch_media_directory(watcher_state).await;
    });

    // Create a robust CORS layer
    let cors = CorsLayer::new()
        .allow_origin(Any)
        .allow_methods([Method::GET, Method::POST, Method::OPTIONS])
        .allow_headers([header::CONTENT_TYPE]);

    // 4. Start Axum WebServer
    let app = Router::new()
        .route("/ws", get(ws_handler))
        .route("/api/media", post(upload_media))
        .nest_service("/media", ServeDir::new("media"))
        .layer(cors)
        .with_state(state);

    println!("clock-backend running on:\n\
        \t\tws://127.0.0.1:5000/ws\n\
        \t\thttp://127.0.0.1:5000/media\n\
        \t\thttp://127.0.0.1:5000/api/media");
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
    let mut subscriptions: HashSet<String> = HashSet::new();

    println!("Client connected");

    loop {
        tokio::select! {
            // 1. Listen for incoming messages
            msg = socket.recv() => {
                match msg {
                    Some(Ok(Message::Text(text))) => {
                        if let Ok(msg) = serde_json::from_str::<Value>(&text) {
                            let msg_type = msg["type"].as_str().unwrap_or("");

                            match msg_type {
                                "request" => {
                                    handle_request(&mut socket, &state, &mut subscriptions, &msg).await;
                                }
                                "publish" => {
                                    handle_publish(&state, &msg).await;
                                }
                                _ => {
                                    // Backwards-compat: treat messages without type as requests
                                    if msg.get("method").is_some() && msg.get("id").is_some() {
                                        handle_request(&mut socket, &state, &mut subscriptions, &msg).await;
                                    } else {
                                        eprintln!("Unknown message type: {}", msg_type);
                                    }
                                }
                            }
                        }
                    }
                    Some(Ok(_)) => {
                        // Ignore non-text messages (binary, ping, pong, close)
                    }
                    Some(Err(e)) => {
                        eprintln!("WebSocket error: {}", e);
                        break;
                    }
                    None => {
                        // Client disconnected
                        break;
                    }
                }
            }
            // 2. Listen for broadcasts — only forward if client is subscribed to the topic
            Ok(broadcast_msg) = rx.recv() => {
                if let Ok(parsed) = serde_json::from_str::<Value>(&broadcast_msg) {
                    let topic = parsed["topic"].as_str().unwrap_or("");
                    if subscriptions.contains(topic) {
                        let _ = socket.send(Message::Text(broadcast_msg)).await;
                    }
                }
            }
        }
    }

    println!("Client disconnected (had {} subscriptions)", subscriptions.len());
    // subscriptions and rx are dropped here automatically
}

// --- Request Handler ---
async fn handle_request(
    socket: &mut WebSocket,
    state: &Arc<AppState>,
    subscriptions: &mut HashSet<String>,
    msg: &Value,
) {
    let method = msg["method"].as_str().unwrap_or("");
    let params = &msg["params"];
    let id = msg["id"].clone();

    let mut config_changed = false;

    match method {
        "subscribe" => {
            if let Some(topic) = params["topic"].as_str() {
                subscriptions.insert(topic.to_string());
                send_result(socket, id, json!({ "subscribed": topic })).await;
                println!("Client subscribed to: {}", topic);
            }
        }
        "unsubscribe" => {
            if let Some(topic) = params["topic"].as_str() {
                subscriptions.remove(topic);
                send_result(socket, id, json!({ "unsubscribed": topic })).await;
                println!("Client unsubscribed from: {}", topic);
            }
        }
        "getConfig" => {
            let config = state.config.lock().await;
            send_result(socket, id, json!(*config)).await;
        }
        "setBrightness" => {
            if let Some(val) = params["value"].as_f64() {
                let v = val as f32;
                let _ = hardware::set_brightness(v).await;
                state.config.lock().await.system_configuration.brightness = v;
                config_changed = true;
                send_result(socket, id, json!({ "brightness": v })).await;
            }
        }
        "setVolume" => {
            if let Some(val) = params["value"].as_f64() {
                let v = val as f32;
                let _ = hardware::set_volume(v).await;
                state.config.lock().await.system_configuration.volume = v;
                config_changed = true;
                send_result(socket, id, json!({ "volume": v })).await;
            }
        }
        "setActiveApp" => {
            if let Some(app_id) = params["app_id"].as_str() {
                state.config.lock().await.active_app_id = app_id.to_string();
                config_changed = true;
                send_result(socket, id, json!({ "active_app_id": app_id })).await;
            }
        }
        "addApp" => {
            let new_app = params.clone();
            state.config.lock().await.applications.push(new_app);
            config_changed = true;
            send_result(socket, id, json!({ "status": "added" })).await;
        }
        "updateApp" => {
            if let Some(app_id) = params["id"].as_str() {
                let mut config = state.config.lock().await;
                if let Some(app) = config.applications.iter_mut().find(|a| a["id"] == app_id) {
                    *app = params.clone();
                    config_changed = true;
                    send_result(socket, id, json!({ "status": "updated" })).await;
                }
            }
        }
        "removeApp" => {
            if let Some(app_id) = params["id"].as_str() {
                let mut config = state.config.lock().await;
                config.applications.retain(|a| a["id"] != app_id);
                config_changed = true;
                send_result(socket, id, json!({ "status": "removed" })).await;
            }
        }
        "getMedia" => {
            let files = list_media_files().await;
            send_result(socket, id, json!({ "files": files })).await;
        }
        _ => {
            let error = json!({
                "jsonrpc": "2.0",
                "type": "response",
                "error": { "code": -32601, "message": "Method not found" },
                "id": id
            });
            let _ = socket.send(Message::Text(error.to_string())).await;
        }
    }

    // If a mutation was successful, save to disk and publish to subscribers
    if config_changed {
        let current_config = state.config.lock().await.clone();
        save_config_to_disk(&current_config).await;

        let publish_msg = json!({
            "jsonrpc": "2.0",
            "type": "publish",
            "topic": "configuration",
            "params": current_config
        });
        println!("Publishing config_changed.");
        let _ = state.tx.send(publish_msg.to_string());
    }
}

// --- Publish Handler (incoming from client) ---
async fn handle_publish(state: &Arc<AppState>, msg: &Value) {
    let topic = msg["topic"].as_str().unwrap_or("");
    let params = &msg["params"];

    match topic {
        "application-status" => {
            let mut status = state.app_status.lock().await;
            if let Some(v) = params["version"].as_str() {
                status.version = Some(v.to_string());
            }
            if let Some(u) = params["uptime"].as_f64() {
                status.uptime = Some(u);
            }
            println!("App status updated: {:?}", *status);
        }
        _ => {
            eprintln!("Unknown publish topic from client: {}", topic);
        }
    }
}

// --- Helpers ---
async fn send_result(socket: &mut WebSocket, id: Value, result: Value) {
    let response = json!({
        "jsonrpc": "2.0",
        "type": "response",
        "result": result,
        "id": id
    });
    let _ = socket.send(Message::Text(response.to_string())).await;
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

// --- Media Helpers ---

/// Returns a list of all filenames in the ./media folder
async fn list_media_files() -> Vec<String> {
    let mut files = Vec::new();
    if let Ok(mut entries) = tokio::fs::read_dir("media").await {
        while let Ok(Some(entry)) = entries.next_entry().await {
            if let Ok(name) = entry.file_name().into_string() {
                files.push(name);
            }
        }
    }
    files
}

/// Watches the media directory and publishes changes to subscribers
async fn watch_media_directory(state: Arc<AppState>) {
    use tokio::time::{sleep, Duration};

    let mut last_files: Vec<String> = list_media_files().await;
    last_files.sort();

    loop {
        sleep(Duration::from_secs(5)).await;

        let mut current_files = list_media_files().await;
        current_files.sort();

        if current_files != last_files {
            println!("Media directory changed: {:?}", current_files);
            let publish_msg = json!({
                "jsonrpc": "2.0",
                "type": "publish",
                "topic": "media",
                "params": { "files": current_files }
            });
            let _ = state.tx.send(publish_msg.to_string());
            last_files = current_files;
        }
    }
}

// --- REST Handlers ---
async fn upload_media(mut multipart: axum::extract::Multipart) -> impl axum::response::IntoResponse {
    while let Some(field) = multipart.next_field().await.unwrap_or(None) {
        let name = field.name().unwrap_or("").to_string();
        let file_name = field.file_name().unwrap_or("").to_string();
        
        if name == "file" && !file_name.is_empty() {
            if let Ok(data) = field.bytes().await {
                let path = format!("media/{}", file_name);
                let _ = std::fs::write(&path, &data);
                println!("Uploaded media file: {}", file_name);
            }
        }
    }
    (axum::http::StatusCode::OK, "Uploaded")
}

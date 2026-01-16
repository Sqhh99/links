mod config;
mod handlers;
mod livekit_client;
mod models;
mod token;

use axum::{
    routing::{delete, get, post},
    Router,
};
use std::net::SocketAddr;
use std::path::PathBuf;
use tower_http::{
    cors::{Any, CorsLayer},
    services::{ServeDir, ServeFile},
    trace::TraceLayer,
};
use tracing::{info, Level};
use tracing_subscriber::FmtSubscriber;

use crate::config::Config;
use crate::handlers::{
    handle_create_room, handle_delete_room, handle_end_room, handle_get_token,
    handle_health, handle_kick_participant, handle_list_participants,
    handle_list_rooms, AppState,
};

#[tokio::main]
async fn main() {
    // Initialize logging
    let subscriber = FmtSubscriber::builder()
        .with_max_level(Level::INFO)
        .with_target(false)
        .with_thread_ids(false)
        .with_file(true)
        .with_line_number(true)
        .finish();
    tracing::subscriber::set_global_default(subscriber).expect("setting default subscriber failed");

    // Load .env file
    if let Err(e) = dotenvy::dotenv() {
        info!(".env file not found, using default config: {}", e);
    }

    // Load configuration
    let config = Config::from_env();
    let enable_https = config.enable_https;
    let cert_file = config.ssl_cert_file.clone();
    let key_file = config.ssl_key_file.clone();
    let port = config.server_port;
    let host = config.server_host.clone();
    let livekit_url = config.livekit_url.clone();
    let livekit_ws_url = config.livekit_ws_url.clone();
    let api_key = config.api_key.clone();

    // Create application state
    let state = AppState::new(config);

    // Configure CORS
    let cors = CorsLayer::new()
        .allow_origin(Any)
        .allow_methods(Any)
        .allow_headers(Any)
        .allow_credentials(false); // Note: Can't use credentials with Any origin

    // Static files path - look in parent directory's static folder
    let static_path = PathBuf::from("../static");
    let index_file = static_path.join("index.html");

    // Create API routes
    let api_routes = Router::new()
        .route("/token", post(handle_get_token))
        .route("/rooms", get(handle_list_rooms))
        .route("/rooms", post(handle_create_room))
        .route("/rooms/{room_name}", delete(handle_delete_room))
        .route("/rooms/{room_name}/participants", get(handle_list_participants))
        .route("/rooms/{room_name}/participants/{identity}", delete(handle_kick_participant))
        .route("/rooms/{room_name}/end", post(handle_end_room))
        .route("/health", get(handle_health));

    // Create main router with static file serving
    let app = Router::new()
        .nest("/api", api_routes)
        .fallback_service(
            ServeDir::new(&static_path)
                .not_found_service(ServeFile::new(&index_file))
        )
        .layer(cors)
        .layer(TraceLayer::new_for_http())
        .with_state(state);

    let addr = SocketAddr::from(([0, 0, 0, 0], port));
    let protocol = if enable_https { "https" } else { "http" };

    info!("LiveKit Signaling Server started successfully");
    info!("Server address: {}://{}:{}", protocol, host, port);
    info!("LiveKit API: {}", livekit_url);
    info!("LiveKit WebSocket: {}", livekit_ws_url);
    info!("API Key: {}", api_key);
    info!("HTTPS enabled: {}", enable_https);

    if enable_https {
        info!("Using TLS certificate: {}", cert_file);
        
        // HTTPS server with rustls
        let rustls_config = axum_server::tls_rustls::RustlsConfig::from_pem_file(&cert_file, &key_file)
            .await
            .expect("Failed to load TLS certificates");

        axum_server::bind_rustls(addr, rustls_config)
            .serve(app.into_make_service())
            .await
            .expect("HTTPS server failed");
    } else {
        // HTTP server
        let listener = tokio::net::TcpListener::bind(addr).await.expect("Failed to bind address");
        axum::serve(listener, app).await.expect("HTTP server failed");
    }
}

use axum::{
    extract::{Path, State},
    http::StatusCode,
    response::IntoResponse,
    Json,
};
use chrono::{TimeZone, Utc};
use std::sync::Arc;
use tracing::{error, info};

use crate::config::Config;
use crate::livekit_client::LiveKitClient;
use crate::models::{
    CreateRoomRequest, ErrorResponse, HealthResponse, MessageResponse, Room,
    TokenRequest, TokenResponse,
};
use crate::token::{AccessToken, VideoGrant};

/// Application state shared across handlers
#[derive(Clone)]
pub struct AppState {
    pub config: Arc<Config>,
    pub livekit_client: LiveKitClient,
}

impl AppState {
    pub fn new(config: Config) -> Self {
        let config = Arc::new(config);
        let livekit_client = LiveKitClient::new(config.clone());
        Self {
            config,
            livekit_client,
        }
    }
}

/// Helper function to create JSON error response
fn json_error(status: StatusCode, message: &str) -> impl IntoResponse {
    (status, Json(ErrorResponse { error: message.to_string() }))
}

/// Health check endpoint
pub async fn handle_health() -> impl IntoResponse {
    let response = HealthResponse {
        status: "ok".to_string(),
        time: Utc::now().to_rfc3339(),
    };
    (StatusCode::OK, Json(response))
}

/// Generate LiveKit access token
pub async fn handle_get_token(
    State(state): State<AppState>,
    Json(mut req): Json<TokenRequest>,
) -> impl IntoResponse {
    // Set defaults
    if req.room_name.is_empty() {
        req.room_name = "default-room".to_string();
    }
    if req.participant_name.is_empty() {
        req.participant_name = format!("user-{}", Utc::now().timestamp());
    }

    // Trim whitespace
    req.room_name = req.room_name.trim().to_string();
    req.participant_name = req.participant_name.trim().to_string();

    // Check if user should be host
    let mut is_host = req.is_host;
    if !is_host {
        // Check if room has participants, if not this user is host
        match state.livekit_client.list_participants(&req.room_name).await {
            Ok(participants) => {
                if participants.participants.is_empty() {
                    is_host = true;
                    info!("User '{}' is host of room '{}'", req.participant_name, req.room_name);
                }
            }
            Err(_) => {
                // Room doesn't exist or error, user is host
                is_host = true;
                info!("User '{}' is host of room '{}'", req.participant_name, req.room_name);
            }
        }
    }

    // Create video grant
    let grant = VideoGrant {
        room_join: Some(true),
        room: Some(req.room_name.clone()),
        can_publish: Some(true),
        can_subscribe: Some(true),
        ..Default::default()
    };

    // Create metadata
    let metadata = format!(r#"{{"isHost":{}}}"#, is_host);

    // Generate token
    let token_result = AccessToken::new(&state.config.api_key, &state.config.api_secret)
        .set_identity(&req.participant_name)
        .set_metadata(&metadata)
        .set_video_grant(grant)
        .set_valid_for(24 * 60 * 60) // 24 hours
        .to_jwt();

    match token_result {
        Ok(token) => {
            info!(
                "Token generated for user '{}' in room '{}' (is_host: {})",
                req.participant_name, req.room_name, is_host
            );

            let response = TokenResponse {
                token,
                url: state.config.livekit_ws_url.clone(),
                room_name: req.room_name,
                is_host,
            };
            (StatusCode::OK, Json(response)).into_response()
        }
        Err(e) => {
            error!("Failed to generate token: {}", e);
            json_error(StatusCode::INTERNAL_SERVER_ERROR, "Failed to generate token").into_response()
        }
    }
}

/// List all active rooms
pub async fn handle_list_rooms(State(state): State<AppState>) -> impl IntoResponse {
    match state.livekit_client.list_rooms().await {
        Ok(rooms) => {
            let room_list: Vec<Room> = rooms
                .into_iter()
                .map(|r| Room {
                    name: r.name.clone(),
                    display_name: r.name.clone(),
                    participants: r.num_participants.unwrap_or(0) as i32,
                    created_at: r.creation_time
                        .map(|t| Utc.timestamp_opt(t, 0).unwrap())
                        .unwrap_or_else(Utc::now),
                })
                .collect();
            (StatusCode::OK, Json(room_list)).into_response()
        }
        Err(e) => json_error(StatusCode::INTERNAL_SERVER_ERROR, &e).into_response(),
    }
}

/// Create a new room
pub async fn handle_create_room(
    State(state): State<AppState>,
    Json(mut req): Json<CreateRoomRequest>,
) -> impl IntoResponse {
    if req.name.is_empty() {
        req.name = format!("room-{}", Utc::now().timestamp());
    }

    match state.livekit_client.create_room(&req.name, 300, 50).await {
        Ok(room) => {
            let response = Room {
                name: room.name.clone(),
                display_name: room.name.clone(),
                participants: room.num_participants.unwrap_or(0) as i32,
                created_at: room.creation_time
                    .map(|t| Utc.timestamp_opt(t, 0).unwrap())
                    .unwrap_or_else(Utc::now),
            };
            (StatusCode::CREATED, Json(response)).into_response()
        }
        Err(e) => json_error(StatusCode::INTERNAL_SERVER_ERROR, &e).into_response(),
    }
}

/// Delete a room
pub async fn handle_delete_room(
    State(state): State<AppState>,
    Path(room_name): Path<String>,
) -> impl IntoResponse {
    match state.livekit_client.delete_room(&room_name).await {
        Ok(()) => {
            let response = MessageResponse {
                message: "Room deleted".to_string(),
                identity: None,
            };
            (StatusCode::OK, Json(response)).into_response()
        }
        Err(e) => json_error(StatusCode::INTERNAL_SERVER_ERROR, &e).into_response(),
    }
}

/// List participants in a room
pub async fn handle_list_participants(
    State(state): State<AppState>,
    Path(room_name): Path<String>,
) -> impl IntoResponse {
    match state.livekit_client.list_participants(&room_name).await {
        Ok(participants) => (StatusCode::OK, Json(participants)).into_response(),
        Err(e) => json_error(StatusCode::INTERNAL_SERVER_ERROR, &e).into_response(),
    }
}

/// Kick a participant from a room
pub async fn handle_kick_participant(
    State(state): State<AppState>,
    Path((room_name, identity)): Path<(String, String)>,
) -> impl IntoResponse {
    match state.livekit_client.remove_participant(&room_name, &identity).await {
        Ok(()) => {
            let response = MessageResponse {
                message: "Participant removed".to_string(),
                identity: Some(identity),
            };
            (StatusCode::OK, Json(response)).into_response()
        }
        Err(e) => json_error(StatusCode::INTERNAL_SERVER_ERROR, &e).into_response(),
    }
}

/// End a meeting (kick all participants and delete room)
pub async fn handle_end_room(
    State(state): State<AppState>,
    Path(room_name): Path<String>,
) -> impl IntoResponse {
    // Get all participants
    let participants = match state.livekit_client.list_participants(&room_name).await {
        Ok(p) => p,
        Err(e) => return json_error(StatusCode::INTERNAL_SERVER_ERROR, &format!("Failed to end meeting: {}", e)).into_response(),
    };

    // Kick all participants
    for p in participants.participants {
        if let Err(e) = state.livekit_client.remove_participant(&room_name, &p.identity).await {
            error!("Failed to remove participant '{}': {}", p.identity, e);
        }
    }

    // Delete the room
    if let Err(e) = state.livekit_client.delete_room(&room_name).await {
        error!("Failed to delete room: {}", e);
    }

    info!("Meeting '{}' ended, all participants removed", room_name);

    let response = MessageResponse {
        message: "Meeting ended".to_string(),
        identity: None,
    };
    (StatusCode::OK, Json(response)).into_response()
}

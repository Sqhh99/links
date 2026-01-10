package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"strings"
	"time"

	"github.com/gorilla/mux"
	"github.com/joho/godotenv"
	"github.com/livekit/protocol/auth"
	"github.com/livekit/protocol/livekit"
	lksdk "github.com/livekit/server-sdk-go/v2"
	"github.com/rs/cors"
)

var (
	// LiveKit 配置
	livekitURL   string // HTTP/HTTPS URL for API calls
	livekitWsURL string // WebSocket URL for client connections
	apiKey       string
	apiSecret    string
	serverPort   string
	serverHost   string
	roomClient   *lksdk.RoomServiceClient
)

// Room 房间信息
type Room struct {
	Name         string    `json:"name"`
	DisplayName  string    `json:"displayName"`
	Participants int       `json:"participants"`
	CreatedAt    time.Time `json:"createdAt"`
}

// TokenRequest 请求Token的结构
type TokenRequest struct {
	RoomName        string `json:"roomName"`
	ParticipantName string `json:"participantName"`
	IsHost          bool   `json:"isHost"` // 是否是主持人
}

// TokenResponse Token响应结构
type TokenResponse struct {
	Token    string `json:"token"`
	URL      string `json:"url"`
	RoomName string `json:"roomName"`
	IsHost   bool   `json:"isHost"` // 返回是否是主持人
}

// ErrorResponse 错误响应
type ErrorResponse struct {
	Error string `json:"error"`
}

// KickRequest 踢出参与者请求
type KickRequest struct {
	Identity string `json:"identity"` // 要踢出的参与者标识
}

func init() {
	// 加载环境变量
	if err := godotenv.Load(); err != nil {
		log.Println(".env file not found, using default config")
	}

	livekitURL = getEnv("LIVEKIT_URL", "http://localhost:7880")
	livekitWsURL = getEnv("LIVEKIT_WS_URL", "ws://localhost:7880")
	apiKey = getEnv("LIVEKIT_API_KEY", "devkey")
	apiSecret = getEnv("LIVEKIT_API_SECRET", "secret")
	serverPort = getEnv("SERVER_PORT", "8081")
	serverHost = getEnv("SERVER_HOST", "localhost")
}

func main() {
	log.SetFlags(log.LstdFlags | log.Lshortfile)

	// 初始化 LiveKit Room Client
	roomClient = lksdk.NewRoomServiceClient(livekitURL, apiKey, apiSecret)

	// HTTPS 配置
	enableHTTPS := getEnv("ENABLE_HTTPS", "false") == "true"
	certFile := getEnv("SSL_CERT_FILE", "./certs/server.crt")
	keyFile := getEnv("SSL_KEY_FILE", "./certs/server.key")

	// 创建路由
	router := mux.NewRouter()

	// API 端点 - 必须先注册API路由
	api := router.PathPrefix("/api").Subrouter()
	api.HandleFunc("/token", handleGetToken).Methods("POST")
	api.HandleFunc("/rooms", handleListRooms).Methods("GET")
	api.HandleFunc("/rooms", handleCreateRoom).Methods("POST")
	api.HandleFunc("/rooms/{roomName}", handleDeleteRoom).Methods("DELETE")
	api.HandleFunc("/rooms/{roomName}/participants", handleListParticipants).Methods("GET")
	api.HandleFunc("/rooms/{roomName}/participants/{identity}", handleKickParticipant).Methods("DELETE")
	api.HandleFunc("/rooms/{roomName}/end", handleEndRoom).Methods("POST")
	api.HandleFunc("/health", handleHealth).Methods("GET")

	// 静态文件服务
	router.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		http.ServeFile(w, r, "./static/index.html")
	}).Methods("GET")

	router.PathPrefix("/").Handler(http.FileServer(http.Dir("./static")))

	// CORS 配置
	c := cors.New(cors.Options{
		AllowedOrigins:   []string{"*"},
		AllowedMethods:   []string{"GET", "POST", "PUT", "DELETE", "OPTIONS"},
		AllowedHeaders:   []string{"*"},
		AllowCredentials: true,
	})

	handler := c.Handler(router)

	addr := fmt.Sprintf(":%s", serverPort)
	protocol := "http"
	if enableHTTPS {
		protocol = "https"
	}

	log.Println("LiveKit Signaling Server started successfully")
	log.Printf("Server address: %s://%s%s", protocol, serverHost, addr)
	log.Printf("LiveKit API: %s", livekitURL)
	log.Printf("LiveKit WebSocket: %s", livekitWsURL)
	log.Printf("API Key: %s", apiKey)
	log.Printf("HTTPS enabled: %v", enableHTTPS)

	var err error
	if enableHTTPS {
		log.Printf("Using TLS certificate: %s", certFile)
		err = http.ListenAndServeTLS(addr, certFile, keyFile, handler)
	} else {
		err = http.ListenAndServe(addr, handler)
	}
	if err != nil {
		log.Fatalf("Server failed to start: %v", err)
	}
}

// ============================================================================
// 辅助函数
// ============================================================================

func getEnv(key, defaultValue string) string {
	value := os.Getenv(key)
	if value == "" {
		return defaultValue
	}
	return value
}

func respondJSON(w http.ResponseWriter, status int, data interface{}) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(data)
}

func respondError(w http.ResponseWriter, status int, message string) {
	respondJSON(w, status, ErrorResponse{Error: message})
}

// ============================================================================
// HTTP 处理器
// ============================================================================

func serveHome(w http.ResponseWriter, r *http.Request) {
	http.ServeFile(w, r, "./static/index.html")
}

func handleHealth(w http.ResponseWriter, r *http.Request) {
	respondJSON(w, http.StatusOK, map[string]string{
		"status": "ok",
		"time":   time.Now().Format(time.RFC3339),
	})
}

// handleGetToken 生成 LiveKit 访问令牌
func handleGetToken(w http.ResponseWriter, r *http.Request) {
	var req TokenRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		respondError(w, http.StatusBadRequest, "Invalid request format")
		return
	}

	if req.RoomName == "" {
		req.RoomName = "default-room"
	}
	if req.ParticipantName == "" {
		req.ParticipantName = fmt.Sprintf("user-%d", time.Now().Unix())
	}

	// 清理房间名和参与者名
	req.RoomName = strings.TrimSpace(req.RoomName)
	req.ParticipantName = strings.TrimSpace(req.ParticipantName)

	// 检查房间是否已存在，如果不存在则此用户是主持人
	isHost := req.IsHost
	if !isHost {
		// 检查房间中是否已有参与者，如果没有则此用户是主持人
		participants, err := roomClient.ListParticipants(r.Context(), &livekit.ListParticipantsRequest{
			Room: req.RoomName,
		})
		if err != nil || len(participants.Participants) == 0 {
			isHost = true
			log.Printf("User '%s' is host of room '%s'", req.ParticipantName, req.RoomName)
		}
	}

	// 创建 Access Token
	at := auth.NewAccessToken(apiKey, apiSecret)
	canPublish := true
	canSubscribe := true
	grant := &auth.VideoGrant{
		RoomJoin:     true,
		Room:         req.RoomName,
		CanPublish:   &canPublish,
		CanSubscribe: &canSubscribe,
	}

	// 设置主持人元数据
	metadata := fmt.Sprintf(`{"isHost":%t}`, isHost)

	at.AddGrant(grant).
		SetIdentity(req.ParticipantName).
		SetMetadata(metadata).
		SetValidFor(24 * time.Hour)

	token, err := at.ToJWT()
	if err != nil {
		log.Printf("Failed to generate token: %v", err)
		respondError(w, http.StatusInternalServerError, "Failed to generate token")
		return
	}

	log.Printf("Token generated for user '%s' in room '%s' (is_host: %v)", req.ParticipantName, req.RoomName, isHost)

	respondJSON(w, http.StatusOK, TokenResponse{
		Token:    token,
		URL:      livekitWsURL,
		RoomName: req.RoomName,
		IsHost:   isHost,
	})
}

// handleListRooms 列出所有活跃的房间
func handleListRooms(w http.ResponseWriter, r *http.Request) {
	rooms, err := roomClient.ListRooms(r.Context(), &livekit.ListRoomsRequest{})
	if err != nil {
		log.Printf("Failed to list rooms: %v", err)
		respondError(w, http.StatusInternalServerError, "Failed to list rooms")
		return
	}

	var roomList []Room
	for _, room := range rooms.Rooms {
		roomList = append(roomList, Room{
			Name:         room.Name,
			DisplayName:  room.Name,
			Participants: int(room.NumParticipants),
			CreatedAt:    time.Unix(room.CreationTime, 0),
		})
	}

	respondJSON(w, http.StatusOK, roomList)
}

// handleCreateRoom 创建新房间
func handleCreateRoom(w http.ResponseWriter, r *http.Request) {
	var req struct {
		Name string `json:"name"`
	}
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		respondError(w, http.StatusBadRequest, "Invalid request format")
		return
	}

	if req.Name == "" {
		req.Name = fmt.Sprintf("room-%d", time.Now().Unix())
	}

	room, err := roomClient.CreateRoom(r.Context(), &livekit.CreateRoomRequest{
		Name:            req.Name,
		EmptyTimeout:    300, // 5分钟无人自动关闭
		MaxParticipants: 50,
	})
	if err != nil {
		log.Printf("Failed to create room: %v", err)
		respondError(w, http.StatusInternalServerError, "Failed to create room")
		return
	}

	log.Printf("Room created: %s", room.Name)

	respondJSON(w, http.StatusCreated, Room{
		Name:         room.Name,
		DisplayName:  room.Name,
		Participants: int(room.NumParticipants),
		CreatedAt:    time.Unix(room.CreationTime, 0),
	})
}

// handleDeleteRoom 删除房间
func handleDeleteRoom(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	roomName := vars["roomName"]

	_, err := roomClient.DeleteRoom(r.Context(), &livekit.DeleteRoomRequest{
		Room: roomName,
	})
	if err != nil {
		log.Printf("Failed to delete room: %v", err)
		respondError(w, http.StatusInternalServerError, "Failed to delete room")
		return
	}

	log.Printf("Room deleted: %s", roomName)

	respondJSON(w, http.StatusOK, map[string]string{
		"message": "Room deleted",
	})
}

// handleListParticipants 列出房间中的参与者
func handleListParticipants(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	roomName := vars["roomName"]

	participants, err := roomClient.ListParticipants(r.Context(), &livekit.ListParticipantsRequest{
		Room: roomName,
	})
	if err != nil {
		log.Printf("Failed to list participants: %v", err)
		respondError(w, http.StatusInternalServerError, "Failed to list participants")
		return
	}

	respondJSON(w, http.StatusOK, participants)
}

// handleKickParticipant 踢出参与者
func handleKickParticipant(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	roomName := vars["roomName"]
	identity := vars["identity"]

	_, err := roomClient.RemoveParticipant(r.Context(), &livekit.RoomParticipantIdentity{
		Room:     roomName,
		Identity: identity,
	})
	if err != nil {
		log.Printf("Failed to remove participant: %v", err)
		respondError(w, http.StatusInternalServerError, "Failed to remove participant: "+err.Error())
		return
	}

	log.Printf("Participant '%s' removed from room '%s'", identity, roomName)

	respondJSON(w, http.StatusOK, map[string]string{
		"message":  "Participant removed",
		"identity": identity,
	})
}

// handleEndRoom 结束会议（踢出所有参与者并删除房间）
func handleEndRoom(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	roomName := vars["roomName"]

	// 获取所有参与者并踢出
	participants, err := roomClient.ListParticipants(r.Context(), &livekit.ListParticipantsRequest{
		Room: roomName,
	})
	if err != nil {
		log.Printf("Failed to list participants: %v", err)
		respondError(w, http.StatusInternalServerError, "Failed to end meeting: "+err.Error())
		return
	}

	// 踢出所有参与者
	for _, p := range participants.Participants {
		_, err := roomClient.RemoveParticipant(r.Context(), &livekit.RoomParticipantIdentity{
			Room:     roomName,
			Identity: p.Identity,
		})
		if err != nil {
			log.Printf("Failed to remove participant '%s': %v", p.Identity, err)
		}
	}

	// 删除房间
	_, err = roomClient.DeleteRoom(r.Context(), &livekit.DeleteRoomRequest{
		Room: roomName,
	})
	if err != nil {
		log.Printf("Failed to delete room: %v", err)
	}

	log.Printf("Meeting '%s' ended, all participants removed", roomName)

	respondJSON(w, http.StatusOK, map[string]string{
		"message": "Meeting ended",
	})
}

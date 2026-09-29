# PR：用一个 compose 文件启动 PostgreSQL、LiveKit 和服务端

- **日期：** 2026-09-29
- **仓库：** [Sqhh99/links-sig-server](https://github.com/Sqhh99/links-sig-server)（本仓库的 `server/` 子模块）
- **分支：** `chore/docker-stack` → `main`
- **基线提交：** `d615906`
- **PR：** [Sqhh99/links-sig-server#2](https://github.com/Sqhh99/links-sig-server/pull/2)
- **关联记录：** [工作记录](../work-logs/2026-09-29-server-docker-stack.md) · [客户端 PR 记录](2026-09-29-server-docker-stack.md)

## 关联

原来启动后端要分三次：`docker/postgres-docker/`、`docker/livekit-docker/`、`docker/links-sig-server/` 各有一套 compose，配置也分散在三处。这些配置很久没用，检查后发现下面这些问题：

- **Dockerfile：**
  - 每次改代码都会重新编译全部依赖；
  - 没有健康检查；
  - `docker stop` 要等到超时才会结束容器。
- **LiveKit：**
  - 镜像用的是 `latest`，没有固定版本；
  - API secret 写死在 `livekit.yaml` 里，并提交到了公开仓库；
  - 没有设置对外通告的 IP，LiveKit 会把容器内的 IP 发给客户端，而客户端访问不到这个 IP。
- **`docs/DEPLOYMENT.md`：**
  - 文件路径和镜像名都对不上；
  - 健康检查写成了 `/health`，实际是 `/api/health`。

本 PR 把三套配置合并成一个 `docker/compose.yaml`，所有需要修改的配置都放在 `docker/.env`，并补充使用说明 `docker/README.md`。

## 改了什么

### 新增 `docker/`

- `compose.yaml`：
  - 包含三个服务：
    - `postgres`：端口只绑定 `127.0.0.1`；
    - `livekit`：固定为 `v1.13.7`；
    - `server`：从 `Dockerfile` 构建，等 `postgres` 健康后再启动。
  - 连接地址：`server` 访问数据库和 LiveKit 的地址由 compose 自动填好，指向容器内的服务。
  - 密钥：
    - LiveKit 的 key/secret 通过 `LIVEKIT_KEYS` 从 `.env` 注入，服务端和 LiveKit 共用这一组；
    - `JWT_SECRET`、`LIVEKIT_API_SECRET`、`POSTGRES_PASSWORD` 为必填项，没填时 `docker compose` 会直接报错，并提示缺哪一项。
  - LiveKit 的对外 IP 由 `LIVEKIT_NODE_IP` 注入（对应 `NODE_IP`），默认是 `127.0.0.1`。
  - 运维相关：
    - 日志按 3 个 10 MB 文件轮转；
    - `server` 设置了 `init: true`，停止时不用等超时；
    - 数据卷名可以用 `POSTGRES_VOLUME` 覆盖，方便沿用旧数据卷。
  - 另有 `postgres-test` 服务，给 `cargo test` 用：
    - 属于 `test` profile，只在点名时启动；
    - 数据放在 tmpfs，端口仍是 `127.0.0.1:5433`。
- `.env.example`：配置模板，每项都有中文注释。`.gitignore` 增加了 `!docker/.env.example`，否则会被已有的 `.env.example` 规则忽略。
- `livekit.yaml`：只保留端口和 RTC 设置，不再写密钥。
- `README.md`，内容包括：
  - 首次启动；
  - 常用命令：启动、日志、重启、备份等；
  - 配置说明：本机、局域网、公网三种场景下的 `LIVEKIT_WS_URL` / `LIVEKIT_NODE_IP` 填法，以及 HTTPS 反向代理；
  - `cargo run` 混合开发、测试库；
  - 从旧配置迁移；
  - 常见问题。

### `Dockerfile` 与 `.dockerignore`

- **Dockerfile：**
  - Rust 从 1.92 升到 1.98，与本地开发用的版本一致；
  - cargo registry 和 `target/` 放进 BuildKit 缓存挂载，并加上 `--locked`。只改源码时，只重新编译本 crate：实测 45 秒，首次需要 3 分钟；
  - 运行镜像加装 `curl`，并增加指向 `/api/health` 的 `HEALTHCHECK`；
  - `static/` 直接从构建上下文复制；
  - 默认设置 `SERVER_HOST=0.0.0.0`。
- **`.dockerignore`：** 增加 `docker/`、`.env.*`、`*.md`、`.github`、`Dockerfile` 等，避免把本地配置带进镜像。

### 删除与文档

- 删除 `docker/postgres-docker/`、`docker/livekit-docker/`、`docker/links-sig-server/`，以及已过时的 `docs/DEPLOYMENT.md`。
- `docs/TROUBLESHOOTING.md`、`PROJECT_STRUCTURE.md` 改为新的目录和命令。
- `tests/support/db.rs`：连不上测试库时，提示改为 `docker compose -f docker/compose.yaml up -d postgres-test`。

## 升级注意

- **旧容器：** `links-sig-postgres`、`links-sig-postgres-test`、`livekit-server` 如果还在，会和新配置抢同样的端口。先执行 `docker rm -f` 删除它们，数据卷不受影响。
- **旧数据：** 旧开发库在数据卷 `postgres-docker_postgres_data` 里。想继续使用，在 `.env` 里设置 `POSTGRES_VOLUME=postgres-docker_postgres_data`。
- **密钥：** 旧的 LiveKit secret 已经在公开仓库的历史里，不要再用。
- **`cargo run` 混合开发：** 根目录 `.env` 里的 `LIVEKIT_API_KEY` / `LIVEKIT_API_SECRET` 要和 `docker/.env` 保持一致。

## 怎么验证

环境：Docker Desktop（Engine 29.7.2，Compose v5.5.0），Windows 上的 Rust 1.98.1。

- [x] `docker compose config --quiet` 通过。删掉必填密钥后，会报出对应的提示。
- [x] 首次构建 `docker compose build server` 成功：
  - 总耗时 7 分 40 秒，其中编译 3 分 03 秒，镜像 150 MB；
  - 编译时的 2 个 dead code warning 在本次改动之前就有。
- [x] 在 `src/main.rs` 加一行注释后重新构建：只编译本 crate，45 秒完成。之后已还原。
- [x] `docker compose up -d`：
  - 三个服务都已启动，`server` 和 `postgres` 为 healthy；
  - 迁移 001–006 执行完成；
  - LiveKit 日志显示它通告的 node IP 是 `127.0.0.1`。
- [x] 用 curl 检查 API：
  - `/api/health` 返回 200；
  - 新用户名登录返回 201，再次登录返回 200；
  - `/api/rooms` 返回 200，说明服务端用共享的 key/secret 调通了 LiveKit API；
  - 主持人入会返回的 `url` 是 `ws://127.0.0.1:7880`；
  - `/join/?meetingNo=...` 返回 200。
- [x] 媒体链路：在 Windows 上用 LiveKit CLI 2.18.8 跑 `lk load-test`，1 个视频发布者、1 个订阅者，持续 15 秒。订阅端收到 1878 个包，码率 1.1 Mbps，丢包 0。
- [x] `docker compose down` 用时 2.6 秒。重新 `up` 后，之前的账号仍能登录，数据保留。
- [x] `docker compose up -d postgres-test` 后执行 `cargo test`，全部通过：
  - 单元测试 12 个，另有 1 个被忽略，跑了两遍；
  - 集成测试：`auth_login` 14、`auth_refresh` 7、`health` 5、`meetings` 36、`rooms` 17、`token` 11。
- [x] `cargo run` 混合模式：在本机运行服务端，连 Docker 里的 PostgreSQL 和 LiveKit。`/api/rooms` 和登录都返回 200。
- [x] `POSTGRES_VOLUME` 覆盖：用一个属于其他 compose 项目的临时数据卷测试，`postgres` 挂载的就是这个数据卷。
- [ ] 没有用 Links 客户端实际连接这套服务。
- [ ] 没有验证局域网、公网、HTTPS 反向代理这几种场景。

## 检查项

- [ ] 已阅读 CONTRIBUTING.md（服务端仓库有 `docs/CONTRIBUTING.md`，但这次改动不涉及其中的代码规范）
- [x] 若改动了打包资源，已在上文写明：`Dockerfile` 和 `.dockerignore` 有改动，删除了旧的 compose 配置
- [x] AI 使用披露：是。全部改动由 Claude Code 完成，并按上文完成了构建和运行验证。

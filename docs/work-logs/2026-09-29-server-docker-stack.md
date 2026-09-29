# 工作记录：服务端 Docker 配置检查与一键启动

- **日期：** 2026-09-29
- **分支：**
  - 服务端子模块 `server/`：`chore/docker-stack`，基于 `main` 的 `d615906`（已包含 links-sig-server#1），提交 `a032234`
  - 客户端：`chore/server-docker-stack`，基于 `main` 的 `8d31360`，升级 `server` 子模块指针并加入本记录
- **关联文档：**
  - [服务端 PR 记录](../pull-requests/2026-09-29-docker-stack.md)（[Sqhh99/links-sig-server#2](https://github.com/Sqhh99/links-sig-server/pull/2)）
  - [客户端 PR 记录](../pull-requests/2026-09-29-server-docker-stack.md)
  - [用户名登录的工作记录](2026-09-28-username-login.md)

## 一、用户的请求

> Currently, starting the backend services is a hassle because I have to launch LiveKit, PostgreSQL, and `links-sig-server` individually. Although there are Dockerfiles available for building containers, I haven't used them in a long time and lack confidence in their current state. Please review them and make any necessary fixes or optimizations. Ideally, I would like a setup that includes the appropriate configuration files and a `docker-compose` script to streamline the startup process. Please also add a `README.md` file explaining how to start the services, view logs, handle configuration, and so on.

追加请求：

> pls pr

即：提交改动，开服务端 PR，再开客户端 PR 升级子模块指针并加入记录。

实际要做的事：

- 检查服务端现有的 `Dockerfile` 和三套 compose 配置，修掉问题。
- 提供一个 compose 文件，一条命令启动 PostgreSQL、LiveKit 和 links-sig-server。
- 配套配置模板和 README，说明启动、日志、配置。

## 二、检查发现的问题

| 位置 | 问题 |
| --- | --- |
| 整体 | 三套 compose 各管一件事：`postgres-docker/` 管数据库，`livekit-docker/` 管 LiveKit，`links-sig-server/` 只跑服务端。服务端通过 `host.docker.internal` 访问另外两个，要分三次启动、分别配置 |
| `Dockerfile` | `COPY . .` 之后直接 `cargo build`：任何源码改动都会重新编译全部依赖（首次编译约 3 分钟）。没有 `--locked`。Rust 1.92 比本地实际使用的 1.98 旧 |
| `Dockerfile` | 没有健康检查。服务端没有处理 SIGTERM，作为容器内的 1 号进程时，`docker stop` 要等到超时才会强制结束 |
| `livekit-docker/` | 镜像用 `latest`，没有固定版本 |
| `livekit-docker/` | API secret 写死在 `livekit.yaml` 里并提交到了公开仓库 |
| `livekit-docker/` | 没有设置对外通告的 IP，LiveKit 会把容器内的 IP 发给客户端，客户端访问不到 |
| `docs/DEPLOYMENT.md` | 与实际文件对不上：`.env.deploy.example` 实际在 `docker/links-sig-server/` 下；镜像名前后不一致（`links-sig-rust-server` / `links-sig-server`）；健康检查写成 `/health`，实际是 `/api/health` |
| 本地 `server/.env.example`（未跟踪） | `LIVEKIT_API_SECRET=secret`，与 `livekit.yaml` 里的 secret 不一致 |

## 三、具体改了哪些文件

均在服务端子模块 `server/` 中。

| 文件 | 改动 | 原因 |
| --- | --- | --- |
| `docker/compose.yaml` | 新增，编排 `postgres`、`livekit`、`server` 三个服务，另有 `postgres-test`（`test` profile，点名才启动）。具体做法：<br>- `server` 从上级目录的 `Dockerfile` 构建，等 `postgres` 健康后再启动；<br>- 数据库地址和 LiveKit 地址自动指向容器内的服务；<br>- 密钥用 `${VAR:?}` 强制必填；<br>- LiveKit 的 key/secret 通过 `LIVEKIT_KEYS` 从 `.env` 注入，对外 IP 通过 `NODE_IP` 注入；<br>- PostgreSQL 端口只绑定 `127.0.0.1`；<br>- 日志按 3×10 MB 轮转；<br>- `server` 设置 `init: true`；<br>- 测试库使用 tmpfs；<br>- 数据卷名可以通过 `POSTGRES_VOLUME` 覆盖 | 一条命令启动全部服务；配置集中到一处 |
| `docker/.env.example` | 新增配置模板，每项都有中文注释 | 用户只需改这一个文件 |
| `docker/livekit.yaml` | 新增：只保留端口和 RTC 设置，不再写密钥 | 密钥不进仓库 |
| `docker/README.md` | 新增，内容包括：<br>- 首次启动和常用命令（启动、日志、重启、备份等）；<br>- 配置说明，重点是本机、局域网、公网三种场景下 `LIVEKIT_WS_URL` / `LIVEKIT_NODE_IP` 的填法，以及 HTTPS 反向代理；<br>- `cargo run` 混合开发模式和测试库的用法；<br>- 从旧配置迁移、常见问题 | 用户要求的说明文档 |
| `Dockerfile` | 改动：<br>- Rust 升到 1.98；<br>- 用 BuildKit 缓存挂载保存 cargo registry 和 `target/`，并加 `--locked`；<br>- 运行镜像加装 `curl`，并加 `HEALTHCHECK`（`/api/health`）；<br>- `static/` 直接从构建上下文复制；<br>- 默认 `SERVER_HOST=0.0.0.0` | 只改源码时不再重编依赖；compose 能判断服务是否就绪 |
| `.dockerignore` | 增加 `docker/`、`.env.*`、`*.md`、`.github`、`Dockerfile` 等 | 缩小构建上下文，避免把本地配置带进镜像 |
| `.gitignore` | 增加 `!docker/.env.example` | 原有的 `.env.example` 规则会把新模板也忽略掉 |
| `docker/postgres-docker/`、`docker/livekit-docker/`、`docker/links-sig-server/` | 删除 | 已合并进 `docker/compose.yaml` |
| `docs/DEPLOYMENT.md` | 删除 | 内容已过时，由 `docker/README.md` 取代 |
| `docs/TROUBLESHOOTING.md`、`PROJECT_STRUCTURE.md` | 命令和目录说明改为新的布局 | 与实际一致 |
| `tests/support/db.rs` | 连不上测试库时的提示改为 `docker compose -f docker/compose.yaml up -d postgres-test` | 旧提示依赖已删除的目录 |

本地未跟踪的配置文件（不会进入提交）：

| 文件 | 改动 |
| --- | --- |
| `server/docker/.env` | 从模板生成，`JWT_SECRET` 和 `LIVEKIT_API_SECRET` 用 `openssl rand -hex 32` 生成了新值 |
| `server/.env` | `LIVEKIT_API_SECRET` 改为与 `docker/.env` 一致，使 `cargo run` 能连上 Docker 里的 LiveKit；保留 CRLF 行尾 |

## 四、验证情况

环境：Docker Desktop（Engine 29.7.2，Compose v5.5.0），Windows 上的 Rust 1.98.1，均从 WSL 调用。

- [x] `docker compose config --quiet` 通过。
  - 删掉两个密钥后执行 `config`，会报 `required variable JWT_SECRET is missing a value: set JWT_SECRET in docker/.env`。
- [x] 首次构建 `docker compose build server` 成功：总耗时 7 分 40 秒，其中编译 3 分 03 秒。镜像 150 MB。
  - 编译中的 2 个 dead code warning 在改动之前就有。
- [x] 修改 `src/main.rs`（加一行注释）后重新构建：只编译了本 crate，45 秒完成。之后已还原该文件。
- [x] `docker compose up -d`：
  - 三个服务都起来了，`server` 和 `postgres` 为 healthy；
  - 服务端日志显示连上了数据库，并完成了迁移 001–006；
  - LiveKit 日志显示使用显式的 node IP `127.0.0.1`，版本 1.13.7。
- [x] 用 curl 访问服务端 API：
  - `GET /api/health` 返回 200；
  - 新用户名登录返回 201，再次登录返回 200；
  - `GET /api/rooms` 返回 200，说明服务端用共享的 key/secret 调通了 LiveKit API；
  - 创建会议后以主持人身份入会，返回的 `url` 为 `ws://127.0.0.1:7880`；
  - `/join/?meetingNo=...` 返回 200 的 HTML 页面。
- [x] 媒体链路：在 Windows 上用 LiveKit CLI 2.18.8 执行 `lk load-test`（1 个视频发布者、1 个订阅者、15 秒）。订阅端收到 1878 个包，码率 1.1 Mbps，丢包 0。这说明从宿主机经 Docker 端口转发的媒体链路是通的。
- [x] `docker compose down` 用时 2.6 秒（`init: true` 生效）。重新 `up` 后，之前创建的账号再次登录返回 200，数据保留。
- [x] `docker compose up -d postgres-test` 后执行 `cargo test`，全部通过：
  - 单元测试 12 个（另有 1 个被忽略），跑了两遍；
  - 集成测试：`auth_login` 14、`auth_refresh` 7、`health` 5、`meetings` 36、`rooms` 17、`token` 11。
- [x] 混合模式：停掉 `server` 容器，在本机 `cargo run`，连接 Docker 里的 PostgreSQL 和 LiveKit。`/api/rooms` 返回 200，登录返回 200。
- [x] `POSTGRES_VOLUME` 覆盖：用一个标记为其他 compose 项目的临时数据卷测试，`postgres` 挂载的确实是该数据卷。
- [x] 收尾：
  - 已执行 `docker compose --profile test down`，并删除测试中创建的 `links_postgres_data` 和临时数据卷；
  - 已构建的 `links-sig-server:local` 镜像和构建缓存保留；
  - 旧数据卷（`postgres-docker_postgres_data` 等）没有动过。
- [ ] 没有用 Links 客户端实际连这套服务。
- [ ] 没有验证局域网、公网和 HTTPS 反向代理这几种场景，README 里相关内容是按 LiveKit 的配置语义写的。

## 五、遗留事项

- **合并顺序**：先合并服务端 PR #2（沿用 merge commit），再合并客户端 PR。
  - 客户端 PR 把 `server` 子模块指针从 `bf360ec` 升到 `a032234`，一并带上已合并的 links-sig-server#1 和本次改动。
  - 如果服务端 PR 改用 squash 合并，或者合并前又有新提交，客户端 PR 里的指针需要改成合并后 `main` 上的提交。
- 服务端的 tracing 日志总是带 ANSI 颜色码。在终端里用 `docker compose logs` 看没问题，重定向到文件时会有转义字符。如果需要，可以在 `init_logging` 里按是否为终端来决定是否输出颜色。
- 旧的 LiveKit secret 已经在公开仓库的历史里，本地的 `docker/.env` 已换成新值，不要再用旧值。
- 机器上还有几个旧数据卷：`postgres-docker_postgres_data`、`links-sig-rust-server_postgres_data`、`postgre-docker_postgresql_data`，以及几个测试库数据卷。是否沿用或清理，由作者决定。

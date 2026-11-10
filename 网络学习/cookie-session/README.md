# Cookie / Session：从自己的 HTTP 服务器继续

目录已由 `Cookie` 改名为 `cookie-session`，原来的 `网络学习/HTTP` 保留为上一章。本章沿用 `Socket → TcpServer → HttpServer → 业务回调`，同时保留原有 Cookie 实验。

## 编译与启动

```bash
cd /home/whb/linux/网络学习/cookie-session
make
./http_server 18081
```

也可以用 `make run PORT=18081`；`make clean` 只删除本目录程序。浏览器访问 `http://VPS公网IP:18081/`，本机访问 `http://127.0.0.1:18081/`。如果此前已经启动过旧程序，先在那个终端 Ctrl+C，再启动新版；改目录名不会让运行中的程序自动更新。

## 先体验 Session

1. 打开 `/private`：未登录返回 **401**，不返回受保护内容。
2. 首页点击“一键演示登录”：固定用户 whb，模拟账号认证通过。服务端创建 Session，响应设置 `cpp_session_id` Cookie，再 **303** 跳转 `/session`。
3. 刷新 `/session`，查看用户、访问次数、剩余有效期。每次 TCP 连接都已经关闭，但同一个 Cookie 仍能找回同一份 Session；访问次数保存在服务器。
4. 打开 `/private`：服务端查询到有效会话后返回 **200**。仅设置普通的 `demo_user` Cookie 无法获得访问权。
5. 点击“退出登录”：删除服务器 Session，同时发送 `Max-Age=0` 删除 Cookie。再次访问受保护页面返回 401；保存过的旧编号也无法恢复登录。
6. 在 `/login.html` 点击“创建 5 秒实验会话”，稍等 6 秒后刷新，观察过期。默认会话为 300 秒，刷新不会续期。

无需输入账号，也无需在 main 设置 Cookie 内容或属性。**这里模拟认证通过，仅实现会话管理；没有真实账号密码校验和账号数据库，不是可直接部署的登录系统。** 原注册页仍只是接收表单的演示。

开发者工具的 Network 面板中观察 `Set-Cookie → 303 → Cookie`，Storage / Application 中查看 `cpp_session_id`。编号不包含用户名，也不在 HTML 页面或服务端日志中显示。用浏览器无痕窗口可以创建另一份独立会话；同一浏览器同一主机的普通标签页通常共享 Cookie。

## 再对比原来的 Cookie 实验

- **设置**：POST `/cookie/set`，保存固定的 `demo_user`、`demo_theme`，跳转 `/cookie`。
- **回传**：GET `/cookie` 显示本次收到的 Cookie，登录凭据隐藏。请求只携带名称和值，不携带 Path、HttpOnly 等保存属性。
- **路径**：POST `/cookie/path` 设置 Path=/a/b 的 `demo_path`；比较 `/a/b`、`/a/b/inside`、`/a/bc`。
- **删除**：POST `/cookie/clear` 删除这三个演示 Cookie，使用原名称和 Path。它不负责退出 Session 登录；退出请用 `/logout`。

`Cookie.hpp::Options` 提供统一默认值：Path=/、Max-Age=300、HttpOnly=true、SameSite=Lax、Secure=false、不设置 Domain，并根据有效期生成 GMT Expires。Session 只在封装内部覆盖有效期；清除时覆盖为 0。当前是 HTTP 实验，HTTPS 部署时还需要调整 Secure 等传输配置。

## 类与数据的关系

```text
HttpServer 路由中的 lambda 按值捕获 shared_ptr<SessionService>
    ↓ 所有相关路由共用一份 SessionService
SessionService：处理登录、查看、受保护页面、退出
    ↓ 拥有
SessionManager：mutex + unordered_map<随机编号, Session>
    ↓ 保存
Session：用户名、服务端到期时间、访问次数
```

```text
登录请求 → 创建随机编号 → 服务端保存 Session → Set-Cookie
新请求   → 读取 Cookie → 查表并检查期限 → 返回对应用户内容
退出请求 → 删除表中记录 → 删除浏览器 Cookie
```

`SessionManager` 用 Linux `getrandom()` 获取 32 字节随机数，编码成 64 个十六进制字符。重新登录会生成新编号并撤销当前 Cookie 对应的旧会话；不沿用客户端提供的编号。查询时返回 Session 的副本，避免解锁后还持有可能被其他线程删除的元素指针。

过期由服务器独立检查，不能只相信浏览器的 Max-Age。计时使用 `steady_clock`，不受系统墙上时钟调整影响。创建会话时清理全部过期记录，查询时清理本条过期记录；没有额外后台清理线程。Session 在内存中，**服务器重启会使所有会话失效**。

## 为什么本章改了 TcpServer

原来的双重 fork 会复制进程内存：一个请求在子进程创建的 Session，不会进入父进程的表，下一个请求也找不到它。

本章改成每连接一个线程；线程共享同一个 Manager，访问 map 时用 mutex 保护。主线程把客户端 socket 交给工作线程后继续 accept，由工作线程处理完响应再关闭；不能沿用原来父进程立即 close 的代码，否则会关闭线程正在使用的同一个 fd。原 HTTP 章节的进程版本保留不变。

这是方便理解共享状态的学习版，还没有线程池、连接超时或优雅停机，不作为高并发部署方案。多进程/多实例场景后续可以使用 Redis、数据库等外部共享存储。

## 对照代码阅读

1. `HttpServer.cc → SetDemoCookies`：普通 Cookie 设置和回传。
2. `HttpServer.cc → SessionService::Login`：模拟认证通过、创建会话、发 Cookie。
3. `Session.hpp → CreateSession / SearchSession / DeleteSession`：随机编号、服务端状态、互斥与过期。
4. `HttpServer.cc → RequireSession / View / Private / Logout`：根据服务端状态放行或拒绝，退出时撤销。
5. `main → shared_ptr 捕获的路由`、`TcpServer.hpp → 连接线程`：对象生命周期和共享关系。
6. `HttpProtocol.hpp → HttpRequest::Cookie / HttpResponse::SetCookie / Serialize`：报文解析与多条 Set-Cookie。

| 地址 | 方法 | 作用 |
| --- | --- | --- |
| `/`、`/login.html` | GET | 实验首页、演示登录和短期会话入口 |
| `/login` | POST | 创建 300 秒会话，轮换旧编号 |
| `/session/login-short` | POST | 创建 5 秒实验会话 |
| `/session` | GET | 有效会话的信息；无效则 401 |
| `/private` | GET | 需要有效会话的页面 |
| `/logout` | POST | 撤销会话并删除凭据 Cookie |
| `/cookie/set`、`/cookie/path`、`/cookie/clear` | POST | 普通 Cookie 设置、路径与删除实验 |
| `/cookie`、`/a/b`、`/a/b/inside`、`/a/bc` | GET | 查看本次请求的 Cookie |

## 验证

```bash
make test
```

测试在本机临时端口启动自己的服务器，用真实 HTTP 请求验证登录、跨连接状态、伪造拒绝、并发计数、会话隔离、编号轮换、退出后重放、5 秒过期、重启失效；用 curl Cookie 引擎验证会话自动回传及原 Cookie 路径、删除。临时服务和 Cookie 文件自动清理，不占用手动运行的 18081，也不输出会话编号。`make test` 在终端输出结果，本次执行的摘要见 `test.log`。

HTTP 仍为一连接、一请求、一响应、关闭连接，不支持 chunked 请求体或 Keep-Alive。原有 64 KiB Header / 1 MiB Body 限制、Content-Length 检查、HTML 转义、搜索和回显继续保留。静态文件仍为学习版路径检查，未实现完整符号链接隔离。

参考：[RFC 6265](https://www.rfc-editor.org/rfc/rfc6265)、[OWASP Session Management](https://cheatsheetseries.owasp.org/cheatsheets/Session_Management_Cheat_Sheet.html)、[Linux getrandom(2)](https://man7.org/linux/man-pages/man2/getrandom.2.html)。

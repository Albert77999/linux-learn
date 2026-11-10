#!/usr/bin/env python3
"""本机真实 HTTP 联调；不输出或保存 Session 编号，不操作已有服务。"""

from concurrent.futures import ThreadPoolExecutor
from http.client import HTTPConnection
from http.cookies import SimpleCookie
from pathlib import Path
from tempfile import TemporaryDirectory, TemporaryFile
import re
import socket
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
COOKIE = "cpp_session_id"


class Server:
    def __init__(self):
        with socket.socket() as listener:
            listener.bind(("127.0.0.1", 0))
            self.port = listener.getsockname()[1]
        self.log = TemporaryFile()
        self.start()

    def start(self):
        self.process = subprocess.Popen(
            [str(ROOT / "http_server"), str(self.port)], cwd=ROOT,
            stdout=self.log, stderr=subprocess.STDOUT,
        )
        for _ in range(100):
            assert self.process.poll() is None, "测试服务器意外退出"
            try:
                with socket.create_connection(("127.0.0.1", self.port), timeout=0.1):
                    return
            except OSError:
                time.sleep(0.03)
        raise RuntimeError("测试服务器启动超时")

    def stop(self):
        self.process.terminate()
        try:
            self.process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.wait()

    def request(self, method, path, cookie="", body=b""):
        headers = {"Cookie": cookie} if cookie else {}
        if method == "POST":
            headers["Content-Type"] = "application/x-www-form-urlencoded"
        connection = HTTPConnection("127.0.0.1", self.port, timeout=4)
        try:
            connection.request(method, path, body=body, headers=headers)
            response = connection.getresponse()
            data = response.read()
            assert int(response.getheader("Content-Length")) == len(data), "响应长度不正确"
            assert response.getheader("Connection") == "close", "本版应关闭连接"
            return response.status, dict(response.getheaders()), data.decode("utf-8")
        finally:
            connection.close()


def main():
    server = Server()
    tokens = []

    def login(path="/login", old_cookie="", expected_age=300):
        status, headers, body = server.request("POST", path, old_cookie)
        assert status == 303 and headers.get("Location") == "/session", "登录跳转失败"
        cookie = SimpleCookie()
        cookie.load(headers.get("Set-Cookie", ""))
        assert COOKIE in cookie, "没有设置会话 Cookie"
        item = cookie[COOKIE]
        assert re.fullmatch(r"[0-9a-f]{64}", item.value), "编号格式错误"
        assert item["path"] == "/" and item["httponly"], "Cookie 默认属性错误"
        assert item["samesite"] == "Lax" and not item["secure"], "HTTP 演示属性错误"
        assert int(item["max-age"]) == expected_age and item["expires"], "有效期错误"
        assert item.value not in body, "页面泄露了登录凭据"
        tokens.append(item.value)
        return item.value, COOKIE + "=" + item.value

    def visits(body):
        match = re.search(r'id="session-visits">(\d+)<', body)
        assert match, "会话页面缺少计数"
        return int(match.group(1))

    try:
        for path in ("/", "/login.html", "/style.css", "/register.html"):
            assert server.request("GET", path)[0] == 200, "静态资源不能访问"
        assert server.request("GET", "/not-found.jpg")[0] == 404
        assert server.request("GET", "/../Session.hpp")[0] == 403
        print("PASS：目录、页面、CSS、404 和静态路径限制")

        for cookie in ("", "demo_user=whb", COOKIE + "=whb", COOKIE + "=" + "0" * 64):
            assert server.request("GET", "/private", cookie)[0] == 401, "伪造身份被接受"
        for path in ("/login", "/logout", "/session/login-short"):
            assert server.request("GET", path)[0] == 405, "状态修改允许了 GET"
        assert server.request("POST", "/session")[0] == 405
        print("PASS：未登录/伪造 Cookie 被拒绝，方法限制正确")

        first_id, first = login()
        assert visits(server.request("GET", "/session", first)[2]) == 1
        assert visits(server.request("GET", "/session", first)[2]) == 2
        assert server.request("GET", "/private", first)[0] == 200
        status, _, body = server.request("GET", "/cookie", first)
        assert status == 200 and "已隐藏" in body and first_id not in body
        assert server.request("GET", "/private", first + "; demo_user=attacker")[0] == 200
        assert 'id="session-user">whb<' in server.request("GET", "/session", first)[2]
        print("PASS：跨 TCP 连接保留状态；身份来自 Session；页面隐藏凭据")

        _, second = login()
        assert visits(server.request("GET", "/session", second)[2]) == 1
        rotated_id, rotated = login(old_cookie=first)
        assert rotated_id != first_id, "重新登录没有轮换编号"
        assert server.request("GET", "/private", first)[0] == 401, "旧会话仍有效"
        assert server.request("GET", "/private", rotated)[0] == 200
        assert visits(server.request("GET", "/session", second)[2]) == 2
        print("PASS：浏览器会话相互独立，重新登录生成新编号并撤销旧会话")

        _, shared = login()

        def visit(_):
            status, _, body = server.request("GET", "/session", shared)
            assert status == 200
            return visits(body)

        with ThreadPoolExecutor(max_workers=8) as workers:
            counters = list(workers.map(visit, range(40)))
        assert sorted(counters) == list(range(1, 41)), "并发访问丢失计数"
        assert visits(server.request("GET", "/session", shared)[2]) == 41

        def independent(_):
            token, cookie = login()
            assert visits(server.request("GET", "/session", cookie)[2]) == 1
            return token

        with ThreadPoolExecutor(max_workers=8) as workers:
            independent_ids = list(workers.map(independent, range(20)))
        assert len(set(independent_ids)) == 20, "独立会话编号重复"
        print("PASS：40 次并发读改计数无遗漏；20 个并发登录状态隔离")

        # 一个未发完整 Header 的连接不能挡住另一个连接。
        with socket.create_connection(("127.0.0.1", server.port), timeout=4) as slow:
            slow.sendall(b"GET /session HTTP/1.1\r\nHost: localhost\r\n")
            assert server.request("GET", "/private", second)[0] == 200
        assert not Path(f"/proc/{server.process.pid}/task/{server.process.pid}/children").read_text().strip()
        print("PASS：未完成请求不阻塞其他连接，本章不再创建连接子进程")

        status, headers, _ = server.request("POST", "/logout", rotated)
        assert status == 303
        assert "Max-Age=0" in headers.get("Set-Cookie", "")
        assert server.request("GET", "/private", rotated)[0] == 401, "退出后重放仍可访问"
        assert server.request("POST", "/logout", rotated)[0] == 303
        assert server.request("GET", "/private", second)[0] == 200
        print("PASS：退出同时撤销服务端会话与 Cookie；旧编号重放失败；重复退出可用")

        _, short = login("/session/login-short", expected_age=5)
        assert server.request("GET", "/private", short)[0] == 200
        time.sleep(5.2)
        assert server.request("GET", "/private", short)[0] == 401, "服务器未检查过期"
        assert server.request("GET", "/private", second)[0] == 200
        print("PASS：5 秒会话过期后强行回传旧 Cookie 仍被拒绝，长会话不受影响")

        # 使用真实 curl Cookie 引擎回归默认值、Path 边界和删除。
        with TemporaryDirectory(prefix="cookie-session-test-") as temporary:
            jar = str(Path(temporary) / "cookies.jar")

            def curl(path, post=False, expected_status=None):
                command = ["curl", "-sS", "--noproxy", "*", "--max-time", "4",
                           "-b", jar, "-c", jar, "-w", "\n%{http_code}"]
                if post:
                    command += ["--data", ""]
                command += [f"http://127.0.0.1:{server.port}{path}"]
                output = subprocess.run(command, check=True, capture_output=True).stdout.decode()
                body, status_text = output.rsplit("\n", 1)
                allowed = {expected_status} if expected_status is not None else {200, 303}
                assert int(status_text) in allowed, "curl HTTP 状态不符合预期"
                return body

            curl("/cookie/set", True)
            assert "demo_user=whb" in curl("/cookie")
            curl("/cookie/path", True)
            assert "demo_path=inside-a-b" in curl("/a/b")
            assert "demo_path=inside-a-b" in curl("/a/b/inside")
            assert "demo_path=inside-a-b" not in curl("/a/bc")
            curl("/cookie/clear", True)
            assert "demo_user=whb" not in curl("/cookie")
            assert "demo_path=inside-a-b" not in curl("/a/b")
            curl("/login", True, 303)
            assert 'id="session-user">whb<' in curl("/session", expected_status=200)
            assert "受保护的页面" in curl("/private", expected_status=200)
            curl("/logout", True, 303)
            curl("/private", expected_status=401)
        assert server.request("GET", "/private", second)[0] == 200
        assert server.request("GET", "/search?keyword=HTTP")[0] == 200
        assert "&lt;script&gt;" in server.request("GET", "/test?message=%3Cscript%3E")[2]
        assert server.request("POST", "/register", body=b"username=demo&password=")[0] == 400
        print("PASS：curl 自动保存/回传会话及退出；Cookie 路径/删除与原 HTTP 业务回归")

        server.stop()
        server.start()
        assert server.request("GET", "/private", second)[0] == 401
        _, fresh = login()
        assert server.request("GET", "/private", fresh)[0] == 200
        server.stop()
        server.log.seek(0)
        output = server.log.read().decode(errors="replace")
        assert all(token not in output for token in tokens), "运行日志泄露会话编号"
        print("PASS：重启使内存 Session 失效；服务端日志没有记录会话编号")
    finally:
        server.stop()
        server.log.close()
    print("全部通过；临时服务已停止，临时 Cookie 文件已清理。")


if __name__ == "__main__":
    main()

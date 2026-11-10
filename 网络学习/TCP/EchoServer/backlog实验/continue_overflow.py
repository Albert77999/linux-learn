#!/usr/bin/env python3
"""继续观察同一个溢出客户端：腾位后成功，或由内核自然结束 connect。"""
import datetime as dt
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import socket
import subprocess
import tempfile
import time
from zoneinfo import ZoneInfo

BASE = Path(__file__).resolve().parent
SOURCE = BASE.parent
STAMP = dt.datetime.now(ZoneInfo("America/Los_Angeles")).strftime("%Y%m%d_%H%M%S")
OUT = BASE / ("overflow_" + STAMP)
OUT.mkdir()
LOG = (OUT / "test.log").open("w", buffering=1)
START = time.monotonic()
PROCS = []
SNAPS = []
RESULT = {"phases": {}, "snapshots": SNAPS}
FILES = sorted([*SOURCE.glob("*.hpp"), *SOURCE.glob("*.cc"), SOURCE / "Makefile"])


def say(message):
    line = f"[{time.monotonic() - START:8.3f}s] {message}"
    print(line, flush=True)
    LOG.write(line + "\n")


def run(args, **kw):
    return subprocess.run(args, text=True, capture_output=True, check=True, **kw)


def read(p):
    return p.read_text(errors="replace") if p.exists() else ""


def hashes():
    return {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in FILES}


def wait(predicate, label, timeout=6):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if predicate():
            return
        time.sleep(0.04)
    raise RuntimeError("等待超时：" + label)


def spawn(name, command, cwd):
    out = (OUT / (name + ".stdout.log")).open("w")
    p = subprocess.Popen(["strace", "-ff", "-ttt", "-T", "-s", "256", "-e",
                          "trace=network,read,write,close", "-o", str(OUT / (name + ".strace")),
                          *command], cwd=cwd, stdin=subprocess.PIPE, stdout=out,
                         stderr=subprocess.STDOUT, text=True, start_new_session=True)
    x = {"name": name, "proc": p, "out": out,
         "log": OUT / (name + ".stdout.log"), "spawn_time": time.monotonic()}
    PROCS.append(x)
    return x


def stop(x):
    if x["proc"].poll() is None:
        os.killpg(x["proc"].pid, signal.SIGTERM)
        try:
            x["proc"].wait(timeout=3)
        except subprocess.TimeoutExpired:
            os.killpg(x["proc"].pid, signal.SIGKILL)
            x["proc"].wait(timeout=3)


def connected(x):
    return "Init service success!" in read(x["log"])


def echoed(x):
    return "echo #" + x["name"] in read(x["log"])


def send(x, s):
    x["proc"].stdin.write(s + "\n")
    x["proc"].stdin.flush()


def trace(x):
    return "\n".join(read(p) for p in OUT.glob(x["name"] + ".strace.*"))


def accepted(server):
    return len(re.findall(r"accept(?:4)?\([^\n]*\)\s+=\s+\d+", trace(server)))


def netstat(port, options):
    raw = run(["netstat", options]).stdout
    lines = []
    for line in raw.splitlines():
        f = line.split()
        if line.startswith("Active") or line.startswith("Proto"):
            lines.append(line)
        elif f and f[0].startswith("tcp") and (f[3].rsplit(":", 1)[-1] == str(port)
                                               or f[4].rsplit(":", 1)[-1] == str(port)):
            lines.append(line)
    return "\n".join(lines) + "\n"


def snap(label, port, server, clients):
    listening = netstat(port, "-nltp")
    rows = [line.split() for line in listening.splitlines() if line.startswith("tcp")]
    assert len(rows) == 1 and server["proc"].poll() is None
    states = netstat(port, "-ntop")
    extra = run(["ss", "-ntio", f"( sport = :{port} or dport = :{port} )"]).stdout
    x = {"label": label, "elapsed": round(time.monotonic() - START, 3),
         "port": port, "recv_q": int(rows[0][1]), "send_q": int(rows[0][2]),
         "accept_count": accepted(server), "clients": {c["name"]: {
             "connected": connected(c), "echo": echoed(c), "exit_code": c["proc"].poll()
         } for c in clients}}
    SNAPS.append(x)
    (OUT / (label + ".netstat.txt")).write_text(
        "$ netstat -nltp（仅本次端口）\n" + listening
        + "\n$ netstat -ntop（仅本次端口，增加定时器）\n" + states
        + "\n$ ss -ntio（仅辅助记录 TCP 定时器/重传状态）\n" + extra)
    say(label + ": " + json.dumps(x, ensure_ascii=False))
    return x


def setup(work, tag):
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        port = s.getsockname()[1]
    server = spawn(tag + "_server", [str(work / "tcp_server"), str(port)], work)
    wait(lambda: "LISTEN" in netstat(port, "-nltp"), tag + " server listen")
    clients = []
    for i in range(4):
        c = spawn(tag + "_C" + str(i), [str(work / "tcp_client"), "127.0.0.1", str(port)], work)
        clients.append(c)
        wait(lambda c=c: connected(c), "已连接 " + c["name"])
        if i == 0:
            wait(lambda: accepted(server) == 1, "C0 已 accept")
        else:
            send(c, c["name"])
    full = snap(tag + "_01_full", port, server, clients)
    assert full["recv_q"] == 3 and full["accept_count"] == 1
    overflow = spawn(tag + "_C4", [str(work / "tcp_client"), "127.0.0.1", str(port)], work)
    clients.append(overflow)
    time.sleep(2.2)
    assert overflow["proc"].poll() is None and not connected(overflow)
    snap(tag + "_02_overflow_waiting", port, server, clients)
    return port, server, clients, overflow


def main():
    RESULT["source_hashes_before"] = hashes()
    RESULT["sysctl"] = run(["sysctl", "net.ipv4.tcp_syn_retries",
                            "net.ipv4.tcp_syn_linear_timeouts",
                            "net.ipv4.tcp_abort_on_overflow", "net.core.somaxconn"]).stdout
    RESULT["kernel"] = run(["uname", "-r"]).stdout.strip()
    say("继续实验，保持默认内核重试参数，不给原客户端添加应用层连接超时。")
    say(RESULT["sysctl"].strip())
    try:
        with tempfile.TemporaryDirectory(prefix="tcp-overflow-continuation-") as td:
            work = Path(td)
            for p in FILES:
                shutil.copy2(p, work / p.name)
            original = (work / "TcpServer.hpp").read_text()
            assert original.count("listen(_sockfd, 8)") == 1
            assert original.count("_local_addr(port)") == 1
            modified = original.replace("listen(_sockfd, 8)", "listen(_sockfd, 2)").replace(
                "_local_addr(port)", '_local_addr("127.0.0.1", port)')
            (work / "TcpServer.hpp").write_text(modified)
            (OUT / "temporary-source.diff").write_text("".join(difflib.unified_diff(
                original.splitlines(True), modified.splitlines(True),
                fromfile="original/TcpServer.hpp", tofile="temporary/TcpServer.hpp")))
            build = subprocess.run(["make", "-B"], cwd=work, text=True, capture_output=True)
            (OUT / "build.log").write_text(build.stdout + build.stderr)
            assert build.returncode == 0
            say("构建通过；阶段 A：让同一个溢出客户端等待，然后释放队列位置。")
            port, server, clients, overflow = setup(work, "A")
            RESULT["phases"]["A"] = {"port": port, "overflow_client": overflow["name"]}
            send(clients[0], "q")
            wait(lambda: echoed(clients[1]), "A_C1 被 accept 并获得回声")
            snap("A_03_slot_released", port, server, clients)
            wait(lambda: connected(overflow), "原溢出客户端自己完成 connect", timeout=20)
            send(overflow, overflow["name"])
            elapsed = time.monotonic() - overflow["spawn_time"]
            snap("A_04_same_client_connected", port, server, clients)
            RESULT["phases"]["A"]["connect_observed_after_seconds"] = elapsed
            for c in clients[1:]:
                wait(lambda c=c: echoed(c), c["name"] + " 获得回声")
                send(c, "q")
                wait(lambda c=c: c["proc"].poll() is not None, c["name"] + " 正常退出")
            final = snap("A_05_all_served", port, server, clients)
            assert final["accept_count"] == 5 and final["recv_q"] == 0
            connect_lines = [s for s in trace(overflow).splitlines() if re.search(r"\bconnect\(", s)]
            assert len(connect_lines) == 1 and re.search(r"= 0\s+<", connect_lines[0])
            RESULT["phases"]["A"]["connect_syscall"] = connect_lines[0]
            RESULT["phases"]["A"]["status"] = "passed"
            stop(server)
            say("阶段 A 通过：原来的阻塞 connect 在队列腾位后成功，未重新启动客户端。")

            say("阶段 B：队列始终保持满，等待原客户端的 connect 自然返回。")
            port, server, clients, overflow = setup(work, "B")
            RESULT["phases"]["B"] = {"port": port, "overflow_client": overflow["name"]}
            last_snap = 0
            while overflow["proc"].poll() is None:
                elapsed = time.monotonic() - overflow["spawn_time"]
                if elapsed > 240:
                    raise RuntimeError("240 秒观察上限已到，不能宣称已观察到内核最终超时")
                if elapsed - last_snap >= 15:
                    x = snap("B_wait_" + str(int(elapsed)), port, server, clients)
                    assert x["recv_q"] == 3 and x["accept_count"] == 1
                    last_snap = elapsed
                time.sleep(0.2)
            elapsed = time.monotonic() - overflow["spawn_time"]
            snap("B_03_connect_returned", port, server, clients)
            connect_lines = [s for s in trace(overflow).splitlines() if re.search(r"\bconnect\(", s)]
            assert len(connect_lines) == 1 and "ETIMEDOUT" in connect_lines[0]
            assert not connected(overflow)
            RESULT["phases"]["B"].update({"status": "passed", "seconds": elapsed,
                "client_exit_code": overflow["proc"].returncode,
                "connect_syscall": connect_lines[0], "application_timeout": False})
            RESULT["status"] = "passed"
            say("阶段 B 通过：" + connect_lines[0])
    except BaseException as exc:
        RESULT["status"] = "failed"
        RESULT["error"] = repr(exc)
        say("异常：" + repr(exc))
        raise
    finally:
        for x in reversed(PROCS):
            stop(x)
            if x["proc"].stdin:
                x["proc"].stdin.close()
            x["out"].close()
        RESULT["source_hashes_after"] = hashes()
        RESULT["original_sources_unchanged"] = RESULT["source_hashes_before"] == RESULT["source_hashes_after"]
        RESULT["all_tracers_exited"] = all(x["proc"].poll() is not None for x in PROCS)
        RESULT["listeners_removed"] = {tag: "LISTEN" not in netstat(p["port"], "-nltp")
                                       for tag, p in RESULT["phases"].items()}
        (OUT / "results.json").write_text(json.dumps(RESULT, ensure_ascii=False, indent=2))
        say("清理完成，原始源码哈希一致：" + str(RESULT["original_sources_unchanged"]))
        say("记录目录：" + str(OUT))
        LOG.close()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""临时复制 EchoServer，以 backlog=2 重现排队、溢出和 accept 后释放位置。"""
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
OUT = BASE / ("run_" + STAMP)
OUT.mkdir()
LOG = (OUT / "test.log").open("w", encoding="utf-8", buffering=1)
START = time.monotonic()
PROCESSES = []
CLIENTS = {}
SNAPSHOTS = []
SOURCES = sorted([*SOURCE.glob("*.hpp"), *SOURCE.glob("*.cc"), SOURCE / "Makefile"])


def say(message):
    line = f"[{time.monotonic() - START:7.3f}s] {message}"
    print(line, flush=True)
    LOG.write(line + "\n")


def hashes():
    return {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in SOURCES}


def run(command, **kwargs):
    return subprocess.run(command, capture_output=True, text=True, check=True, **kwargs)


def wait_for(predicate, description, seconds=5):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.04)
    raise RuntimeError("等待超时：" + description)


def text(path):
    return path.read_text(encoding="utf-8", errors="replace") if path.exists() else ""


def spawn(name, command, cwd):
    logpath = OUT / (name + ".stdout.log")
    handle = logpath.open("w", encoding="utf-8")
    proc = subprocess.Popen(
        ["strace", "-ff", "-ttt", "-T", "-s", "256", "-e",
         "trace=network,read,write,close", "-o", str(OUT / (name + ".strace")), *command],
        cwd=cwd, stdin=subprocess.PIPE, stdout=handle, stderr=subprocess.STDOUT,
        text=True, start_new_session=True,
    )
    item = {"name": name, "proc": proc, "handle": handle, "logpath": logpath}
    PROCESSES.append(item)
    return item


def stop(item):
    proc = item["proc"]
    if proc.poll() is None:
        os.killpg(proc.pid, signal.SIGTERM)
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGKILL)
            proc.wait(timeout=3)


def connected(item):
    return "Init service success!" in text(item["logpath"])


def echoed(item):
    return "echo #" + item["name"] + "_BACKLOG_2" in text(item["logpath"])


def sendline(item, message):
    item["proc"].stdin.write(message + "\n")
    item["proc"].stdin.flush()
    say(item["name"] + " 输入：" + message)


def accepts():
    trace = "\n".join(text(p) for p in OUT.glob("server.strace.*"))
    return len(re.findall(r"accept(?:4)?\([^\n]*\)\s+=\s+\d+", trace))


def listening(port):
    raw = run(["netstat", "-nltp"]).stdout
    kept = []
    for line in raw.splitlines():
        fields = line.split()
        if line.startswith("Active") or line.startswith("Proto"):
            kept.append(line)
        elif fields and fields[0].startswith("tcp") and fields[3].rsplit(":", 1)[-1] == str(port):
            kept.append(line)
    return "\n".join(kept) + "\n"


def snapshot(label, port):
    listener = listening(port)
    rows = [s.split() for s in listener.splitlines() if s.startswith("tcp")]
    assert len(rows) == 1, "应当只有本次实验的监听套接字"
    raw_sockets = run(["netstat", "-ntp"]).stdout
    sockets = "\n".join(line for line in raw_sockets.splitlines()
                        if f":{port} " in line or line.startswith("Proto"))
    secondary = run(["ss", "-ltnp", f"sport = :{port}"]).stdout
    states = {name: {"connect_returned": connected(item), "echo_received": echoed(item),
                     "tracer_exit_code": item["proc"].poll()} for name, item in CLIENTS.items()}
    result = {"label": label, "elapsed": round(time.monotonic() - START, 3),
              "recv_q": int(rows[0][1]), "send_q": int(rows[0][2]),
              "accept_count": accepts(), "clients": states}
    SNAPSHOTS.append(result)
    number = len(SNAPSHOTS)
    (OUT / f"{number:02d}_{label}.netstat.txt").write_text(
        "$ netstat -nltp（仅保留本次端口）\n" + listener
        + "\n$ netstat -ntp（仅保留本次端口）\n" + sockets
        + "\n\n$ ss -ltnp（仅辅助核对 backlog）\n" + secondary)
    say(f"{label}: netstat LISTEN Recv-Q={result['recv_q']}, Send-Q={result['send_q']}, "
        f"已返回的 accept={result['accept_count']}")
    say("客户端状态：" + json.dumps(states, ensure_ascii=False))
    return result


def main():
    before = hashes()
    environment = {"kernel": run(["uname", "-r"]).stdout.strip(), "source": str(SOURCE),
                   "local_time": dt.datetime.now(ZoneInfo("America/Los_Angeles")).isoformat(),
                   "sysctl": run(["sysctl", "net.core.somaxconn", "net.ipv4.tcp_max_syn_backlog",
                                  "net.ipv4.tcp_abort_on_overflow", "net.ipv4.tcp_syncookies"]).stdout}
    (OUT / "environment.json").write_text(json.dumps(environment, ensure_ascii=False, indent=2))
    say("测试源代码：" + str(SOURCE))
    say("环境：" + json.dumps(environment, ensure_ascii=False))
    result = {"status": "running", "source_hashes_before": before, "snapshots": SNAPSHOTS}
    try:
        with tempfile.TemporaryDirectory(prefix="tcp-backlog-2-") as temporary:
            work = Path(temporary)
            for p in SOURCES:
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
            build = subprocess.run(["make", "-B"], cwd=work, capture_output=True, text=True)
            (OUT / "build.log").write_text(build.stdout + build.stderr)
            assert build.returncode == 0, "编译失败，请查看 build.log"
            say("按原 Makefile 编译服务器和客户端成功；临时 backlog=2，仅监听 127.0.0.1")
            with socket.socket() as candidate:
                candidate.bind(("127.0.0.1", 0))
                port = candidate.getsockname()[1]
            result["port"] = port
            server = spawn("server", [str(work / "tcp_server"), str(port)], work)
            wait_for(lambda: "LISTEN" in listening(port), "监听套接字出现")
            assert server["proc"].poll() is None
            snapshot("S0_listen_no_clients", port)

            first = spawn("C0", [str(work / "tcp_client"), "127.0.0.1", str(port)], work)
            CLIENTS["C0"] = first
            wait_for(lambda: connected(first) and accepts() >= 1, "C0 已连接且被 accept")
            say("C0 被 accept 后保持空闲，服务端阻塞在处理 C0 的 read")
            snapshot("S1_C0_accepted_idle", port)

            for i in range(1, 6):
                name = "C" + str(i)
                item = spawn(name, [str(work / "tcp_client"), "127.0.0.1", str(port)], work)
                CLIENTS[name] = item
                deadline = time.monotonic() + 0.8
                while time.monotonic() < deadline and not connected(item):
                    if item["proc"].poll() is not None:
                        raise RuntimeError(name + " 提前退出，请查看日志")
                    time.sleep(0.04)
                if connected(item):
                    sendline(item, name + "_BACKLOG_2")
                    say(name + " connect 已成功，等待服务器 accept/echo")
                else:
                    say(name + " 在 0.8 秒观察期内 connect 尚未返回成功")
            full = snapshot("S2_queue_under_load", port)
            assert accepts() == 1, "C0 未退出时不能开始处理其他连接"
            queued = [x for name, x in CLIENTS.items() if name != "C0" and connected(x)]
            pending = [x for name, x in CLIENTS.items() if name != "C0" and not connected(x)]
            assert queued and pending, "本轮应同时观测到排队连接和未完成连接"
            assert all(not echoed(x) for x in queued), "排队客户端不应提前收到回声"
            say("补充 Python 客户端：connect 设置 2 秒应用层超时")
            with socket.socket() as probe:
                probe.settimeout(2)
                started = time.monotonic()
                try:
                    probe.connect(("127.0.0.1", port))
                    probe_result = "connect succeeded"
                except OSError as exc:
                    probe_result = type(exc).__name__ + ": " + str(exc)
                probe_elapsed = time.monotonic() - started
            say(f"Python probe: {probe_result}; 耗时 {probe_elapsed:.3f}s")
            result["probe"] = {"result": probe_result, "seconds": probe_elapsed}
            snapshot("S3_queue_still_full", port)
            for item in pending:
                stop(item)
                say(item["name"] + " 观察结束后由测试程序终止；不是记录为内核 connect 超时")

            sendline(first, "q")
            wait_for(lambda: first["proc"].poll() is not None, "C0 正常退出")
            wait_for(lambda: echoed(queued[0]), "首个排队客户端收到 echo")
            freed = snapshot("S4_accept_frees_slot", port)
            assert freed["recv_q"] < full["recv_q"]

            recovered = spawn("C6", [str(work / "tcp_client"), "127.0.0.1", str(port)], work)
            CLIENTS["C6"] = recovered
            wait_for(lambda: connected(recovered), "释放位置后 C6 connect 成功")
            sendline(recovered, "C6_BACKLOG_2")
            snapshot("S5_new_client_uses_freed_slot", port)
            for item in queued + [recovered]:
                wait_for(lambda item=item: echoed(item), item["name"] + " 收到正确回声")
                sendline(item, "q")
                wait_for(lambda item=item: item["proc"].poll() is not None,
                         item["name"] + " 正常退出")
            final = snapshot("S6_all_served", port)
            assert final["recv_q"] == 0
            assert final["accept_count"] == len(queued) + 2
            assert all(echoed(x) for x in queued + [recovered])
            result["status"] = "passed"
            result["queued_clients"] = [x["name"] for x in queued]
            result["pending_clients"] = [x["name"] for x in pending]
            result["accepted_total"] = final["accept_count"]
            say("所有阶段通过；用户原始客户端已实际收发并正常退出")
    except Exception as exc:
        result["status"] = "failed"
        result["error"] = repr(exc)
        say("测试失败：" + repr(exc))
        raise
    finally:
        for item in reversed(PROCESSES):
            stop(item)
            if item["proc"].stdin:
                item["proc"].stdin.close()
            item["handle"].close()
        result["source_hashes_after"] = hashes()
        result["original_sources_unchanged"] = before == result["source_hashes_after"]
        result["test_processes_stopped"] = all(x["proc"].poll() is not None for x in PROCESSES)
        if "port" in result:
            result["listener_removed"] = "LISTEN" not in listening(result["port"])
        (OUT / "results.json").write_text(json.dumps(result, ensure_ascii=False, indent=2))
        say("原始源码哈希一致：" + str(result["original_sources_unchanged"]))
        say("本次测试进程均已停止：" + str(result["test_processes_stopped"]))
        say("结果目录：" + str(OUT))
        LOG.close()


if __name__ == "__main__":
    main()

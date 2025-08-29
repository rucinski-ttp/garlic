import os
import sys
import time
import socket
import subprocess
from pathlib import Path
from shutil import which


def _which(name: str):
    return which(name)


class RTTCapture:
    def __init__(self, outfile: str, tool: str | None, channel: int):
        self.outfile = outfile
        self.tool = tool or "auto"
        self.channel = channel
        self.proc = None

    def start(self):
        script = Path("scripts") / "rtt_capture.py"
        if not script.exists():
            raise RuntimeError("scripts/rtt_capture.py not found")
        tool = self.tool
        if tool in (None, "auto"):
            if _which("JLinkGDBServer"):
                tool = "jlink"
            elif _which("JLinkRTTLogger"):
                tool = "jlink-logger"
            else:
                tool = "auto"
        self.tool = tool
        cmd = [sys.executable or "python3", str(script), "--tool", tool, "--outfile", self.outfile]
        if self.channel is not None:
            cmd += ["--channel", str(self.channel)]
        self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        time.sleep(0.8)

    def stop(self):
        if self.proc is not None:
            try:
                self.proc.terminate()
                self.proc.wait(timeout=1.0)
            except Exception:
                pass

    def reset_board(self):
        # J-Link GDBServer reset if available; otherwise discrete JLinkExe reset
        if self.tool == "jlink" and _which("JLinkGDBServer"):
            def _find_gdb():
                for name in ("arm-none-eabi-gdb", "arm-zephyr-eabi-gdb", "gdb-multiarch"):
                    if _which(name):
                        return name
                return None
            gdb = _find_gdb()
            if gdb:
                deadline = time.time() + 3.0
                while time.time() < deadline:
                    try:
                        with socket.create_connection(("127.0.0.1", 2331), timeout=0.3):
                            cmd = [gdb, "-q", "-batch", "-ex", "set confirm off", "-ex", "set pagination off",
                                   "-ex", "target remote :2331", "-ex", "monitor reset", "-ex", "monitor go", "-ex", "quit"]
                            subprocess.run(cmd, check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                            return
                    except Exception:
                        time.sleep(0.1)
        if _which("JLinkExe"):
            script = "device NRF52832_XXAA\nif SWD\nspeed 4000\nconnect\nr\ng\nq\n"
            subprocess.run(["JLinkExe"], input=script.encode(), check=False)

    def expect_in_log(self, pattern: str, timeout: float = 5.0) -> bool:
        deadline = time.time() + timeout
        while time.time() < deadline:
            if os.path.exists(self.outfile):
                try:
                    text = Path(self.outfile).read_text(errors="ignore")
                    if pattern in text:
                        return True
                except Exception:
                    pass
            time.sleep(0.2)
        return False

    def read(self) -> str:
        try:
            return Path(self.outfile).read_text(errors="ignore")
        except Exception:
            return ""


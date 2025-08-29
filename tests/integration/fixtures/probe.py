import os
import subprocess
from shutil import which


def _which(name: str):
    return which(name)


class DebugProbe:
    def __init__(self):
        self.backend = None
        if _which("JLinkExe"):
            self.backend = "jlink"
        else:
            self.backend = None

    def available(self) -> bool:
        return self.backend is not None

    def reset_run(self):
        if self.backend == "jlink":
            script = "device NRF52832_XXAA\nif SWD\nspeed 4000\nconnect\nr\ng\nq\n"
            subprocess.run(["JLinkExe"], input=script.encode(), check=False)

    def reset_halt(self):
        if self.backend == "jlink":
            script = "device NRF52832_XXAA\nif SWD\nspeed 4000\nconnect\nr\nh\nq\n"
            subprocess.run(["JLinkExe"], input=script.encode(), check=False)

    def go(self):
        if self.backend == "jlink":
            script = "device NRF52832_XXAA\nif SWD\nspeed 4000\nconnect\ng\nq\n"
            subprocess.run(["JLinkExe"], input=script.encode(), check=False)


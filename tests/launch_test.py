import time
import pytest

from engine_tools import engine_executable

class TestLaunch:

    def test_basic_launch(self, get_engine : engine_executable.EngineExecutable):
        engine = get_engine
        engine.start()
        time.sleep(5)
        engine.stop()
        
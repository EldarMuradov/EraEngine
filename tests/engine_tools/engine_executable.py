import os
import subprocess


def get_engine_path():
    dir_path = os.path.dirname(os.path.realpath(__file__))
    return os.path.join(dir_path, "..", "..")


def run_engine(app_name, args, config='Release'):
    engine_path = os.path.join(get_engine_path(), "_build", "apps", app_name, config, f"{app_name}.exe")
    if args:
        proc = subprocess.Popen([engine_path] + args)
    else:
        proc = subprocess.Popen([engine_path])
    return proc


class EngineExecutable:

    def __init__(self, app_name, args = [], config = 'Release'):
        self.app_name = app_name
        self.config = config
        self.proc = None
        self.args = args

    def start(self):
        self.proc = run_engine(self.app_name, self.args, self.config)

    def stop(self):
        self.proc.terminate()
        assert not self.proc.returncode

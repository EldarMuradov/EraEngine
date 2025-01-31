import pytest

from engine_tools import *
from tests.engine_tools import engine_executable

def pytest_addoption(parser):
    parser.addoption('--app_name', default='editor', help='Running app name')
    parser.addoption('--config', default='Debug', help='Running app Config')


@pytest.fixture(scope='session')
def get_app_name(request):
    return request.config.getoption('--app_name')


@pytest.fixture(scope='session')
def get_app_configuration(request):
    return request.config.getoption('--config')


@pytest.fixture(scope='session')
def get_engine(get_app_name, get_app_configuration):
    engine = engine_executable.EngineExecutable(get_app_name, ['--verbose=1'], get_app_configuration)
    return engine

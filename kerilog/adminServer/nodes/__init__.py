from os.path import dirname, basename, isfile
import glob
import importlib
_modules = glob.glob(dirname(__file__)+"/*.py")
_modules = [ basename(_f)[:-3] for _f in _modules if isfile(_f) and not _f.endswith('__init__.py')]
# we have the list of modules by now

CONFIG = {}

for each in _modules:
    node_id = each[2:]
    cfg = importlib.import_module("nodes." + each).CONFIG
    CONFIG[cfg.pop("node_id")] = cfg

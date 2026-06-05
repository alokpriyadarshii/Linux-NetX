from importlib import metadata

try:
    __version__ = metadata.version("Linux-Net")
except metadata.PackageNotFoundError:
    __version__ = "0.4.3"

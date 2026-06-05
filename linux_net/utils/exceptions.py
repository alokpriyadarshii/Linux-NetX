# Custom exceptions for Linux-Net
class LinuxNetError(Exception):
    """Base class for Linux-Net-specific exceptions"""

    pass


class WorkerUnavailableError(LinuxNetError):
    """
    Raised when no worker is available to handle the job, could be:
    - Scheduler is unable to find a suitable node to assign a pinned worker
    - No FIFO worker is available to handle the FIFO job
    """

    pass


class JobOperationError(LinuxNetError):
    """Raised when a job operation fails"""

    pass


# These exceptions are used in the Worker class only,
# as they are raised in execution nodes instead of the controller.
class LinuxNetWorkerError(LinuxNetError):
    """Base class for exceptions raised by Linux-Net workers"""

    pass


class HostAlreadyPinnedError(LinuxNetWorkerError):
    """Raised when a host is already pinned to a worker."""

    pass


class NodePreemptedError(LinuxNetWorkerError):
    """Raised when a node is preempted by another pinned worker."""

    pass

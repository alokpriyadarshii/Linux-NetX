import unittest
from unittest.mock import MagicMock, patch

from linux_net.models.common import DriverName
from linux_net.models.request import ExecutionRequest, TemplateRenderRequest
from linux_net.plugins.drivers.paramiko.model import (
    ParamikoConnectionArgs,
    ParamikoSendCommandArgs,
    ParamikoSendConfigArgs,
)
from linux_net.services import rpc


class TestParamikoJinja2(unittest.TestCase):
    def setUp(self):
        self.conn_args = ParamikoConnectionArgs(host="1.1.1.1", username="test", password="test")

    @patch("linux_net.plugins.drivers.paramiko.ParamikoDriver.connect")
    @patch("linux_net.plugins.drivers.paramiko.ParamikoDriver.disconnect")
    @patch("paramiko.SSHClient")
    def test_rendering_in_send(self, mock_ssh, mock_disconnect, mock_connect):
        # Mock the session
        mock_session = mock_ssh.return_value
        mock_connect.return_value = mock_session

        mock_stdout = MagicMock()
        mock_stdout.read.return_value = b"bar\n"
        mock_stdout.channel.recv_exit_status.return_value = 0
        mock_stderr = MagicMock()
        mock_stderr.read.return_value = b""

        mock_session.exec_command.return_value = (MagicMock(), mock_stdout, mock_stderr)

        req = ExecutionRequest(
            driver=DriverName.PARAMIKO,
            connection_args=self.conn_args,
            command="echo {{ foo }}",
            driver_args=ParamikoSendCommandArgs(),
            rendering=TemplateRenderRequest(name="jinja2", context={"foo": "bar"}),
        )

        # Test rendering through rpc.execute
        result = rpc.execute(req)

        # Verify exec_command was called with rendered string
        mock_session.exec_command.assert_called_with("echo bar", get_pty=False)
        self.assertEqual(result[0].command, "echo bar")
        self.assertEqual(result[0].stdout, "bar\n")

    @patch("linux_net.plugins.drivers.paramiko.ParamikoDriver.connect")
    @patch("linux_net.plugins.drivers.paramiko.ParamikoDriver.disconnect")
    @patch("paramiko.SSHClient")
    def test_rendering_in_config(self, mock_ssh, mock_disconnect, mock_connect):
        # Mock the session
        mock_session = mock_ssh.return_value
        mock_connect.return_value = mock_session

        mock_stdout = MagicMock()
        mock_stdout.read.return_value = b"success\n"
        mock_stdout.channel.recv_exit_status.return_value = 0
        mock_stderr = MagicMock()
        mock_stderr.read.return_value = b""
        mock_session.exec_command.return_value = (MagicMock(), mock_stdout, mock_stderr)

        req = ExecutionRequest(
            driver=DriverName.PARAMIKO,
            connection_args=self.conn_args,
            config=["vlan {{ vlan_id }}"],
            driver_args=ParamikoSendConfigArgs(),
            rendering=TemplateRenderRequest(name="jinja2", context={"vlan_id": 100}),
        )

        # Test rendering through rpc.execute
        result = rpc.execute(req)

        # Verify rendering
        mock_session.exec_command.assert_called_with("vlan 100", get_pty=False)
        self.assertEqual(result[0].command, "vlan 100")


if __name__ == "__main__":
    unittest.main()

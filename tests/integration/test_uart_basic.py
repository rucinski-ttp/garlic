"""
Basic UART functionality tests for Garlic device.

Medical device grade testing for serial communication.
"""

import pytest
import time
import logging

logger = logging.getLogger(__name__)


@pytest.mark.hardware
class TestUartBasic:
    """Basic UART communication tests."""
    
    def test_device_connection(self, serial_garlic_device):
        """Test that we can connect to the device."""
        assert serial_garlic_device.serial is not None
        assert serial_garlic_device.serial.is_open
        
        # Get and log device info
        info = serial_garlic_device.get_device_info()
        logger.info(f"Connected to: {info}")
        
    def test_device_startup_message(self, serial_garlic_device):
        """Test that device sends startup messages."""
        # Clear any existing data
        serial_garlic_device.flush_input()
        
        # Wait for startup messages (device sends status every second)
        found = serial_garlic_device.wait_for_pattern("Status:", timeout=2.0)
        assert found, "No status message received from device"
        
    def test_echo_functionality(self, serial_garlic_device):
        """Test that device echoes back sent data."""
        test_message = b"Hello Garlic!\n"
        
        # Clear buffers
        serial_garlic_device.flush_input()
        
        # Send test message
        bytes_written = serial_garlic_device.write(test_message)
        assert bytes_written == len(test_message)
        
        # Wait for echo response
        found = serial_garlic_device.wait_for_pattern("Echo: Hello Garlic!", timeout=2.0)
        assert found, "Echo response not received"
        
    def test_multiple_echo_messages(self, serial_garlic_device):
        """Test multiple echo messages in sequence."""
        messages = [
            b"Test1\n",
            b"Test2\n", 
            b"Test3\n"
        ]
        
        for msg in messages:
            serial_garlic_device.flush_input()
            serial_garlic_device.write(msg)
            
            expected = f"Echo: {msg.decode().strip()}"
            found = serial_garlic_device.wait_for_pattern(expected, timeout=2.0)
            assert found, f"Echo not received for message: {msg}"
            
    def test_status_message_format(self, serial_garlic_device):
        """Test that status messages have correct format."""
        serial_garlic_device.flush_input()
        
        # Wait for a complete status line
        timeout = time.time() + 3.0
        status_line = None
        
        while time.time() < timeout:
            line = serial_garlic_device.read_line()
            if line.startswith("[") and "Status:" in line:
                status_line = line
                break
                
        assert status_line is not None, "No status message received"
        
        # Verify format: [NNNN] Status: LED=XXX | TX=N bytes | RX=N bytes | ...
        assert "[" in status_line
        assert "]" in status_line
        assert "Status:" in status_line
        assert "LED=" in status_line
        assert "TX=" in status_line
        assert "RX=" in status_line
        assert "bytes" in status_line
        
    def test_status_message_timing(self, serial_garlic_device):
        """Test that status messages arrive at 1Hz."""
        serial_garlic_device.flush_input()
        
        # Collect timestamps of status messages
        timestamps = []
        start_time = time.time()
        timeout = start_time + 5.0  # Collect for 5 seconds
        
        while time.time() < timeout and len(timestamps) < 5:
            line = serial_garlic_device.read_line()
            if line.startswith("[") and "Status:" in line:
                timestamps.append(time.time())
                
        assert len(timestamps) >= 4, f"Only received {len(timestamps)} status messages in 5 seconds"
        
        # Check intervals (should be ~1 second)
        intervals = [timestamps[i+1] - timestamps[i] for i in range(len(timestamps)-1)]
        
        for interval in intervals:
            assert 0.8 <= interval <= 1.3, f"Status interval {interval}s outside expected range"
            
    def test_led_state_in_status(self, serial_garlic_device):
        """Test that LED state toggles in status messages."""
        serial_garlic_device.flush_input()
        
        # Collect several status messages
        led_states = []
        timeout = time.time() + 5.0
        
        while time.time() < timeout and len(led_states) < 5:
            line = serial_garlic_device.read_line()
            if "LED=" in line:
                if "LED=ON" in line:
                    led_states.append("ON")
                elif "LED=OFF" in line:
                    led_states.append("OFF")
                    
        assert len(led_states) >= 4, "Insufficient LED state readings"
        
        # Verify we see both states
        assert "ON" in led_states, "Never saw LED ON state"
        assert "OFF" in led_states, "Never saw LED OFF state"
        
    def test_tx_rx_counters(self, serial_garlic_device):
        """Test that TX/RX byte counters increment correctly."""
        serial_garlic_device.flush_input()
        
        # Get initial counter values
        initial_tx = None
        initial_rx = None
        
        timeout = time.time() + 3.0
        while time.time() < timeout:
            line = serial_garlic_device.read_line()
            if "TX=" in line and "RX=" in line:
                # Parse TX value
                tx_start = line.find("TX=") + 3
                tx_end = line.find(" bytes", tx_start)
                initial_tx = int(line[tx_start:tx_end])
                
                # Parse RX value
                rx_start = line.find("RX=") + 3
                rx_end = line.find(" bytes", rx_start)
                initial_rx = int(line[rx_start:rx_end])
                break
                
        assert initial_tx is not None, "Could not get initial TX counter"
        assert initial_rx is not None, "Could not get initial RX counter"
        
        # Send some data
        test_data = b"Test data for counters\n"
        serial_garlic_device.write(test_data)
        time.sleep(2)  # Wait for processing and next status
        
        # Get new counter values
        new_tx = None
        new_rx = None
        
        timeout = time.time() + 3.0
        while time.time() < timeout:
            line = serial_garlic_device.read_line()
            if "TX=" in line and "RX=" in line:
                # Parse TX value
                tx_start = line.find("TX=") + 3
                tx_end = line.find(" bytes", tx_start)
                new_tx = int(line[tx_start:tx_end])
                
                # Parse RX value
                rx_start = line.find("RX=") + 3
                rx_end = line.find(" bytes", rx_start)
                new_rx = int(line[rx_start:rx_end])
                break
                
        assert new_tx is not None, "Could not get new TX counter"
        assert new_rx is not None, "Could not get new RX counter"
        
        # TX should increase (device sends echo and status messages)
        assert new_tx > initial_tx, f"TX counter did not increase: {initial_tx} -> {new_tx}"
        
        # RX should increase by at least the size of our test data
        rx_increase = new_rx - initial_rx
        assert rx_increase >= len(test_data), \
            f"RX counter increase {rx_increase} less than sent data size {len(test_data)}"
            
    def test_buffer_status_in_messages(self, serial_garlic_device):
        """Test that buffer status is reported correctly."""
        serial_garlic_device.flush_input()
        
        # Wait for status message with buffer info
        timeout = time.time() + 3.0
        found_buffer_info = False
        
        while time.time() < timeout:
            line = serial_garlic_device.read_line()
            if "TX_free=" in line and "RX_avail=" in line:
                found_buffer_info = True
                
                # Parse TX free space
                tx_free_start = line.find("TX_free=") + 8
                tx_free_end = line.find(" ", tx_free_start)
                if tx_free_end == -1:
                    tx_free_end = line.find("\r", tx_free_start)
                if tx_free_end == -1:
                    tx_free_end = len(line)
                    
                tx_free = int(line[tx_free_start:tx_free_end])
                
                # TX free should be substantial (buffer is 1024)
                assert tx_free > 0, "TX buffer reports no free space"
                assert tx_free <= 1024, f"TX free space {tx_free} exceeds buffer size"
                
                break
                
        assert found_buffer_info, "No buffer status information received"
        
    def test_continuous_data_stream(self, serial_garlic_device):
        """Test sending a continuous stream of data."""
        serial_garlic_device.flush_input()
        
        # Send multiple chunks without waiting
        total_sent = 0
        for i in range(10):
            chunk = f"Chunk {i:03d}\n".encode()
            bytes_written = serial_garlic_device.write(chunk)
            assert bytes_written == len(chunk)
            total_sent += bytes_written
            
        # Wait a bit for processing
        time.sleep(2)
        
        # Verify we got echo responses
        echo_count = 0
        timeout = time.time() + 3.0
        
        while time.time() < timeout:
            line = serial_garlic_device.read_line()
            if line.startswith("Echo: Chunk"):
                echo_count += 1
                
        assert echo_count >= 8, f"Only received {echo_count} echoes out of 10 chunks"
import pytest
pytestmark = pytest.mark.hardware

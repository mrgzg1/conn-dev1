#!/usr/bin/env python3
"""
Setup script for RP2040 MicroPython data logger
"""
import os
import sys
import time
import platform
import argparse
import subprocess
from pathlib import Path

def download_firmware(output_path="rp2-pico-w-latest.uf2"):
    """Download the latest MicroPython firmware"""
    import requests
    
    print("Downloading latest MicroPython firmware...")
    url = "https://micropython.org/download/rp2-pico-w/rp2-pico-w-latest.uf2"
    
    try:
        response = requests.get(url, stream=True)
        response.raise_for_status()
        
        with open(output_path, 'wb') as f:
            for chunk in response.iter_content(chunk_size=8192):
                f.write(chunk)
                
        print(f"Firmware downloaded to {output_path}")
        return True
    except Exception as e:
        print(f"Error downloading firmware: {e}")
        return False

def detect_device_port():
    """Detect the device port"""
    system = platform.system()
    
    if system == "Darwin":  # macOS
        import glob
        ports = glob.glob("/dev/tty.usbmodem*") + glob.glob("/dev/tty.SLAB_USBtoUART*")
        if ports:
            return ports[0]
    elif system == "Linux":
        import glob
        ports = glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*")
        if ports:
            return ports[0]
    elif system == "Windows":
        # Windows port detection requires more work, possibly using pyserial
        pass
    
    return None

def upload_files(port, files):
    """Upload files to the device using ampy"""
    if not port:
        print("Error: No device port specified or detected")
        return False
    
    print(f"Uploading files to {port}...")
    success = True
    
    for file_path in files:
        file_path = Path(file_path)
        if not file_path.exists():
            print(f"Error: File {file_path} does not exist")
            success = False
            continue
            
        # Determine destination path on device
        if file_path.name in ["boot.py", "main.py"]:
            dest_path = file_path.name
        else:
            dest_path = file_path.name
            
        cmd = ["ampy", "--port", port, "put", str(file_path), dest_path]
        print(f"Running: {' '.join(cmd)}")
        
        try:
            result = subprocess.run(cmd, check=True, capture_output=True, text=True)
            print(f"Uploaded {file_path} to {dest_path}")
        except subprocess.CalledProcessError as e:
            print(f"Error uploading {file_path}: {e}")
            print(f"stdout: {e.stdout}")
            print(f"stderr: {e.stderr}")
            success = False
    
    return success

def main():
    parser = argparse.ArgumentParser(description="RP2040 MicroPython setup tool")
    parser.add_argument("--download", action="store_true", help="Download the latest firmware")
    parser.add_argument("--port", help="Device port (auto-detect if not specified)")
    parser.add_argument("--upload", action="store_true", help="Upload files to the device")
    
    args = parser.parse_args()
    
    if args.download:
        download_firmware()
    
    if args.upload:
        port = args.port or detect_device_port()
        if not port:
            print("Error: No device port specified or detected")
            return 1
            
        files = [
            "boot.py",
            "main.py",
            "wifi_manager.py",
            "data_logger.py",
            "flash_storage.py",
            "sensors.py"
        ]
        
        upload_files(port, files)
    
    if not args.download and not args.upload:
        parser.print_help()
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
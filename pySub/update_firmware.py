# ESP flash script to update Nina WiFi firmware
firmware_path = "firmware.bin"

print("Starting firmware update...")
print(f"Using firmware file: {firmware_path}")

import espflash
flasher = espflash.ESPFlash()
flasher.flash(firmware_path, md5sum=None)

print("Firmware update completed!")
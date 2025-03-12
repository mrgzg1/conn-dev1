import espflash

print("ESPFlash class info:")
help(espflash.ESPFlash)

# Try to instantiate with default arguments
try:
    flash = espflash.ESPFlash()
    print("Successfully created ESPFlash instance with default arguments")
except Exception as e:
    print(f"Error creating ESPFlash instance: {e}")
    
print("\nESPFlash version:", getattr(espflash, "__version__", "unknown"))
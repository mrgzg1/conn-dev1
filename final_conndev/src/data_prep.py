# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "jsmin",
#     "htmlmin",
# ]
# ///
from jsmin import jsmin
from htmlmin import minify as htmlmin
import gzip
import os

def clean_content(content, file_type):
    # First minify the content
    if file_type == 'js':
        try:
            content = jsmin(content)
        except Exception as e:
            print(f"Warning: JS minification failed ({e}), using original content")
    elif file_type == 'html':
        try:
            content = htmlmin(content, remove_empty_space=True, remove_comments=True)
        except Exception as e:
            print(f"Warning: HTML minification failed ({e}), using original content")
    
    # Debug: Show first bit of content pre-compression
    print(f"\nFirst 100 chars of minified content:")
    print(content[:100])
    
    # Compress with gzip
    compressed = gzip.compress(content.encode('utf-8'), 
                             compresslevel=9,
                             mtime=None)
    
    print(f"\nSize stats:")
    print(f"Original: {len(content)} bytes")
    print(f"Compressed: {len(compressed)} bytes")
    
    return compressed

# Files to process
raw_files = ["index.html", "react_app.js", "chart.js"]

with open("src/web_files.h", "w", encoding='utf-8') as f:
    f.write("#ifndef WEB_FILES_H\n#define WEB_FILES_H\n\n")
    f.write("#include <Arduino.h>\n\n")
    
    for file in raw_files:
        print(f"\nProcessing {file}...")
        file_path = os.path.join("src/web", file)
        
        # Check if file exists
        if not os.path.exists(file_path):
            print(f"Warning: File {file_path} not found, skipping")
            continue
            
        with open(file_path, "r", encoding='utf-8') as src:
            content = src.read()
            
            # Process the file
            try:
                compressed = clean_content(content, file.split('.')[-1])
                var_name = file.replace('.', '_')
                
                # Write the size constant first
                f.write(f"const size_t {var_name}_len = {len(compressed)};\n")
                
                # Write the data array
                f.write(f"const uint8_t {var_name}[] PROGMEM = {{\n")
                
                # Format the bytes in a readable way, 12 bytes per line
                bytes_per_line = 12
                for i in range(0, len(compressed), bytes_per_line):
                    chunk = compressed[i:i+bytes_per_line]
                    hex_bytes = [f"0x{b:02x}" for b in chunk]
                    f.write("  " + ", ".join(hex_bytes) + ",\n")
                
                f.write("};\n\n")
                
            except Exception as e:
                print(f"Error processing {file}: {e}")
                continue
    
    f.write("#endif // WEB_FILES_H\n")

print("web_files.h generated successfully!")
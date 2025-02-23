# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "jsmin",
#     "htmlmin",
# ]
# ///
from jsmin import jsmin
from htmlmin import minify as htmlmin
import gzip

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
    
    # Convert compressed bytes to C array format
    c_array = ", ".join([f"0x{b:02x}" for b in compressed])
    
    print(f"\nSize stats:")
    print(f"Original: {len(content)} bytes")
    print(f"Compressed: {len(compressed)} bytes")
    
    return f"{{ {c_array} }}"

# Files to process
raw_files = ["index.html", "react_app.js", "chart.js"]

with open("web_files.h", "w", encoding='utf-8') as f:
    f.write("#ifndef WEB_FILES_H\n#define WEB_FILES_H\n\n")
    f.write("#include <Arduino.h>\n")
    
    for file in raw_files:
        print(f"\nProcessing {file}...")
        with open(f"web/{file}", "r", encoding='utf-8') as src:
            content = src.read()
            
            # Process the file
            try:
                processed = clean_content(content, file.split('.')[-1])
                var_name = file.replace('.', '_')
                f.write(f"const uint8_t {var_name}[] PROGMEM = {processed};\n")
                f.write(f"const size_t {var_name}_len = sizeof({var_name});\n\n")
            except Exception as e:
                print(f"Error processing {file}: {e}")
                continue

    f.write("#endif\n")

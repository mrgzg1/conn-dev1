# Files that should be stored as raw strings
raw_files = ["index.html", "react_app.js", "chart.js"]

with open("web_files.h", "w", encoding='utf-8') as f:
    f.write("#ifndef WEB_FILES_H\n#define WEB_FILES_H\n\n")
    
    # Handle all files as raw strings
    for file in raw_files:
        with open(f"web/{file}", "r", encoding='utf-8') as src:
            content = src.read()
            # Remove any potential null bytes or invalid characters
            content = ''.join(char for char in content if ord(char) >= 32 or char in '\n\r\t')
            # Escape backslashes first, then quotes and newlines
            content = content.replace('\\', '\\\\')  # Escape backslashes first!
            content = content.replace('"', '\\"')
            content = content.replace('\n', '\\n')
            var_name = file.replace('.', '_')
            f.write(f"const char {var_name}[] = \"{content}\";\n\n")

    f.write("#endif\n")
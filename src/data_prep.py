# Files that should be stored as raw strings
raw_files = ["index.html", "react_app.js", "chart.js"]

with open("web_files.h", "w") as f:
    f.write("#ifndef WEB_FILES_H\n#define WEB_FILES_H\n\n")
    
    # Handle all files as raw strings
    for file in raw_files:
        with open(f"web/{file}", "r") as src:
            content = src.read()
            # Escape backslashes first, then quotes and newlines
            content = content.replace('\\', '\\\\')  # Escape backslashes first!
            content = content.replace('"', '\\"').replace('\n', '\\n')
            var_name = file.replace('.', '_')
            f.write(f"const char {var_name}[] = \"{content}\";\n")

    f.write("#endif")
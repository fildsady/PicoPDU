"""Convert an HTML file to a C header with a static const char array."""
import sys, os

src  = sys.argv[1]
dst  = sys.argv[2]

with open(src, 'r', encoding='utf-8') as f:
    content = f.read()

lines = content.split('\n')
# Drop trailing empty line if present
if lines and lines[-1] == '':
    lines = lines[:-1]

with open(dst, 'w', encoding='utf-8') as f:
    f.write('#pragma once\n')
    f.write('static const char s_html[] =\n')
    for line in lines:
        escaped = line.replace('\\', '\\\\').replace('"', '\\"')
        f.write(f'  "{escaped}\\n"\n')
    f.write(';\n')

print(f"html2c: {os.path.basename(src)} -> {os.path.basename(dst)} ({len(content)} bytes)")

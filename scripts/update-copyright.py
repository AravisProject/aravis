#! /bin/env python3

import os
import re

# Define the new header content
new_header_content = """ * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>"""

# Define the pattern to match the old header content up to the Author(s) line
pattern = re.compile(r'^.*Copyright.*Pacaud.*$', re.MULTILINE)

def replace_header_content(file_path):
    with open(file_path, 'r') as file:
        content = file.read()

    # Replace the old header content with the new header content
    new_content = pattern.sub(new_header_content, content)

    with open(file_path, 'w') as file:
        file.write(new_content)

def process_directory(directory):
    for filename in os.listdir(directory):
        print(filename)
        if filename.endswith('.h') or filename.endswith('.c'):
            file_path = os.path.join(directory, filename)
            replace_header_content(file_path)
            print(f'Processed file: {file_path}')

if __name__ == '__main__':
    process_directory(".")

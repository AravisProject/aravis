#! /bin/env python3

import os
import re

# Define the new header content
new_header_content = """/* Aravis - Digital camera library
 *
 * Copyright © 2023 Václav Šmilauer
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author"""

# Define the pattern to match the old header content up to the Author(s) line
pattern = re.compile(r'/\*.*?\* Author', re.DOTALL)

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

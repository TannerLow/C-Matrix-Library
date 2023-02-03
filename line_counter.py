# Script for calculating line count of source code

import os
from datetime import date

excludes = ['.git', '.vscode', 'OpenCL']
interests = ['c', 'h']
language = 'C'

def get_interested_files(path):
    output = []
    everything = os.listdir(path)

    for thing in everything:
        new_path = os.path.join(path, thing)

        if thing not in excludes:
            if os.path.isfile(new_path):
                tokens = thing.split('.')
                extension = tokens[len(tokens)-1]
                if extension in interests:
                    output.append(new_path)
            else:
                sub_files = get_interested_files(new_path)
                for file in sub_files:
                    output.append(file)

    return output


def get_line_count(filepath):
    count = 0

    with open(filepath) as file:
        count = len(file.readlines())

    return count


filepaths = get_interested_files('.')
total = 0
for filepath in filepaths:
    total += get_line_count(filepath)

# Read readme and replace stat line with new data
readme_text = ""
with open("README.md", 'r', encoding="utf-8") as file:
    readme_text = file.read()

identifier = "## Total lines of " + language + " source code and headers as of "
if identifier in readme_text:
    readme_text_tokens = readme_text.split(identifier)
else:
    readme_text += '\n' + identifier + "lines"
    readme_text_tokens = readme_text.split(identifier)
dynamic_text = str(date.today()) + " = " + str(total) + " lines"
remainder = readme_text_tokens[1].split("lines", 1)[1]
readme_text = readme_text_tokens[0] + identifier + dynamic_text + remainder

with open("README.md", 'w', encoding="utf-8") as file:
    file.write(readme_text)

print("Total lines of C source code and headers as of", date.today(), '=', total, "lines")
print("==[ README Updated ]==")
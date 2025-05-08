import re

def slugify(text):
    # Lowercase
    text = text.lower()
    # Remove punctuation (except dashes)
    text = re.sub(r"[^\w\s-]", "", text)
    # Replace spaces with dashes
    text = re.sub(r"\s+", "-", text)
    return "#" + text

def extract_headings(md_lines):
    toc = []
    for line in md_lines:
        if line.startswith("# "):  # Only top-level headings
            title = line[2:].strip()
            anchor = slugify(title)
            toc.append(f"- [{title}]({anchor})")
    return toc

# Read your Markdown file
with open("README.md", "r") as f:
    lines = f.readlines()

# Generate TOC
toc = extract_headings(lines)

# Output TOC
print("# Table of Contents\n")
for line in toc:
    print(line)

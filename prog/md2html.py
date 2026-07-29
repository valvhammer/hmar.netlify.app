#!/usr/bin/env python3

import sys
import os
import json
import subprocess

SCRIPT_DIR = os.path.dirname(__file__)

def emit_str(data):
    print(data, end="")

def emit_space():
    print(" ", end="")

def emit_header(data, level: int):
    print(f"<h{level}>", end="")
    for section in data:
        if section['t'] == "Str": emit_str(section['c'])
        elif section['t'] == "Space": emit_space()
    print(f"</h{level}>", end="")
    
def emit_link(href: str, data: str):
    print(f"<a href=\"{href[0]}\" target=\"_blank\">", end="")
    for section in data:
        if section['t'] == "Str": emit_str(section['c'])
        elif section['t'] == "Space": emit_space()
    print(f"</a>", end="")
    
def emit_image(href: str, data: str):
    print(f"<img src=\"{href[0]}\" alt=\"", end="")
    for section in data:
        if section['t'] == "Str": emit_str(section['c'])
        elif section['t'] == "Space": emit_space()
    print(f"\"/>", end="")

# "t": "Strong" according to pandoc
def emit_bold(data: str):
    print(f"<b>", end="")
    for section in data:
        if section['t'] == "Str": emit_str(section['c'])
        elif section['t'] == "Space": emit_space()
    print("</b>", end="")

# "t": "Emph" according to pandoc
def emit_italic(data: str):
    print(f"<i>", end="")
    for section in data:
        if section['t'] == "Str": emit_str(section['c'])
        elif section['t'] == "Space": emit_space()
    print("</i>", end="")

def emit_strikethough(data: str):
    print(f"<del>", end="")
    for section in data:
        if section['t'] == "Str": emit_str(section['c'])
        elif section['t'] == "Space": emit_space()
    print("</del>", end="")

def emit_raw_inline(data: str):
    print(data[1], end="")
    
def emit_raw_block(data: str):
    print(data[1], end="")
    
def emit_small_code(data):
    print(f"<code>{data[1]}</code>")

def emit_para(data):
    print("<p>", end="")
    for section in data:
        if section['t'] == "Link": emit_link(section['c'][2], section['c'][1])
        elif section['t'] == "Image": emit_image(section['c'][2], section['c'][1])
        elif section['t'] == "Strong": emit_bold(section['c'])
        elif section['t'] == "Emph": emit_italic(section['c'])
        elif section['t'] == "Strikeout": emit_strikethough(section['c'])
        elif section['t'] == "RawInline": emit_raw_inline(section['c'])
        elif section['t'] == "Code": emit_small_code(section['c'])
        elif section['t'] == "Str": emit_str(section['c'])
        elif section['t'] == "Space": emit_space()
    print("</p>", end="")

def emit_code_block(data: str, lang: str):
    actual_lang = ""
    if lang == "c" or lang == "b" or lang == "glsl":
        actual_lang = "c"
    else:
        print(f"currently unsupported language {lang}");
        exit(69)
        
    with open("md2html.temp", "w") as f:
        f.write(data)
   
    result = subprocess.run([f"{SCRIPT_DIR}/code2html", "-input", "md2html.temp", "-output", "md2html_2.temp"])
    if result.returncode != 0:
        print(f"code2html failed")
        exit(69)
    
    with open("md2html_2.temp", "r") as f:
        print("<div class=\"code-block\">")
        print("<code>")
        print(f.read())
        print("</code>")
        print("</div>")
        
    os.remove("md2html.temp")
    os.remove("md2htm_2.temp")
        
def emit_bulletlist(data):
    print("<ul>")
    for item in data:
        print("<li>", end="")
        type = item[0]['t']
        if type == "Plain":
            for section in item[0]['c']:
                if section['t'] == "Str": emit_str(section['c'])
                elif section['t'] == "Space": emit_space()
        print("</li>")
    print("</ul>")

def emit_ordererdlist(data):
    print(f"<ol type=\"{data[0][0]}\">")
    for item in data[1:]:
        for what in item:
            for anotherwhat in what:
                print("<li>", end="")
                type = anotherwhat['t']
                if type == "Plain":
                    for section in anotherwhat['c']:
                        if section['t'] == "Str": emit_str(section['c'])
                        elif section['t'] == "Space": emit_space()
                print("</li>")
    print("</ol>")

def get_json_from_gfm(path: str):
    result = subprocess.run(["pandoc", "-f", "gfm", "-t", "json", path], capture_output=True)
    if result.returncode != 0:
        printf(f"pandoc failed")
        exit(69)
      
    return json.loads(result.stdout.decode("utf-8"))
       
def walk(dictionary: dict):
    for block in dictionary["blocks"]:
        type = block['t']
        if type == "Header":
            emit_header(block['c'][2], block['c'][0])
        if type == "Para":
            emit_para(block['c'])
        if type == "CodeBlock":
            emit_code_block(block['c'][1], block['c'][0][1][0])
        if type == "RawBlock":
            emit_raw_block(block['c'])
        if type == "BulletList":
            emit_bulletlist(block['c'])
        if type == "OrderedList":
            emit_ordererdlist(block['c'])
        print("")


def main():
    print("<!DOCTYPE html>")
    print("<html>")
    print("<head>")
    print(f"<title>{sys.argv[2]}</title>")
    print(f"<link rel=\"stylesheet\" href=\"/static/index.css\">")
    print("</head>")
    
    dict = get_json_from_gfm(sys.argv[1])
    walk(dict)
    
    print("</html>")
    
if __name__ == "__main__": main()
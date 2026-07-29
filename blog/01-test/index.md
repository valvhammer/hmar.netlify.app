# Test

This is a test of md2html.py, a hand written converter. Hopefully it works *fine*...

## Heading

### Subheading

#### Sub-subheading

Body

**Bold**

*Italic*

~~Strikethrough~~

<ins>Underline</ins>


## Code

This is a test of code2html, a vibecoded (unfortunately, but i had no idea how to do it) highlighter

```c
#include <stdio.h>

int main()
{
	printf("Hello, World!\n");
	return 0;
}
```

### code2html
To use `code2html`, run the following command `./code2html -input &lt;input&gt; [additional options]`

I will gatekeep the additional options.

## Le table au chocolat

There should be a table of common keybinds here. If it's not here, that probably means I didnt write that into md2html.py

| Keybind | Action     |
|---------|------------|
| Ctrl+C  | Copy       |
| Ctrl+V  | Paste      |
| Ctrl+X  | Cut        |
| Ctrl+A  | Select all |
| Ctrl+P  | Print      |
| Ctrl+S  | Save       |
| Ctrl+O  | Open       |
| Ctrl+N  | New        |

## Other funny stuff

Horizontal line below this
<hr></hr>
Horizontal line above this

### Hyperlinks

[URL](https://example.com)

![Image](https://http.cat/402)

### What are these called

 - List
 - Second item

1. list with numbers
2. second item

<small>Written on 2026-07-29</small>
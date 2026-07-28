/* code2.c - Transform code to HTML for syntax highlighting
 *
 * the following tags are required in your stylesheet:
 *  - keyword: keywords, e.g.: if, return, while, const
 *  - number
 *  - string: characters enclosed by "", '' or <>
 *  - string-escape: escape sequences
 *  - type: typenames, e.g.: char, size_t
 *  - comment
 *  - func: function names and function calls
 *  - preproc: general preprocessor statements
 *  - preproc-import: #include, #import
 *  - preproc-define: #define, 
 *  - preproc-macro: the first token following #define
 * */

// hello

#include <stdio.h>
#define NOB_IMPLEMENTATION
#define FLAG_IMPLEMENTATION
#include "nob.h"
#include "flag.h"

#define STB_C_LEX_DISCARD_PREPROCESSOR N
#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"

static void basename_noext(const char *filename, char *buf, size_t bufsz)
{
    if (!filename) return;
    if (!buf) return;
    if (bufsz == 0) return;
    const char *last_slash = strrchr(filename, '/');
#ifdef _WIN32
    const char *last_backslash = strrchr(filename, '\\');
    if (last_backslash > last_slash) last_slash = last_backslash;
#endif
    const char *name = last_slash ? last_slash+1 : filename;
    strncpy(buf, name, bufsz-1);
    buf[bufsz-1] = '\0';

    char *dot = strrchr(buf, '.');
    if (dot) *dot = '\0';
}

static struct {
    char input[256];
    char lang[64];
    char output[256];
} settings = {0};

void usage(void)
{
    fprintf(stderr, "[USAGE] %s -input <input file> [OPTIONS]\n", flag_program_name());
    fprintf(stderr, "Options:\n");
    flag_print_options(stderr);
}

void parse_args(int argc, char **argv)
{
    bool *help = flag_bool("help", false, "show this");
    char **input = flag_str("input", NULL, "input file (MANDATORY)");
    char **output = flag_str("output", NULL, "output file (def.: ./<input basename>.html)");
    char **lang = flag_str("lang", "c", "language, pass \'list\' for list");
    
    if (!flag_parse(argc, argv)) {
        usage();
        flag_print_error(stderr);
        exit(1);
    }

    if ( *help) {
        usage();
        exit(0);
    }

    if (*input == NULL) {
        usage();
        nob_log(NOB_ERROR, "no input file specified");
        exit(2);
    }
    
    snprintf(settings.input, sizeof(settings.input), "%s", *input);

    if (*output == NULL) {
        char name[256];
        basename_noext(*input, name, sizeof(name));
        snprintf(settings.output, sizeof(settings.output), "./%s.html", name);
    } else snprintf(settings.output, sizeof(settings.output), "%s", *output);
    
    if (strcmp(*lang, "list") == 0) {
        nob_log(NOB_INFO, "List of languages:\n"
                   "           c\n"
                   "               C-like languages, e.g.: C, C++, GLSL, etc\n");
        exit(3);
    }
    snprintf(settings.lang, sizeof(settings.lang), "%s", *lang);
}

#include <ctype.h>
 
static const char *keywords[] = {
    "if","else","for","while","do","switch","case","default","break",
    "continue","return","goto","sizeof","typedef","struct","union","enum",
    "static","const","volatile","extern","inline","register","void", "public", 
    "private", "super", "class", NULL
};
static const char *types[] = {
    "int","char","float","double","long","short","unsigned","signed",
    "size_t","bool", "boolean", NULL
};
 
static int in_list(const char *s, const char **list) {
    for (int i = 0; list[i]; i++) if (strcmp(s, list[i]) == 0) return 1;
    return 0;
}

static void emit_char(char c, Nob_String_Builder *out) {
    if (c == '\n') nob_sb_appendf(out, "<br>\n");
    else if (c == ' ') nob_sb_appendf(out, "&nbsp;");
    else if (c == '\t') nob_sb_appendf(out, "&nbsp;&nbsp;&nbsp;&nbsp;");
    else if (c == '<') nob_sb_appendf(out, "&lt;");
    else if (c == '>') nob_sb_appendf(out, "&gt;");
    else if (c == '&') nob_sb_appendf(out, "&amp;");
    else nob_sb_appendf(out, "%c", c);
}
 
// HTML-escape + convert whitespace to your &nbsp;/<br> markup
static void emit_gap(const char *start, const char *end, Nob_String_Builder *out) {
    const char *p = start;
    int at_line_start = 1;

    while (p < end) {
        if (at_line_start && *p == '#') {
            nob_sb_appendf(out, "<preproc>");
            while (p < end && *p != '\n') {
                emit_char(*p, out); p++; 
            }
            nob_sb_appendf(out, "</preproc>");
            continue;
        }

        if (*p == '/' && p + 1 < end && p[1] == '*') {
            nob_sb_appendf(out, "<comment>");
            emit_char(p[0], out); emit_char(p[1], out); p += 2;
            while (p < end && !(p[0] == '*' && p + 1 < end && p[1] == '/')) { emit_char(*p, out); p++; }
            if (p < end) { emit_char(p[0], out); emit_char(p[1], out); p += 2; }
            nob_sb_appendf(out, "</comment>");
            at_line_start = 0;
            continue;
        }

        if (*p == '/' && p + 1 < end && p[1] == '/') {
            nob_sb_appendf(out, "<comment>");
            while (p < end && *p != '\n') { emit_char(*p, out); p++; }
            nob_sb_appendf(out, "</comment>");
            continue;
        }

        if (*p == '\n') at_line_start = 1;
        else if (!isspace((unsigned char)*p)) at_line_start = 0;
        emit_char(*p, out);
        p++;
    }
}
 
static void emit_escaped(const char *start, const char *end, Nob_String_Builder *out) {
    for (const char *p = start; p < end; p++) {
        if (*p == '<') nob_sb_appendf(out, "&lt;");
        else if (*p == '>') nob_sb_appendf(out, "&gt;");
        else if (*p == '&') nob_sb_appendf(out, "&amp;");
        else nob_sb_appendf(out, "%c", *p);
    }
}
 
// double-quoted string body: split out \n \t \\ etc into <string-escape>
static void emit_string_body(const char *start, const char *end, Nob_String_Builder *out) {
    const char *run = start;
    for (const char *p = start; p < end; p++) {
        if (*p == '\\' && p + 1 < end) {
            if (p > run) { emit_escaped(run, p, out); }
            nob_sb_appendf(out, "<string-escape>");
            emit_escaped(p, p + 2, out);
            nob_sb_appendf(out, "</string-escape>");
            p++;
            run = p + 1;
        }
    }
    if (run < end) emit_escaped(run, end, out);
}
 
// grab rest of the current line for a '#...' directive, return pointer to '\n'
static const char *rest_of_line(const char *p, const char *eof) {
    while (p < eof && *p != '\n') p++;
    return p;
}

static const char *scan_number(const char *start, const char *eof,
                                const char **prefix_end, const char **suffix_start) {
    const char *p = start;
    *prefix_end = start;
 
    if (p[0] == '0' && p + 1 < eof &&
        (p[1]=='b' || p[1]=='B' || p[1]=='o' || p[1]=='O' || p[1]=='x' || p[1]=='X')) {
        char base = (char)tolower((unsigned char)p[1]);
        p += 2;
        *prefix_end = p;
        while (p < eof &&
               ((base == 'x' && isxdigit((unsigned char)*p)) ||
                (base == 'b' && (*p == '0' || *p == '1')) ||
                (base == 'o' && (*p >= '0' && *p <= '7'))))
            p++;
    } else {
        while (p < eof && isdigit((unsigned char)*p)) p++;
        if (p < eof && *p == '.') {
            p++;
            while (p < eof && isdigit((unsigned char)*p)) p++;
        }
        if (p < eof && (*p == 'e' || *p == 'E')) {
            const char *save = p++;
            if (p < eof && (*p == '+' || *p == '-')) p++;
            if (p < eof && isdigit((unsigned char)*p))
                while (p < eof && isdigit((unsigned char)*p)) p++;
            else
                p = save; // wasn't actually an exponent
        }
    }
 
    *suffix_start = p;
    while (p < eof && strchr("fFlLuU", *p)) p++;
    return p;
}
 
void highlight_c(Nob_String_Builder *in, Nob_String_Builder *out) {
    stb_lexer lex;
    char string_store[1024];
    stb_c_lexer_init(&lex, in->items, in->items + in->count, string_store, sizeof(string_store));
 
    const char *cursor = in->items;
 
    while (stb_c_lexer_get_token(&lex)) {
        emit_gap(cursor, lex.where_firstchar, out);

        {
            const char *fc = lex.where_firstchar;

            if (*fc == '#') emit_gap(lex.where_firstchar, lex.where_lastchar, out);

            int looks_like_number =
                isdigit((unsigned char)fc[0]) ||
                (fc[0] == '.' && fc + 1 < in->items + in->count && isdigit((unsigned char)fc[1]));
            if (looks_like_number) {
                const char *prefix_end, *suffix_start;
                const char *end = scan_number(fc, in->items + in->count, &prefix_end, &suffix_start);
                nob_sb_appendf(out, "<number>");
                if (prefix_end > fc) {
                    nob_sb_appendf(out, "<number-prefix>");
                    emit_escaped(fc, prefix_end, out);
                    nob_sb_appendf(out, "</number-prefix>");
                }
                emit_escaped(prefix_end, suffix_start, out);
                if (end > suffix_start) {
                    nob_sb_appendf(out, "<number-suffix>");
                    emit_escaped(suffix_start, end, out);
                    nob_sb_appendf(out, "</number-suffix>");
                }
                nob_sb_appendf(out, "</number>");
                lex.parse_point = (char *)end; // resync stb past whatever it made of this
                cursor = end;
                continue;
            }
        }

        switch (lex.token) {
            case CLEX_id: {
                const char *name = lex.string; // parsed identifier text
                // lookahead: is next non-space char '(' ?
                const char *p = lex.where_lastchar + 1;
                while (p < (in->items + in->count) && isspace((unsigned char)*p)) p++;
                const char *tag =
                    in_list(name, keywords) ? "keyword" :
                    in_list(name, types)    ? "type" :
                    (p < (in->items + in->count) && *p == '(') ? "func" : NULL;
                if (tag) nob_sb_appendf(out, "<%s>", tag);
                emit_escaped(lex.where_firstchar, lex.where_lastchar + 1, out);
                if (tag) nob_sb_appendf(out, "</%s>", tag);
                break;
            }
            case CLEX_dqstring:
                nob_sb_appendf(out, "<string>&quot;");
                emit_string_body(lex.where_firstchar + 1, lex.where_lastchar, out);
                nob_sb_appendf(out, "&quot;</string>");
                break;
            case CLEX_charlit:
                nob_sb_appendf(out, "<string>");
                emit_escaped(lex.where_firstchar, lex.where_lastchar + 1, out);
                nob_sb_appendf(out, "</string>");
                break;
            default:
                // punctuation / operators, unstyled
                emit_escaped(lex.where_firstchar, lex.where_lastchar + 1, out);
                break;
        }
 
        cursor = lex.where_lastchar + 1;
    }
    // trailing whitespace after the last token
    emit_gap(cursor, (in->items + in->count), out);
}

void run(void)
{
    Nob_String_Builder input = {0}, output = {0};
    if (!nob_read_entire_file(settings.input, &input)) exit(-1);

    if (strncmp(settings.lang, "c", sizeof(settings.lang)) == 0) {
        highlight_c(&input, &output);
    } else {
        TODOF("settings.lang == %s <-- unimplemented language", settings.lang);
    }

    nob_write_entire_file(settings.output, output.items, output.count);
    nob_sb_free(input);
    nob_sb_free(output);
}

int main(int argc, char **argv)
{
    parse_args(argc, argv);
     
    run();

    return 0;
}

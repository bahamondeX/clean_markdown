// clean_markdown.c - C extension for Python
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <time.h>

// Structure to hold regex pattern and replacement
typedef struct {
    const char* pattern;
    const char* replacement;
    int flags;
    regex_t regex;
} RegexPattern;

// Apply a single regex pattern to text
char* apply_pattern(const char* text, regex_t* regex, const char* replacement) {
    size_t nmatch = 10;  // Support up to 10 capture groups
    regmatch_t pmatch[10];
    
    // Check if pattern matches
    if (regexec(regex, text, nmatch, pmatch, 0) != 0) {
        return strdup(text);  // No match, return copy of original
    }
    
    // Calculate size for new string
    size_t result_size = strlen(text) + strlen(replacement) + 1;
    char* result = (char*)malloc(result_size);
    if (!result) return NULL;
    
    // Copy prefix
    strncpy(result, text, pmatch[0].rm_so);
    result[pmatch[0].rm_so] = '\0';
    
    // Apply replacement (with backreferences)
    char* current = result + pmatch[0].rm_so;
    const char* repl_ptr = replacement;
    while (*repl_ptr) {
        if (*repl_ptr == '\\' && *(repl_ptr + 1) >= '1' && *(repl_ptr + 1) <= '9') {
            // Handle backreference
            int group = *(repl_ptr + 1) - '0';
            if (pmatch[group].rm_so != -1 && pmatch[group].rm_eo != -1) {
                size_t len = pmatch[group].rm_eo - pmatch[group].rm_so;
                strncpy(current, text + pmatch[group].rm_so, len);
                current += len;
            }
            repl_ptr += 2;
        } else {
            // Copy regular character
            *current++ = *repl_ptr++;
        }
    }
    *current = '\0';
    
    // Copy suffix
    strcat(result, text + pmatch[0].rm_eo);
    
    return result;
}

// Clean markdown text
char* clean_markdown_c(const char* markdown) {
    // Define patterns
    RegexPattern patterns[] = {
        // Headers
        {"^#[[:space:]]+(.+)$", "\\1", REG_EXTENDED | REG_NEWLINE},
        
        // Bold and Italic
        {"\\*\\*(.+?)\\*\\*", "\\1", REG_EXTENDED},
        {"__(.+?)__", "\\1", REG_EXTENDED},
        {"\\*([^*]+)\\*", "\\1", REG_EXTENDED},
        {"_([^_]+)_", "\\1", REG_EXTENDED},
        
        // Links - extract text, remove URLs
        {"\\[([^]]+)\\]\\([^)]+\\)", "\\1", REG_EXTENDED},
        
        // HTTP URLs - remove completely
        {"https?://[a-zA-Z0-9\\-\\._~:/\\?#\\[\\]@!\\$&'\\(\\)\\*\\+,;=%]+", "", REG_EXTENDED},
        
        // Images - remove completely
        {"!\\[([^]]*)?\\]\\([^)]+\\)", "", REG_EXTENDED},
        
        // Code blocks with or without ending
        {"```(?:[^`]|`[^`]|``[^`])*(?:```|$)", "[Code Omitted]", REG_EXTENDED | REG_NEWLINE},
        
        // Inline code
        {"`([^`]+)`", "\\1", REG_EXTENDED},
        
        // Lists
        {"^[[:space:]]*[-*+][[:space:]]+(.+)$", "\\1", REG_EXTENDED | REG_NEWLINE},
        {"^[[:space:]]*[0-9]+\\.[[:space:]]+(.+)$", "\\1", REG_EXTENDED | REG_NEWLINE},
        
        // Blockquotes
        {"^>[[:space:]]+(.+)$", "\\1", REG_EXTENDED | REG_NEWLINE},
        
        // Horizontal rule
        {"^[-*_]{3,}$", "", REG_EXTENDED | REG_NEWLINE},
        
        // Tables
        {"\\|", " ", REG_EXTENDED},
        {"^[-|: ]+$", "", REG_EXTENDED | REG_NEWLINE}
    };
    
    const int pattern_count = sizeof(patterns) / sizeof(patterns[0]);
    
    // Compile all patterns
    for (int i = 0; i < pattern_count; i++) {
        if (regcomp(&patterns[i].regex, patterns[i].pattern, patterns[i].flags) != 0) {
            fprintf(stderr, "Failed to compile pattern: %s\n", patterns[i].pattern);
            // Clean up already compiled patterns
            for (int j = 0; j < i; j++) {
                regfree(&patterns[j].regex);
            }
            return NULL;
        }
    }
    
    // Initial result is a copy of the input
    char* result = strdup(markdown);
    if (!result) return NULL;
    
    // Apply each pattern
    for (int i = 0; i < pattern_count; i++) {
        char* new_result = apply_pattern(result, &patterns[i].regex, patterns[i].replacement);
        free(result);
        result = new_result;
        
        if (!result) {
            // Clean up remaining compiled patterns
            for (int j = i; j < pattern_count; j++) {
                regfree(&patterns[j].regex);
            }
            return NULL;
        }
    }
    
    // Free all compiled patterns
    for (int i = 0; i < pattern_count; i++) {
        regfree(&patterns[i].regex);
    }
    
    return result;
}

// Python module functions

// Clean markdown function for Python
static PyObject* clean_markdown(PyObject* self, PyObject* args) {
    const char* markdown;
    
    // Parse arguments (expect a string)
    if (!PyArg_ParseTuple(args, "s", &markdown)) {
        return NULL;
    }
    
    // Call the C function
    char* cleaned = clean_markdown_c(markdown);
    if (!cleaned) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to clean markdown");
        return NULL;
    }
    
    // Create Python string from result
    PyObject* result = PyUnicode_FromString(cleaned);
    
    // Free the C string
    free(cleaned);
    
    return result;
}

// Module method definitions
static PyMethodDef MarkdownCleanerMethods[] = {
    {"clean_markdown", clean_markdown, METH_VARARGS, "Clean markdown formatting and replace code blocks."},
    {NULL, NULL, 0, NULL}  // Sentinel
};

// Module definition
static struct PyModuleDef markdown_cleaner_module = {
    PyModuleDef_HEAD_INIT,
    "markdown_cleaner",
    "A module for cleaning markdown text",
    -1,
    MarkdownCleanerMethods
};

// Module initialization function
PyMODINIT_FUNC PyInit_markdown_cleaner(void) {
    return PyModule_Create(&markdown_cleaner_module);
}
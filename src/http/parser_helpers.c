/**
Returns 1 when true
for alpha is any character between A-Z and a-z
*/
int is_alpha(char c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        return 1;
    }

    return 0;
}

int is_numeric(char c) {
    if (c >= '0' && c <= '9') {
        return 1;
    }

    return 0;
}

int is_ctl(char c) {
    // Values between 0 - 31 in base 8 (octal) and value number 127 (0177 in base
    // 8) specified in HTTP/1.0 spec sec 2.2
    if ((c >= 00 && c <= 031) || c == 0177) {
        return 1;
    }

    return 0;
}

// tspecials from HTTP/1.0 spec
int is_tspecial(char c) {
    switch (c) {
    case ('('):
    case (')'):
    case ('<'):
    case ('>'):
    case ('@'):
    case (','):
    case (';'):
    case (':'):
    case ('\\'):
    case ('"'):
    case ('/'):
    case ('['):
    case (']'):
    case ('?'):
    case ('='):
    case ('{'):
    case ('}'):
    case (' '):
    case (0x9):
        return 1;
    default:
        return 0;
    }
}

int is_token(char c) {
    if (!is_ctl(c) && !is_tspecial(c)) {
        return 1;
    }

    return 0;
}

int is_hex(char c) {
    if (is_numeric(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
        return 1;
    }

    return 0;
}

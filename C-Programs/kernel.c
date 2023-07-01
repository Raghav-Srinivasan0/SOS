#include "kernel.h"
#include "ide.h"

//Keyboard Stuff

unsigned char inPortB (unsigned short _port)
{
	return ioport_in(_port);
}

unsigned char inb (unsigned short _port)
{
	return ioport_in(_port);
}

void outPortB (unsigned short _port, unsigned char _data)
{
	ioport_out(_port,_data);
}

void outb (unsigned short _port, unsigned char _data)
{
	ioport_out(_port,_data);
}

/* string stuff */
char *strcpy(char *s1, const char *s2)
{
    char *s1_p = s1;
    while (*s1++ = *s2++)
      ;
    return s1_p;
}

size_t strlen(const char *s) {
    size_t cnt = 0;
    while (*s ++ != '\0') {
        cnt ++;
    }
    return cnt;
}

size_t strnlen(const char *s, size_t len) {
    size_t cnt = 0;
    while (cnt < len && *s ++ != '\0') {
        cnt ++;
    }
    return cnt;
}

char * strcat(char *dst, const char *src) {
    return strcpy(dst + strlen(dst), src);
}

char * strncpy(char *dst, const char *src, size_t len) {
    char *p = dst;
    while (len > 0) {
        if ((*p = *src) != '\0') {
            src ++;
        }
        p ++, len --;
    }
    return dst;
}

int strcmp(const char *s1, const char *s2) {
#ifdef __HAVE_ARCH_STRCMP
    return __strcmp(s1, s2);
#else
    while (*s1 != '\0' && *s1 == *s2) {
        s1 ++, s2 ++;
    }
    return (int)((unsigned char)*s1 - (unsigned char)*s2);
#endif /* __HAVE_ARCH_STRCMP */
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n > 0 && *s1 != '\0' && *s1 == *s2) {
        n --, s1 ++, s2 ++;
    }
    return (n == 0) ? 0 : (int)((unsigned char)*s1 - (unsigned char)*s2);
}

char * strchr(const char *s, char c) {
    while (*s != '\0') {
        if (*s == c) {
            return (char *)s;
        }
        s ++;
    }
    return NULL;
}

char * strfind(const char *s, char c) {
    while (*s != '\0') {
        if (*s == c) {
            break;
        }
        s ++;
    }
    return (char *)s;
}

long strtol(const char *s, char **endptr, int base) {
    int neg = 0;
    long val = 0;

    // gobble initial whitespace
    while (*s == ' ' || *s == '\t') {
        s ++;
    }

    // plus/minus sign
    if (*s == '+') {
        s ++;
    }
    else if (*s == '-') {
        s ++, neg = 1;
    }

    // hex or octal base prefix
    if ((base == 0 || base == 16) && (s[0] == '0' && s[1] == 'x')) {
        s += 2, base = 16;
    }
    else if (base == 0 && s[0] == '0') {
        s ++, base = 8;
    }
    else if (base == 0) {
        base = 10;
    }

    // digits
    while (1) {
        int dig;

        if (*s >= '0' && *s <= '9') {
            dig = *s - '0';
        }
        else if (*s >= 'a' && *s <= 'z') {
            dig = *s - 'a' + 10;
        }
        else if (*s >= 'A' && *s <= 'Z') {
            dig = *s - 'A' + 10;
        }
        else {
            break;
        }
        if (dig >= base) {
            break;
        }
        s ++, val = (val * base) + dig;
        // we don't properly detect overflow!
    }

    if (endptr) {
        *endptr = (char *) s;
    }
    return (neg ? -val : val);
}

/* mem stuff */

void* memmove(void* dstptr, const void* srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	if (dst < src) {
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	} else {
		for (size_t i = size; i != 0; i--)
			dst[i-1] = src[i-1];
	}
	return dstptr;
}

int memcmp(const void* aptr, const void* bptr, size_t size) {
	const unsigned char* a = (const unsigned char*) aptr;
	const unsigned char* b = (const unsigned char*) bptr;
	for (size_t i = 0; i < size; i++) {
		if (a[i] < b[i])
			return -1;
		else if (b[i] < a[i])
			return 1;
	}
	return 0;
}

void* memset(void* bufptr, int value, size_t size) {
	unsigned char* buf = (unsigned char*) bufptr;
	for (size_t i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}

void* memcpy(void* restrict dstptr, const void* restrict srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	for (size_t i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}

bool isdigit(char c) {
  return c >= '0' && c <= '9';
}

void init_idt()
{
	unsigned int offset = (unsigned int)keyboard_handler;
	IDT[0x21].offset_lowerbits = offset & 0x0000FFFF;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = IDT_INTERRUPT_GATE_32BIT;
	IDT[0x21].offset_upperbits = (offset & 0xFFFF0000) >> 16;

	outPortB(PIC1_COMMAND_PORT, 0x11);
	outPortB(PIC2_COMMAND_PORT, 0x11);

	outPortB(PIC1_DATA_PORT, 0x20);
	outPortB(PIC2_DATA_PORT, 0x20);

	outPortB(PIC1_DATA_PORT, 0x0);
	outPortB(PIC2_DATA_PORT, 0x0);

	outPortB(PIC1_DATA_PORT, 0x1);
	outPortB(PIC2_DATA_PORT, 0x1);

	outPortB(PIC1_DATA_PORT, 0xff);
	outPortB(PIC2_DATA_PORT, 0xff);

	struct IDT_pointer idt_ptr;
	idt_ptr.limit = (sizeof(struct IDT_entry) * IDT_SIZE) - 1;
	idt_ptr.base = (unsigned int) &IDT;
	load_idt(&idt_ptr);
}

void kb_init() 
{
	outPortB(PIC1_DATA_PORT, 0xFD);
}

char digitToChar(uint8_t digit)
{
	return (char)digit;
}

uint16_t color(uint8_t foreground, uint8_t background)
{
	return (uint16_t)foreground | (uint16_t)background << 8;
}

/* Printchar implementation */

/* Printf implementation */

// TODO: Fix the fact that only 130 characters can be printed to the screen before the cursor goes to the beginning

int cursor_row = 0;
int cursor_col = 0;
uint16_t currentColor = 0x0F;
uint16_t defaultColor = 0x0F;

void clear_screen() {
	for (int i = 0; i < VGA_HEIGHT; i++) {
		for (int j = 0; j < VGA_WIDTH; j++) {
			printchar_at(' ', i, j);
		}
	}
    cursor_row = 0;
}

void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outPortB(0x3D4, 0x0A);
	outPortB(0x3D5, (inPortB(0x3D5) & 0xC0) | cursor_start);
 
	outPortB(0x3D4, 0x0B);
	outPortB(0x3D5, (inPortB(0x3D5) & 0xE0) | cursor_end);
}

void disable_cursor()
{
	outPortB(0x3D4, 0x0A);
	outPortB(0x3D5, 0x20);
}

void update_cursor(int x, int y)
{
	uint16_t pos = y * VGA_WIDTH + x;
 
	outPortB(0x3D4, 0x0F);
	outPortB(0x3D5, (uint8_t) (pos & 0xFF));
	outPortB(0x3D4, 0x0E);
	outPortB(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

uint16_t get_cursor_position(void)
{
    uint16_t pos = 0;
    outPortB(0x3D4, 0x0F);
    pos |= inPortB(0x3D5);
    outPortB(0x3D4, 0x0E);
    pos |= ((uint16_t)inPortB(0x3D5)) << 8;
    return pos;
}

void printchar(char c) {
    printchar_at(c, cursor_row, cursor_col++);
    if (cursor_col >= VGA_WIDTH) {
        cursor_col = cursor_col % VGA_WIDTH;
        cursor_row++;
    }
}

void printchar_at(char c, int row, int col) {
	// OFFSET = (ROW * 80) + COL
	char* offset = (char*) (0xb8000 + 2*((row * VGA_WIDTH) + col));
	*offset = c;
	offset[1] = (char)currentColor;
}

char* readline(int row)
{
	char* result = (char*) (0xb8000 + 2*((row * VGA_WIDTH)));
	return result;
}

void shiftUp()
{
	for (int i = 1; i<VGA_HEIGHT; i++)
	{
		char* line_data = readline(i);
		for (int j = 0; j<VGA_WIDTH*2; j+=2)
		{
			printchar_at(line_data[j], i-1,j/2);
		}
	}
	for (int j = 0; j<VGA_WIDTH*2; j+=2)
	{
		printchar_at(' ', VGA_HEIGHT-1,j/2);
	}
}

void newline() {
	if (cursor_row < VGA_HEIGHT-1)
	{
		cursor_row++;
	}
	else{
		shiftUp();
	}
    cursor_col = 0;
}

void println(char* line)
{
	int i = 0;
	while (line[i]!='\0')
	{
		printchar(line[i]);
		i++;
	}
	newline();
}

/*
char* __int_str(intmax_t i, char b[], int base, bool plusSignIfNeeded, bool spaceSignIfNeeded,
                int paddingNo, bool justify, bool zeroPad) {
 
    char digit[32] = {0};
    memset(digit, 0, 32);
    strcpy(digit, "0123456789");
 
    if (base == 16) {
        strcat(digit, "ABCDEF");
    } else if (base == 17) {
        strcat(digit, "abcdef");
        base = 16;
    }
 
    char* p = b;
    if (i < 0) {
        *p++ = '-';
        i *= -1;
    } else if (plusSignIfNeeded) {
        *p++ = '+';
    } else if (!plusSignIfNeeded && spaceSignIfNeeded) {
        *p++ = ' ';
    }
 
    intmax_t shifter = i;
    do {
        ++p;
        shifter = shifter / base;
    } while (shifter);
 
    *p = '\0';
    do {
        *--p = digit[i % base];
        i = i / base;
    } while (i);
 
    int padding = paddingNo - (int)strlen(b);
    if (padding < 0) padding = 0;
 
    if (justify) {
        while (padding--) {
            if (zeroPad) {
                b[strlen(b)] = '0';
            } else {
                b[strlen(b)] = ' ';
            }
        }
 
    } else {
        char a[256] = {0};
        while (padding--) {
            if (zeroPad) {
                a[strlen(a)] = '0';
            } else {
                a[strlen(a)] = ' ';
            }
        }
        strcat(a, b);
        strcpy(b, a);
    }
 
    return b;
}
*/ 
void displayCharacter(char c, int* a) {
    printchar(c);
    *a += 1;
}
 
void displayString(char* c, int* a) {
    for (int i = 0; c[i]; ++i) {
        displayCharacter(c[i], a);
    }
}
 
int vprintf (const char* format, va_list list)
{
    int chars        = 0;
    char intStrBuffer[256] = {0};
 
    for (int i = 0; format[i]; ++i) {
 
        char specifier   = '\0';
        char length      = '\0';
 
        int  lengthSpec  = 0; 
        int  precSpec    = 0;
        bool leftJustify = false;
        bool zeroPad     = false;
        bool spaceNoSign = false;
        bool altForm     = false;
        bool plusSign    = false;
        bool emode       = false;
        int  expo        = 0;
 
        if (format[i] == '%') {
            ++i;
 
            bool extBreak = false;
            while (1) {
 
                switch (format[i]) {
                    case '-':
                        leftJustify = true;
                        ++i;
                        break;
 
                    case '+':
                        plusSign = true;
                        ++i;
                        break;
 
                    case '#':
                        altForm = true;
                        ++i;
                        break;
 
                    case ' ':
                        spaceNoSign = true;
                        ++i;
                        break;
 
                    case '0':
                        zeroPad = true;
                        ++i;
                        break;
 
                    default:
                        extBreak = true;
                        break;
                }
 
                if (extBreak) break;
            }
 
            while (isdigit(format[i])) {
                lengthSpec *= 10;
                lengthSpec += format[i] - 48;
                ++i;
            }
 
            if (format[i] == '*') {
                lengthSpec = va_arg(list, int);
                ++i;
            }
 
            if (format[i] == '.') {
                ++i;
                while (isdigit(format[i])) {
                    precSpec *= 10;
                    precSpec += format[i] - 48;
                    ++i;
                }
 
                if (format[i] == '*') {
                    precSpec = va_arg(list, int);
                    ++i;
                }
            } else {
                precSpec = 6;
            }
 
            if (format[i] == 'h' || format[i] == 'l' || format[i] == 'j' ||
                   format[i] == 'z' || format[i] == 't' || format[i] == 'L') {
                length = format[i];
                ++i;
                if (format[i] == 'h') {
                    length = 'H';
                } else if (format[i] == 'l') {
                    length = 'q';
                    ++i;
                }
            }
            specifier = format[i];
 
            memset(intStrBuffer, 0, 256);
 
            int base = 10;
            if (specifier == 'o') {
                base = 8;
                specifier = 'u';
                if (altForm) {
                    displayString("0", &chars);
                }
            }
            if (specifier == 'p') {
                base = 16;
                length = 'z';
                specifier = 'u';
            }
            switch (specifier) {
                case 'X':
                    base = 16;
                case 'x':
                    base = base == 10 ? 17 : base;
                    if (altForm) {
                        displayString("0x", &chars);
                    }
 
                /*case 'u':
                {
                    switch (length) {
                        case 0:
                        {
                            unsigned int integer = va_arg(list, unsigned int);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            displayString(intStrBuffer, &chars);
                            break;
                        }
                        case 'H':
                        {
                            unsigned char integer = (unsigned char) va_arg(list, unsigned int);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            displayString(intStrBuffer, &chars);
                            break;
                        }
                        case 'h':
                        {
                            unsigned short int integer = va_arg(list, unsigned int);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            displayString(intStrBuffer, &chars);
                            break;
                        }
                        case 'l':
                        {
                            unsigned long integer = va_arg(list, unsigned long);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            displayString(intStrBuffer, &chars);
                            break;
                        }
                        case 'q':
                        {
                            unsigned long long integer = va_arg(list, unsigned long long);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            displayString(intStrBuffer, &chars);
                            break;
                        }
                        case 'j':
                        {
                            uintmax_t integer = va_arg(list, uintmax_t);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            displayString(intStrBuffer, &chars);
                            break;
                        }
                        case 'z':
                        {
                            size_t integer = va_arg(list, size_t);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            displayString(intStrBuffer, &chars);
                            break;
                        }
                        case 't':
                        {
                            ptrdiff_t integer = va_arg(list, ptrdiff_t);
                            __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                            displayString(intStrBuffer, &chars);
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }*/
 
                case 'd':
                /*case 'i':
                {
                    switch (length) {
                    case 0:
                    {
                        int integer = va_arg(list, int);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        displayString(intStrBuffer, &chars);
                        break;
                    }
                    case 'H':
                    {
                        signed char integer = (signed char) va_arg(list, int);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        displayString(intStrBuffer, &chars);
                        break;
                    }
                    case 'h':
                    {
                        short int integer = va_arg(list, int);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        displayString(intStrBuffer, &chars);
                        break;
                    }
                    case 'l':
                    {
                        long integer = va_arg(list, long);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        displayString(intStrBuffer, &chars);
                        break;
                    }
                    case 'q':
                    {
                        long long integer = va_arg(list, long long);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        displayString(intStrBuffer, &chars);
                        break;
                    }
                    case 'j':
                    {
                        intmax_t integer = va_arg(list, intmax_t);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        displayString(intStrBuffer, &chars);
                        break;
                    }
                    case 'z':
                    {
                        size_t integer = va_arg(list, size_t);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        displayString(intStrBuffer, &chars);
                        break;
                    }
                    case 't':
                    {
                        ptrdiff_t integer = va_arg(list, ptrdiff_t);
                        __int_str(integer, intStrBuffer, base, plusSign, spaceNoSign, lengthSpec, leftJustify, zeroPad);
                        displayString(intStrBuffer, &chars);
                        break;
                    }
                    default:
                        break;
                    }
                    break;
                }*/
 
                case 'c':
                {
                    displayCharacter(va_arg(list, int), &chars);
 
                    break;
                }
 
                case 's':
                {
                    displayString(va_arg(list, char*), &chars);
                    break;
                }
 
                case 'n':
                {
                    switch (length) {
                        case 'H':
                            *(va_arg(list, signed char*)) = chars;
                            break;
                        case 'h':
                            *(va_arg(list, short int*)) = chars;
                            break;
 
                        case 0: {
                            int* a = va_arg(list, int*);
                            *a = chars;
                            break;
                        }
 
                        case 'l':
                            *(va_arg(list, long*)) = chars;
                            break;
                        case 'q':
                            *(va_arg(list, long long*)) = chars;
                            break;
                        case 'j':
                            *(va_arg(list, intmax_t*)) = chars;
                            break;
                        case 'z':
                            *(va_arg(list, size_t*)) = chars;
                            break;
                        case 't':
                            *(va_arg(list, ptrdiff_t*)) = chars;
                            break;
                        default:
                            break;
                    }
                    break;
                }
 
                case 'e':
                case 'E':
                    emode = true;
 
                case 'f':
                case 'F':
                case 'g':
                /*case 'G':
                {
                    double floating = va_arg(list, double);
 
                    while (emode && floating >= 10) {
                        floating /= 10;
                        ++expo;
                    }
 
                    int form = lengthSpec - precSpec - expo - (precSpec || altForm ? 1 : 0);
                    if (emode) {
                        form -= 4;      // 'e+00'
                    }
                    if (form < 0) {
                        form = 0;
                    }
 
                    __int_str(floating, intStrBuffer, base, plusSign, spaceNoSign, form, \
                              leftJustify, zeroPad);
 
                    displayString(intStrBuffer, &chars);
 
                    floating -= (int) floating;
 
                    for (int i = 0; i < precSpec; ++i) {
                        floating *= 10;
                    }
                    intmax_t decPlaces = (intmax_t) (floating + 0.5);
 
                    if (precSpec) {
                        displayCharacter('.', &chars);
                        __int_str(decPlaces, intStrBuffer, 10, false, false, 0, false, false);
                        intStrBuffer[precSpec] = 0;
                        displayString(intStrBuffer, &chars);
                    } else if (altForm) {
                        displayCharacter('.', &chars);
                    }
 
                    break;
                }*/
 
 
                case 'a':
                case 'A':
                    //ACK! Hexadecimal floating points...
                    break;
 
                default:
                    break;
            }
 
            if (specifier == 'e') {
                displayString("e+", &chars);
            } else if (specifier == 'E') {
                displayString("E+", &chars);
            }
 
            /*if (specifier == 'e' || specifier == 'E') {
                __int_str(expo, intStrBuffer, 10, false, false, 2, false, true);
                displayString(intStrBuffer, &chars);
            }*/
 
        } else {
            displayCharacter(format[i], &chars);
        }
    }
 
    return chars;
}
 
__attribute__ ((format (printf, 1, 2))) int printf (const char* format, ...)
{
    va_list list;
    va_start (list, format);
    int i = vprintf (format, list);
    va_end (list);
    return i;
 
}

void multiboot_init(multiboot_info_t* mbd, uint32_t magic)
{
	/* Make sure the magic number matches for memory mapping*/
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		currentColor = color(RED,BLACK);
        println("invalid magic number!");
		currentColor = defaultColor;
    }
 
    /* Check bit 6 to see if we have a valid memory map */
    if(!(mbd->flags >> 6 & 0x1)) {
		currentColor = color(RED,BLACK);
        println("invalid memory map given by GRUB bootloader");
		currentColor = defaultColor;
    }
 
    /* Loop through the memory map and display the values */
    int i;
    for(i = 0; i < mbd->mmap_length; 
        i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = 
            (multiboot_memory_map_t*) (mbd->mmap_addr + i);
 
        printf("Start Addr: %llu | Length: %llu | Size: %x | Type: %d\n",
            mmmt->addr, mmmt->len, mmmt->size, mmmt->type);
	}
}

void print_init()
{
	println("                          ********   *******    ********              ");
	println("                         **//////   **/////**  **//////               ");
	println("                        /**        **     //**/**            Operating");
	println("       Srinivasan       /*********/**      /**/*********              ");
	println("                        ////////**/**      /**////////**              ");
	println("                               /**//**      **        /**    System   ");
	println("                         ********  //*******   ********               ");
	println("                        ////////    ///////   ////////                ");
}

void backspace() {
    if (cursor_col > 0) {
        print_char_with_asm(' ', cursor_row, --cursor_col);
		update_cursor(cursor_col,cursor_row);
    }else if(cursor_row > 0) {
		cursor_row--;
		cursor_col = VGA_WIDTH;
		update_cursor(cursor_col,cursor_row);
	}
}

void shell(char* linedata)
{
	if (linedata[0] == 'e' && linedata[2] == 'c' && linedata[4] == 'h' && linedata[6] == 'o' && linedata[8] == ' ')
	{
		currentColor = color(MAGENTA,BLACK);
		for (int i = 10; i<VGA_WIDTH*2; i+=2)
		{
			printchar(linedata[i]);
		}
		for (int i = 0; i<4; i++)
		{
			printchar(' ');
		}
		newline();
		currentColor = defaultColor;
		return;
	}
	else if (linedata[0] == 'l' && linedata[2] == 'i' && linedata[4] == 'n' && linedata[6] == 'e' && linedata[8] == ' ')
	{
		char tensDigit = ' ';
		char onesDigit = ' ';
		if (linedata[12] != ' ')
		{
			onesDigit = linedata[12];
			tensDigit = linedata[10];
		}else{
			onesDigit = linedata[10];
			tensDigit = '0';
		}
		int number = ((int)tensDigit - 48) * 10 + ((int)onesDigit - 48);
		char* linedata = readline(number);
		currentColor = color(MAGENTA,BLACK);
		for (int i = 0; i<VGA_WIDTH*2; i+=2)
		{
			printchar(linedata[i]);
		}
		currentColor = defaultColor;
		return;
	}

    /* broken */
    else if (linedata[0] == 'c' && linedata[2] == 'o' && linedata[4] == 'l' && linedata[6] == 'o' && linedata[8] == ' ')
	{
        char tensDigit_1 = ' ';
		char onesDigit_1 = ' ';
		if (linedata[14] != ' ')
		{
			onesDigit_1 = linedata[14];
			tensDigit_1 = linedata[12];
		}else{
			onesDigit_1 = linedata[12];
			tensDigit_1 = '0';
		}
		int number_1 = ((int)tensDigit_1 - 48) * 10 + ((int)onesDigit_1 - 48);
        char tensDigit_2 = ' ';
		char onesDigit_2 = ' ';
		if (linedata[20] != ' ')
		{
			onesDigit_2 = linedata[20];
			tensDigit_2 = linedata[18];
		}else{
			onesDigit_2 = linedata[18];
			tensDigit_2 = '0';
		}
		int number_2 = ((int)tensDigit_2 - 48) * 10 + ((int)onesDigit_2 - 48);
        defaultColor = color(number_1,number_2);
    }
}

void handle_keyboard_interrupt()
{
	outPortB(PIC1_COMMAND_PORT, 0x20);
	unsigned char status = inPortB(KEYBOARD_STATUS_PORT);
	if (status & 0x1) 
	{
		unsigned char keycode = inPortB(KEYBOARD_DATA_PORT);
		if (keycode < 0 || keycode >= 128) return;
		if (keycode == 14)
		{
			//printchar_at('b',10,10);
			backspace();
			return;
		}
		if (keycode == 28)
		{
			//printchar_at('e',10,10);
			char* line_data = readline(cursor_row);
			/* Implement Shell Commands Here */
			newline();
			shell(line_data);
			return;
		}
		if (keycode == 42 || keycode == 54)
		{
			//printchar_at('s',10,10);
			printchar((char)(((int)keyboard_map[keycode])-32));
		}else{
			printchar(keyboard_map[keycode]);
		}
		update_cursor(cursor_col,cursor_row);
	}
}

/* IDE Drive Stuff */

unsigned char ide_read(unsigned char channel, unsigned char reg) {
   unsigned char result;
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
   if (reg < 0x08)
      result = inb(channels[channel].base + reg - 0x00);
   else if (reg < 0x0C)
      result = inb(channels[channel].base  + reg - 0x06);
   else if (reg < 0x0E)
      result = inb(channels[channel].ctrl  + reg - 0x0A);
   else if (reg < 0x16)
      result = inb(channels[channel].bmide + reg - 0x0E);
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
   return result;
}

void ide_write(unsigned char channel, unsigned char reg, unsigned char data) {
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
   if (reg < 0x08)
      outb(channels[channel].base  + reg - 0x00, data);
   else if (reg < 0x0C)
      outb(channels[channel].base  + reg - 0x06, data);
   else if (reg < 0x0E)
      outb(channels[channel].ctrl  + reg - 0x0A, data);
   else if (reg < 0x16)
      outb(channels[channel].bmide + reg - 0x0E, data);
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

unsigned char ide_polling(unsigned char channel, unsigned int advanced_check) {
 
   // (I) Delay 400 nanosecond for BSY to be set:
   // -------------------------------------------------
   for(int i = 0; i < 4; i++)
      ide_read(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.
 
   // (II) Wait for BSY to be cleared:
   // -------------------------------------------------
   while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
      ; // Wait for BSY to be zero.
 
   if (advanced_check) {
      unsigned char state = ide_read(channel, ATA_REG_STATUS); // Read Status Register.
 
      // (III) Check For Errors:
      // -------------------------------------------------
      if (state & ATA_SR_ERR)
         return 2; // Error.
 
      // (IV) Check If Device fault:
      // -------------------------------------------------
      if (state & ATA_SR_DF)
         return 1; // Device Fault.
 
      // (V) Check DRQ:
      // -------------------------------------------------
      // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
      if ((state & ATA_SR_DRQ) == 0)
         return 3; // DRQ should be set
 
   }
 
   return 0; // No Error.
 
}

/* Hacky Sleep */

void sleep(int millis)
{
    for (int i = 0; i<millis*1000000; i++)
    {
        continue;
    }
    return;
}

static inline long inl(uint16_t port)
{
    long ret;
    asm volatile ( "inl %1, %0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}

void insl(unsigned reg, unsigned int *buffer, int quads)
{
    int index;
    for(index = 0; index < quads; index++)
    {
        buffer[index] = inl(reg);
    }
}

void ide_read_buffer(unsigned char channel, unsigned char reg, unsigned int * buffer,
                     unsigned int quads) {
   /* WARNING: This code contains a serious bug. The inline assembly trashes ES and
    *           ESP for all of the code the compiler generates between the inline
    *           assembly blocks.
    */
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
   asm("pushw %es; movw %ds, %ax; movw %ax, %es");
   if (reg < 0x08)
      insl(channels[channel].base  + reg - 0x00, buffer, quads);
   else if (reg < 0x0C)
      insl(channels[channel].base  + reg - 0x06, buffer, quads);
   else if (reg < 0x0E)
      insl(channels[channel].ctrl  + reg - 0x0A, buffer, quads);
   else if (reg < 0x16)
      insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
   asm("popw %es;");
   if (reg > 0x07 && reg < 0x0C)
      ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}

void ide_initialize(unsigned int BAR0, unsigned int BAR1, unsigned int BAR2, unsigned int BAR3,
unsigned int BAR4) {
 
   int j, k, count = 0;
 
   // 1- Detect I/O Ports which interface IDE Controller:
   channels[ATA_PRIMARY  ].base  = (BAR0 & 0xFFFFFFFC) + 0x1F0 * (!BAR0);
   channels[ATA_PRIMARY  ].ctrl  = (BAR1 & 0xFFFFFFFC) + 0x3F6 * (!BAR1);
   channels[ATA_SECONDARY].base  = (BAR2 & 0xFFFFFFFC) + 0x170 * (!BAR2);
   channels[ATA_SECONDARY].ctrl  = (BAR3 & 0xFFFFFFFC) + 0x376 * (!BAR3);
   channels[ATA_PRIMARY  ].bmide = (BAR4 & 0xFFFFFFFC) + 0; // Bus Master IDE
   channels[ATA_SECONDARY].bmide = (BAR4 & 0xFFFFFFFC) + 8; // Bus Master IDE
    // 2- Disable IRQs:
   ide_write(ATA_PRIMARY  , ATA_REG_CONTROL, 2);
   ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

   // 3- Detect ATA-ATAPI Devices:
   for (int i = 0; i < 2; i++)
      for (j = 0; j < 2; j++) {
        unsigned char err = 0, type = IDE_ATA, status;
        ide_devices[count].Reserved = 0; // Assuming that no drive here.
 
        // (I) Select Drive:
        ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
        sleep(1); // Wait 1ms for drive select to work.
 
        // (II) Send ATA Identify Command:
        ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
        sleep(1); // This function should be implemented in your OS. which waits for 1 ms.
                 // it is based on System Timer Device Driver.
 
        // (III) Polling:
        if (ide_read(i, ATA_REG_STATUS) == 0) continue; // If Status = 0, No Device.
 
        while(1) {
           status = ide_read(i, ATA_REG_STATUS);
           if ((status & ATA_SR_ERR)) {err = 1; break;} // If Err, Device is not ATA.
           if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything is right.
        }
 
        // (IV) Probe for ATAPI Devices:
 
        if (err != 0) {
           unsigned char cl = ide_read(i, ATA_REG_LBA1);
           unsigned char ch = ide_read(i, ATA_REG_LBA2);
 
            if (cl == 0x14 && ch ==0xEB)
               type = IDE_ATAPI;
            else if (cl == 0x69 && ch == 0x96)
               type = IDE_ATAPI;
            else
               continue; // Unknown Type (may not be a device).
 
            ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
            sleep(1);
         }
 
         // (V) Read Identification Space of the Device:
         ide_read_buffer(i, ATA_REG_DATA, (unsigned int *) ide_buf, 128);
 
         // (VI) Read Device Parameters:
         ide_devices[count].Reserved     = 1;
         ide_devices[count].Type         = type;
         ide_devices[count].Channel      = i;
         ide_devices[count].Drive        = j;
         ide_devices[count].Signature    = *((unsigned short *)(ide_buf + ATA_IDENT_DEVICETYPE));
         ide_devices[count].Capabilities = *((unsigned short *)(ide_buf + ATA_IDENT_CAPABILITIES));
         ide_devices[count].CommandSets  = *((unsigned int *)(ide_buf + ATA_IDENT_COMMANDSETS));
 
         // (VII) Get Size:
         if (ide_devices[count].CommandSets & (1 << 26))
            // Device uses 48-Bit Addressing:
            ide_devices[count].Size   = *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
         else
            // Device uses CHS or 28-bit Addressing:
            ide_devices[count].Size   = *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA));
 
         // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
         for(k = 0; k < 40; k += 2) {
            ide_devices[count].Model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
            ide_devices[count].Model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];}
         ide_devices[count].Model[40] = 0; // Terminate String.
 
        count++;
    }
 
   // 4- Print Summary:
   for (int i = 0; i < 4; i++)
        if (ide_devices[i].Reserved == 1) {
            printf(" Found %s Drive %dGB - %s\n", (const char *[]){"ATA", "ATAPI"}[ide_devices[i].Type],         /* Type */
            ide_devices[i].Size / 1024 / 1024 / 2,               /* Size */
            ide_devices[i].Model);
        }
}

/* AHCI Stuff */

void kernel_main(multiboot_info_t* mbd, uint32_t magic)
{
	enable_cursor(0,15);
	currentColor = color(LIGHTGREEN,CYAN);
	print_init();
	currentColor = defaultColor;
	//multiboot_init(mbd,magic);
    ide_initialize(0x1F0, 0x3F6, 0x170, 0x376, 0x000);
	init_idt();
	kb_init();
	enable_interrupts();
	while(1);
}

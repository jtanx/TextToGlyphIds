#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

typedef struct Options {
	int bold;
	int italic;
	int isRange;
	const wchar_t *font_name;
	const wchar_t *text_file;
} Options;

int loadFonts(wchar_t *fonts_list)
{
	wchar_t buffer[BUFSIZ], *ptr;
	size_t count;
	int nLoaded = 0;

	while ((ptr = wcschr(fonts_list, L','))) {
		count = ptr - fonts_list;
		if (count < BUFSIZ) {
			wcsncpy_s(buffer, BUFSIZ, fonts_list, count);
			nLoaded += AddFontResourceEx(buffer, FR_PRIVATE, 0);
		}
		fonts_list = ++ptr;
	}
	if (fonts_list && *fonts_list) {
		nLoaded += AddFontResourceEx(fonts_list, FR_PRIVATE, 0);
	}

	return nLoaded;
}

int parseOpts(int argc, wchar_t *argv[], Options *opts)
{
	int index, next, j;

	for (index = 1, next = 1; index < argc; next = index) {
		if (*argv[index] == L'-') {
			for (j = 1; argv[index][j]; j++) {
				switch (argv[index][j]) {
					case 'b': opts->bold = !opts->bold; break;
					case 'i': opts->italic = !opts->italic; break;
					case 'r': opts->isRange = !opts->isRange; break;
					case 'f': 
						if (next + 1 >= argc) 
							return index;
						opts->font_name = argv[++next];
					break;
					case 'l':
						if (next + 1 >= argc)
							return index;
						printf("%d fonts loaded.\n", loadFonts(argv[++next]));
					break;
					default:
						return index;
				}
			} //End switch iteration
		} else { //No option; must be input file.
			opts->text_file = argv[index];
		}

		index = ++next;
	} //End argument iteration
	return -1;
}

void printUsage(wchar_t *argv[])
{
	fprintf(stderr, "\nTextToGlyphIds v0.0 - Retrieves the glyph ids associated with text.\n");
	fprintf(stderr, "Usage: %S [-bir] [-l font_file...] -f font_name text_file\n", argv[0]);
	fprintf(stderr, "  -b  Selects the bold font face.\n");
	fprintf(stderr, "  -i  Selects the italic font face.\n");
	fprintf(stderr, "  -r  The input file specifies ranges and not text.\n");
	fprintf(stderr, "  -l font_file[,...]  Loads a comma separated list of fonts.\n");
	fprintf(stderr, "  -f font_name  Selects the name of the font to use.\n");
	fprintf(stderr, "  text_file  Specifies the UTF-8 encoded text file to read.\n");
}

void getGlyphIds(FILE *fp, Options *opts)
{
	LOGFONT fontDetails = {0};
	HFONT font;
	HDC hdc = GetDC(NULL);
	HGDIOBJ old;
	WORD result[4];
	DWORD ret;
	wchar_t buffer[BUFSIZ];

	fontDetails.lfWeight = opts->bold ? 700 : 0;
	fontDetails.lfItalic = opts->italic;
	wcsncpy_s(fontDetails.lfFaceName, 32, opts->font_name, _TRUNCATE);
	fontDetails.lfCharSet = SHIFTJIS_CHARSET;
	font = CreateFontIndirect(&fontDetails);

	old = SelectObject(hdc, font);

	GetTextFace(hdc, BUFSIZ, buffer);
	printf("Selected text face: %S\n", buffer);
	ret = GetGlyphIndices(hdc, L"てst", 3, result, GGI_MARK_NONEXISTING_GLYPHS);

	SelectObject(hdc, old);
	DeleteObject(font);
	ReleaseDC(NULL, hdc);
}

void debug(void) {
	system("pause");
}

int wmain(int argc, wchar_t *argv[])
{
	Options opts = {0};
	FILE *fp;
	int i;

	atexit(debug);

	if (argc < 2) {
		printUsage(argv);
		return 1;
	} else if ((i = parseOpts(argc, argv, &opts)) >= 0) {
		printf("Invalid parameter specified in '%S'.\n", argv[i]);
		printUsage(argv);
		return 2;
	} else if (opts.font_name == NULL || opts.text_file == NULL) {
		printf("No font name or input text file specified.\n");
		printUsage(argv);
		return 3;
	} else if (wcslen(opts.font_name) >= 32) {
		printf("The font name is too long (max 31 characters).\n");
		return 4;
	}

	printf("Specified options:\n");
	printf("  Font: '%S' [%s%s]\n", opts.font_name, 
		   opts.bold ? "Bold" : "Regular",
		   opts.italic ? " Italic" : "");
	printf("  Input file: '%S'%s\n", opts.text_file,
		   opts.isRange ? " (range)" : "");
	
	if (_wfopen_s(&fp, opts.text_file, L"r,ccs=UTF-8")) {
		perror("Could not open input file");
		printUsage(argv);
		return 5;
	}
	
	getGlyphIds(fp, &opts);

	fclose(fp);
}
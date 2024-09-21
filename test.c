// cnm.h

#define str(x) #x
#define xstr(x) str(x)

#define cnm(symbol_name, ...) \
	__VA_ARGS__ \
	const char *const _cnm_csrc_##symbol_name = xstr(__VA_ARGS__);
#define cnm_dummy(symbol_name, type) \
	const char *const _cnm_csrc_##symbol_name = "#dummy "sizeof(type)"";


// c23
cnm(constexpr int blah = 23);

cnm_dummy(vec2, struct vec2);

cnm(struct foo {
	struct vec2 pos;
	int arr[blah];
};)
// void console_print(int a);
// const char *const _cnm_csrc_console_print = "struct foo { int arr[blah]; }";

#define cnm_code(symbol_name) _cnm_csrc_##symbol_name

void create_console_module(void) {
	cnm_module_parse_code("console", cnm_code(console_print));
	//								 _cnm_csrc_console_print
}





// cnm_preprocessor.c
//
//

// other scenario

#undef cnm
#define cnm(module)

// foo.h

cnm("foo") struct foo {
	int x, y;
};


// cnm_foo.cnm
struct foo {
	x: i32;
	y: i32;
}


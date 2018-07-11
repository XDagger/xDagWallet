#ifndef XDAG_MAIN_H
#define XDAG_MAIN_H



#ifdef __cplusplus
extern "C" {
#endif
	extern int xdag_init(int argc, char **argv, int isGui);

	extern int xdag_set_password_callback(int(*callback)(const char *prompt, char *buf, unsigned size));

#ifdef __cplusplus
};
#endif
#endif

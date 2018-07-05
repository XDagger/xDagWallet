/* dnet: external interface; T13.011-T13.794; $DVS:time$ */

#ifndef DNET_MAIN_H_INCLUDED
#define DNET_MAIN_H_INCLUDED

#define DNET_VERSION "T11.231-T13.714" /* $DVS:time$ */


#ifdef __cplusplus
extern "C" {
#endif
	extern int dnet_init(void);

	extern int dnet_generate_random_array(void *array, unsigned long size);

	/* выполнить действие с паролем пользователя:
	 * 1 - закодировать данные (data_id - порядковый номер данных, size - размер данных, измеряется в 32-битных словах)
	 * 2 - декодировать -//-
	 * 3 - ввести пароль и проверить его, возвращает 0 при успехе
	 * 4 - ввести пароль и записать его отпечаток в массив data длины 16 байт
	 * 5 - проверить, что отпечаток в массиве data соответствует паролю
	 * 6 - setup callback function to input password, data is pointer to function
	 *     int (*)(const char *prompt, char *buf, unsigned size);
	 */
	extern int dnet_user_crypt_action(unsigned *data, unsigned long long data_id, unsigned size, int action);

#ifdef __cplusplus
};
#endif

#endif

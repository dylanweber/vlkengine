#include <stdbool.h>
#ifndef CONFIG_H
#define CONFIG_H
#define BUILD_NUMBER 413
#define VERSION_NUMBER "0.0.413"
#ifndef NDEBUG
	static const bool enable_validation_layers = true;
#else
	static const bool enable_validation_layers = false;
#endif
#endif

#ifndef KEY_LOGGER_THREAD_H
#define KEY_LOGGER_THREAD_H

enum KeyLoggerState
{
	kKeyLoggerOff,
	kKeyLoggerAnonymized,
	kKeyLoggerFull
};

void SetKeyloggingState(KeyLoggerState);

#endif KEY_LOGGER_THREAD_H

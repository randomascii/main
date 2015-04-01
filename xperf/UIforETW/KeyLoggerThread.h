#pragma once

enum KeyLoggerState
{
	kKeyLoggerOff,
	kKeyLoggerAnonymized,
	kKeyLoggerFull
};

// Call SetKeyloggingState to set what type of key logging should
// be done. If the key logging thread is not started then it will
// be started. Call it with kKeyLoggerOff to shut down the key
// logging thread and stop key logging.
void SetKeyloggingState(KeyLoggerState);

#include "detect.h"
#include "write_rcms_log.h"

int main(int argc, char **argv)
{
	int global_state = STATE_START;

	while (global_state != STATE_TERMINATE)
	{
		switch (global_state)
		{
			case STATE_START:
				global_state = globalCall(START_GLOBAL, argc, argv);
				break;
			case STATE_CONFIG:
				global_state = configCall(START_CONFIG);
				break;
			case STATE_DETECT:
				global_state = detectCall(START_DETECT);
				break;
			case STATE_OUTPUT:
				global_state = outputCall(START_OUTPUT);
				break;
			case STATE_RESULT:
				global_state = resultCall(START_RESULT);
				break;
			case STATE_FINISHED:
				global_state = globalCall(START_DESTROY, 0, NULL);
				break;
			case STATE_TERMINATE:
				break;
			case CONFIG_CHECK:
				exit(0);
			default:
				xabort("no this global state");
				break;
		}
	}

	return 0;
}


#include <stdio.h>
#include <stdlib.h>

#include "supper_log.h"
#include "Utility.h"

CSupperLog slog;
int main(int argc, char *argv[])
{
	if(Utility::CheckProcExist(argv[1]) < 0)
	{
		printf("check failed \n");
	}
	else
	{
		printf("check ok\n");
		sleep(10);
	}
	return 0;
}


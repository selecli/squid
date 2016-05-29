#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>

const char* conf_deviceName = "net_device";

char conf_device[BUFSIZ];

static void str_ltrim(char*, const char*);
static void str_rtrim(char*, const char*);

void getSquidConf();

void getSquidConf()
{
    FILE* confFile;
    FILE* squidFile;
    char line[BUFSIZ];

    if ((confFile = fopen("../etc/monitor.conf", "r")) == NULL)
    {
        perror("monitor.conf");
        exit(1);
    }

    while (fgets(line, BUFSIZ, confFile) != NULL)
    {
        str_ltrim(line, "\t ");
        str_rtrim(line, "\t\r\n ");
        if (!strncmp(conf_deviceName, line, strlen(conf_deviceName)))
        {
            int i = 0;
            while (conf_device[i] = line[i + 1 + strlen(conf_deviceName)])
            {
                i++;
            }
            //printf("%s\n", conf_device);
        }
    }
}

static void str_ltrim(char* str, const char* tokens)
{
	if (str == NULL)
	{
		return;
	}
	char* p = str;
	while ((*p) && (strchr(tokens, *p)))
	{
		p++;
	}
	memmove(str, p, strlen(p) + 1);
}

static void str_rtrim(char* str, const char* tokens)
{
	if (str == NULL)
	{
		return;
	}
	char* p = str + strlen(str) - 1;
	while ((p >= str) && (strchr(tokens, *p)))
	{
		p--;
	}
	*++p = '\0';
	return;
}

main(int argc, char** argv)
{
	int filedes1[2], filedes2[2];
	int pid;
	FILE* ifTestFile;
	char line[BUFSIZ];
	int ethNum = 0;
	int RxNum = 0;
	time_t updateTime;
	FILE* poolFile;
	FILE* RxTxFile;
	char strLine[BUFSIZ];
    unsigned long dataCount = 0;
    float timeCount = 0.0;
    float inBandSpeed = 0.0;
    float outBandSpeed = 0.0;

	const char* ethName = "eth";
	const char* RxBytesName = "RX bytes:";

	char RxBytes[BUFSIZ];
	char TxBytes[BUFSIZ];
	
	char newUpdateTime[BUFSIZ];
	char oldRxBytes[BUFSIZ];
	char oldTxBytes[BUFSIZ];
	char oldUpdateTime[BUFSIZ];

	getSquidConf();

	if (pipe(filedes1) == -1)
	{
		perror ("pipe");
		exit(1);
	}

	if (pipe(filedes2) == -1)
	{
		perror ("pipe");
		exit(1);
	}

	if ((pid = fork()) == 0)
	{
		dup2(filedes1[0], fileno(stdin)); /* Copy the reading end of the pipe. */
		dup2(filedes2[1], fileno(stdout)); /* Copy the writing end of the pipe */

		/* Uncomment this if you want stderr sent too.
  
		dup2(filedes2[1], fileno(stderr));
  
		*/

		/* If execl() returns at all, there was an error. */

		if (execl("/sbin/ifconfig", "ifconfig","-a", (char*)0))
		{
			perror("execl");
			exit(128);
		}
	} else if (pid == -1)
	{
		perror("fork");
		exit(128);
	} else
	{
		FILE *program_input, *program_output, *output_file;
		int c;

		if ((program_input = fdopen(filedes1[1], "w")) == NULL)
		{
			perror("fdopen");
			exit(1);
		}

		if ((program_output = fdopen(filedes2[0], "r")) == NULL)
		{
			perror("fdopen");
			exit(1);
		}


		if ((output_file = fopen("/tmp/ifconfig.test", "w+")) == NULL)
		{
			perror ("ifconfig.test");
			exit(1);
		}

		//fputs(argv[2], program_input); /* Write the string */

		while ( (c = getc(program_output)) != EOF)
		{
			fputc(c, output_file);
			//printf("%c",c);
			close(filedes2[0]);
		}

		fclose(output_file);

		//Process ifconfig.test file
		if ((ifTestFile = fopen("/tmp/ifconfig.test", "r")) == NULL)
		{
			perror("ifTestFile");
			exit(1);
		}

		while (fgets(line, BUFSIZ, ifTestFile) != NULL)
		{
			str_ltrim(line, "\t\r\n ");		//include"  "(space) to trim left space
			str_rtrim(line, "\t\r\n");
			//if (!strncmp(ethName, line, strlen(ethName)))
			if (!strncmp(conf_device, line, strlen(conf_device)))
			{
				ethNum += 1;
			}

			if (!strncmp(RxBytesName, line, strlen(RxBytesName)))
			{
				RxNum += 1;
				if (ethNum == RxNum && RxNum == 1)
				{
					int i = 0;
					char* p;
					while ((RxBytes[i] = line[i +  strlen(RxBytesName)]) != ' ')
					{
						i++;
					}
					str_ltrim(RxBytes, " ");
					str_rtrim(RxBytes, " ");
					//printf("%s\n", RxBytes);

					p = strrchr(line, ':');
					i = 0;
					while ((TxBytes[i] = p[i + 1]) != ' ')
					{
						i++;
					}
					str_ltrim(TxBytes, " ");
					str_rtrim(TxBytes, " ");
					//printf("%s\n", TxBytes);

					//****************************************************
					//Get UpdateTime
					updateTime = time(NULL);

					//*******************************************************
					// Read old Rx, Tx data

					if ((RxTxFile = fopen("/tmp/netBandwidth.dat", "r")) == NULL)
					{
						//perror("RxTx.dat");
						//exit(1);
					}
                    else
					{
					while (fgets(line, BUFSIZ, RxTxFile) != NULL)
					{
						str_ltrim(line, "\t ");
						str_rtrim(line, "\t\r\n ");
						if (!strncmp("RX", line, strlen("RX")))
						{
							int i = 0;
							while (oldRxBytes[i] = line[i + strlen("RX")])
							{
								i++;
							}
							//printf("%s\n", oldRxBytes);
						}
						if (!strncmp("TX", line, strlen("TX")))
						{
							int i = 0;
							while (oldTxBytes[i] = line[i + strlen("TX")])
							{
								i++;
							}
							//printf("%s\n", oldTxBytes);
						}
						if (!strncmp("UT", line, strlen("UT")))
						{
							int i = 0;
							while (oldUpdateTime[i] = line[i + strlen("UT")])
							{
								i++;
							}
							//printf("%s\n", oldUpdateTime);
						}
					}
					fclose(RxTxFile);
					}

					//*******************************************************
					//Write To the RxTx.dat
					if ((RxTxFile = fopen("/tmp/netBandwidth.dat", "w+")) == NULL)
					{
						perror("RxTx.dat");
						exit(1);
					}
					sprintf(strLine,"RX");
					fputs(strLine, RxTxFile);
					sprintf(strLine, "%s\n",  RxBytes);
					fputs(strLine, RxTxFile);

					sprintf(strLine, "TX");
					fputs(strLine, RxTxFile);
					sprintf(strLine, "%s\n", TxBytes);
					fputs(strLine, RxTxFile);

					sprintf(strLine,"UT");
					fputs(strLine, RxTxFile);
					sprintf(strLine, "%ld\n", updateTime);
					fputs(strLine, RxTxFile);

					fclose(RxTxFile);

					//*******************************************************
					//Write To the netBandwidthMon.pool
					if ((poolFile = fopen("../pool/netBandwidthMon.pool", "w+")) == NULL)
					{
						perror("netBandwidthMon.pool");
						exit(1);
					}

					//printf("%s",RxBytes);
					//printf("%s\n",oldRxBytes);
					//unsigned long  temp =atoll(RxBytes);
					//printf("%u\n",temp);
					dataCount = atoll(RxBytes)-atoll(oldRxBytes);
					//printf("%u\n",dataCount);
                    sprintf(strLine,"%ld\n",updateTime);
                    //printf("%ld\n",updateTime);
					//printf("%d\n",atoi(strLine)-atoi(oldUpdateTime));
                    timeCount = atoi(strLine)-atoi(oldUpdateTime);
                    if(timeCount > 0)
                    {
                        inBandSpeed = dataCount / timeCount / 1024;
                    }
                    else
                    {
                        inBandSpeed = 0;
                    }
                    //printf("%f\n", inBandSpeed);

                    dataCount = atoll(TxBytes)-atoll(oldTxBytes);
					if(timeCount > 0)
					{
					    outBandSpeed = dataCount / timeCount / 1024;
					}
					else
					{
					    outBandSpeed = 0;
					}

                    sprintf(strLine, "%.2f\n", inBandSpeed);
					fputs(strLine, poolFile);

					sprintf(strLine, "%.2f\n", outBandSpeed);
					fputs(strLine, poolFile);

					sprintf(strLine, "%ld\n", updateTime);
					fputs(strLine, poolFile);

					fclose(poolFile);

				}//if (ethNum == RxNum)
			}
		}

//        exit(0);
	}

}

//********************************
//Desc:   get disk space
//Author: LuoJun.Zeng
//Date :  2006-04-15
//********************************

#include<stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <mntent.h>
#include <resolv.h>
#include <time.h>
#include <sys/vfs.h>
#include <arpa/inet.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#define BUFFER_SIZE 1000

#define VERSION "1.03"

char appName[BUFSIZ];

struct mount_entry
{
    char *me_devname;       // Device node pathname, including /dev.
    char *me_mountdir;      // Mount point directory pathname.
    char *me_type;          // ext2, ext3 etc.
    dev_t me_dev;           // Device number of me_mountdir.
    struct mount_entry *me_next;
};

struct fs_usage
{
    long fsu_blocks;    //Total blocks.
    long fsu_bfree;     //Free blocks avail to root.
    long fsu_bavail;    //Free blocks avail to normal;
    long fsu_files;     //Total free nodes;
    long fsu_ffree;     //Free file nodes.
};

struct fs_info
{
	char* FileSystem;
	char* Blocks;
	char* Used;
	char* Available;
	char* UsePercent;
	char* MountPoint;
};

const char* conf_diskdirName = "disk_dir";

char conf_diskdir[BUFSIZ];
int hasConfig = 0;

long diskFree;
long MinDiskFree;
long diskUsed;
long MaxDiskUsed;
char MinMountPoint[BUFSIZ];
char MaxMountPoint[BUFSIZ];

void getSquidConf();
static void str_ltrim(char*, const char*);
static void str_rtrim(char*, const char*);
struct mount_entry *read_filesystem_list(int, int);
void DisplayFilesystems();
int get_fs_usage(const char*, const char*, struct fs_usage*);
void show_dev(const char*, const char*, const char*);
void get_dev(const char*, const char*, const char*);
void get_AllDev(const char* ,  const char*, const char*);
void get_devInfo(const char*, const char*, const char* );
long adjust_blocks(long, int, int);
void writePool();

struct mount_entry *mount_list;

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
        str_ltrim(line, "\t");
        str_rtrim(line, "\t\r\n");
        if (!strncmp(conf_diskdirName, line, strlen(conf_diskdirName)))
        {
            int i = 0;
            while (conf_diskdir[i] = line[i + 1 + strlen(conf_diskdirName)])
            {
                i++;
            }
			hasConfig = 1;
            //printf("%s\n", conf_diskdir);
        }
    }
}

struct mount_entry *read_filesystem_list(int need_fs_type, int all_fs)
{
    struct mount_entry *mount_list;
    struct mount_entry *me;
    struct mount_entry *mtail;

    struct mntent *mnt;
    char *table=MOUNTED;
    FILE *fp;
    char *devopt;

    me=(struct mount_entry *)malloc(sizeof(struct mount_entry));
    me->me_next=NULL;
    mount_list=mtail=me;

    // MOUNTED_GETMNTENT1	
    fp=setmntent(table, "r");
    if (fp==NULL)
    {
        return NULL;
    }

    while ((mnt=getmntent(fp)))
    {
        if (!all_fs && (!strcmp (mnt->mnt_type, "ignore") || !strcmp (mnt->mnt_type, "auto")))
        {
            continue;
        }

        me=(struct mount_entry *)malloc(sizeof(struct mount_entry));
        me->me_devname=strdup(mnt->mnt_fsname);
        //printf("%s\n", me->me_devname);
        me->me_mountdir=strdup(mnt->mnt_dir);
        me->me_type=strdup(mnt->mnt_type);

        devopt=strstr(mnt->mnt_opts, "dev=");
        if (devopt)
        {
            if (devopt[4]=='0'&&(devopt[5]=='x'||devopt[5]=='X'))
            {
                me->me_dev=atoi(devopt+6);
            } else
            {
                me->me_dev=atoi(devopt+4);
            }
        } else
        {
            me->me_dev=(dev_t)-1;
        }
        me->me_next=NULL;

        mtail->me_next=me;
        mtail=me;
    }

    if (endmntent(fp)==0)
    {
        return NULL;
    }

    me=mount_list;
    mount_list=mount_list->me_next;
    free(me);
    return mount_list;
}

void DisplayFilesystems()
{
    struct mount_entry* me;
    if (mount_list == NULL)
    {
        perror("DF");
    } else
    {
        for (me = mount_list; me; me = me->me_next)
        {
            //me = mount_list;
            show_dev(me->me_devname, me->me_mountdir, me->me_type);
            //printf("%s %s\n", me->me_devname,me->me_mountdir);
        }
    }
}

int get_fs_usage(const char *path, const char *disk, struct fs_usage *fsp){
#define CONVERT_BLOCKS(B) adjust_blocks ((B), fsd.f_frsize ? fsd.f_frsize : fsd.f_bsize, 512)
    struct statvfs fsd;

    if (statvfs(path, &fsd)<0)
    {
        return -1;
    }
    fsp->fsu_blocks=CONVERT_BLOCKS (fsd.f_blocks);
    fsp->fsu_bfree=CONVERT_BLOCKS (fsd.f_bfree);
    fsp->fsu_bavail=CONVERT_BLOCKS (fsd.f_bavail);
    fsp->fsu_files=fsd.f_files;
    fsp->fsu_ffree=fsd.f_ffree;
}

void show_dev(const char *disk, const char *mount_point, const char *fstype)
{
    struct fs_usage fsu;
    long blocks_used;
    long blocks_percent_used;
    long inodes_used;
    long inodes_percent_used;
    const char *stat_file;
    char buffer[BUFFER_SIZE];

    stat_file=mount_point ? mount_point : disk;
    //For Test
    //stat_file= disk;
    get_fs_usage(stat_file, disk, &fsu);
    //printf("%s\n", disk);

    if (fsu.fsu_blocks>0)
    {
        fsu.fsu_blocks/=2*1024;
        fsu.fsu_bfree/=2*1024;
        fsu.fsu_bavail/=2*1024;

        if (fsu.fsu_blocks==0)
        {
            blocks_used=fsu.fsu_bavail=blocks_percent_used=0;
        } else
        {
            blocks_used=fsu.fsu_blocks-fsu.fsu_bfree;
            blocks_percent_used=(long)(blocks_used*100.0/(blocks_used+fsu.fsu_bavail)+ 0.5);
        }

        if (fsu.fsu_files==0)
        {
            inodes_used=fsu.fsu_ffree=inodes_percent_used=0;
        } else
        {
            inodes_used=fsu.fsu_files-fsu.fsu_ffree;
            inodes_percent_used=(long)(inodes_used*100.0/fsu.fsu_files+0.5);
        }

        if (!disk)
        {
            disk="-";
        }

        //printf("%-20s", disk);
        if ((int)strlen(disk)>20)
        {
            printf("\n%20s", "");
        }
        if (!fstype)
        {
            fstype="-";
        }
        if (strncmp("/auto/", mount_point, 6)==0)
        {
            mount_point+=5;
        }

        printf("DFspace_avail %lu MB %s\n\r", fsu.fsu_bavail, mount_point);
        printf("DFspace_%c used %lu %c full %s\n\r", '%', blocks_percent_used, '%', mount_point);

        // Some filesystems like reiserfs will not be able to display inode information.
        if (fsu.fsu_ffree>=0)
        {
            printf("DFinodes_avail %lu inodes %s\n\r", fsu.fsu_ffree, mount_point);
            printf("DFinodes_%c used %lu %c inodes %s\n\r", '%',inodes_percent_used,'%',mount_point);
        }
    }
    printf("\n");
}

long adjust_blocks(long blocks, int fromsize, int tosize){
    if (tosize<=0)
    {
        abort();
    }
    if (fromsize<=0)
    {
        return -1;
    }

    if (fromsize==tosize)
    {
        return blocks;
    } else if (fromsize>tosize)
    {
        // From 2048 to 512.
        return blocks*(fromsize/tosize);
    } else
    {
        // From 256 to 512.
        return(blocks+(blocks<0 ? -1 : 1))/(tosize/fromsize);
    }
}

void get_dev(const char *disk, const char *mount_point, const char *fstype)
{
    struct fs_usage fsu;
    long blocks_used;
    long blocks_percent_used;
    long inodes_used;
    long inodes_percent_used;
    const char *stat_file;
    char buffer[BUFFER_SIZE];

    stat_file=mount_point ? mount_point : disk;
    //For Test
    //stat_file= disk;
    get_fs_usage(stat_file, disk, &fsu);
    
    if (fsu.fsu_blocks>0)
    {
        fsu.fsu_blocks/=2*1024;
        fsu.fsu_bfree/=2*1024;
        fsu.fsu_bavail/=2*1024;

        if (fsu.fsu_blocks==0)
        {
            blocks_used=fsu.fsu_bavail=blocks_percent_used=0;
        } else
        {
            blocks_used=fsu.fsu_blocks-fsu.fsu_bfree;
            blocks_percent_used=(long)(blocks_used*100.0/(blocks_used+fsu.fsu_bavail)+ 0.5);
        }

        if (fsu.fsu_files==0)
        {
            inodes_used=fsu.fsu_ffree=inodes_percent_used=0;
        } else
        {
            inodes_used=fsu.fsu_files-fsu.fsu_ffree;
            inodes_percent_used=(long)(inodes_used*100.0/fsu.fsu_files+0.5);
        }

        if (!disk)
        {
            disk="-";
        }

        //printf("%-20s", disk);
        if ((int)strlen(disk)>20)
        {
            //printf("\n%20s", "");
        }
        if (!fstype)
        {
            fstype="-";
        }
        if (strncmp("/auto/", mount_point, 6)==0)
        {
            mount_point+=5;
        }

		diskFree = fsu.fsu_bavail;
		diskUsed = blocks_percent_used;
        //printf("DFspace_avail %lu MB %s\n\r", fsu.fsu_bavail, mount_point);
        //printf("DFspace_%c used %lu %c full %s\n\r", '%', blocks_percent_used, '%', mount_point);

		// Some filesystems like reiserfs will not be able to display inode information.
        if (fsu.fsu_ffree>=0)
        {
            //printf("DFinodes_avail %lu inodes %s\n\r", fsu.fsu_ffree, mount_point);
            //printf("DFinodes_%c used %lu %c inodes %s\n\r", '%',inodes_percent_used,'%',mount_point);
        }
    }
    //printf("\n");
}

void get_AllDev(const char *disk, const char* mount_point, const char* fstype)
{
	int filedes1[2];
	int filedes2[2];
	int pid;
	FILE* dfTestFile;
	char line[BUFSIZ];
	int lineCount = 0;

	struct fs_info* fs_list;
	diskFree = 0;
	MinDiskFree = 0;
	diskUsed = 0;
	MaxDiskUsed = 0;
	int MinCheck = 0;
	int MaxCheck = 0;

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

		if (execl("/bin/df", "df", (char*)0))
		{
			perror("execl");
			exit(128);
		}
	} 
	else if (pid == -1)
	{
		perror("fork");
		exit(128);
	}
	else
	{
		//Process Here
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


		if ((output_file = fopen("/tmp/df.test", "w+")) == NULL)
		{
			perror ("df.test");
			exit(1);
		}

		while ( (c = getc(program_output)) != EOF)
		{
			fputc(c, output_file);
			//printf("%c",c);
			close(filedes2[0]);
		}

		fclose(output_file);

		if ((dfTestFile = fopen("/tmp/df.test", "r")) == NULL)
		{
			perror("dfTestFile");
			exit(1);
		}

		while (fgets(line, BUFSIZ, dfTestFile) != NULL)
		{
			str_ltrim(line, "\t\r\n ");		//include"  "(space) to trim left space
			str_rtrim(line, "\t\r\n");

			MinCheck = 0;
		    MaxCheck = 0;

			lineCount++;
			if(lineCount > 1) //Don't Process the firest line
			{
				char* pResult = NULL;
				 pResult = strtok(line, " ");
				 int i = 0;
				 
				while (pResult != NULL)
				{
					//printf("%s\n", pResult);
					i++;
					if(i == 4)
					{
						if(lineCount == 2)
						{
							MinDiskFree = atoi(pResult);
							MinCheck = 1;
						}
						else
						{
							if(atoi(pResult) < MinDiskFree)
							{
								MinDiskFree = atoi(pResult);
								MinCheck = 1;
							}
							else
							{
								MinCheck = 0;
							}
						}
					}
					if(i == 5)
					{
						str_rtrim(pResult, "%");
						if(atoi(pResult) > MaxDiskUsed)
						{
							MaxDiskUsed = atoi(pResult);
							MaxCheck = 1;
						}
						else
						{
							MaxCheck = 0;
						}
					}
					if(i == 6)
					{
						if (MinCheck == 1)
						{
							sprintf(MinMountPoint, "%s",pResult);
						}
						if (MaxCheck == 1)
						{
							sprintf(MaxMountPoint, "%s",pResult);
						}
					}
					
					pResult = strtok(NULL, " ");
				}//while
			}//if(lineCount > 1)
		}//while
		MinDiskFree /= 1024;
		//printf("%d, %d\n", MinDiskFree, MaxDiskUsed);
		//printf("%s, %s\n",MinMountPoint, MaxMountPoint);
	}//else
}

void get_devInfo(const char* disk, const char* mount_point, const char* fstype)
{
	char* pResult = NULL;
	char line[BUFSIZ];
	sprintf(line,"%s",mount_point);
	 pResult = strtok(line, " ");
	 int i = 0;

	 MinDiskFree = 0;
	 MaxDiskUsed = 0;
	 int MinCheck = 0;
	 int MaxCheck = 0;
	 
	while (pResult != NULL)
	{
		i++;
		//printf("%s\n", pResult);
		get_dev(disk, pResult, fstype);
		//printf("%d,%d \n", diskFree, diskUsed);

		if(i == 1)
		{
			MinDiskFree = diskFree;
			MaxDiskUsed = diskUsed;
			MinCheck = 1;
			MaxCheck = 1;
		}
		else
		{
			if(diskFree < MinDiskFree)
			{
				MinDiskFree = diskFree;
				MinCheck = 1;
			}
			else
			{
				MinCheck = 0;
			}

			if(diskUsed > MaxDiskUsed)
			{
				MaxDiskUsed = diskUsed;
				MaxCheck = 1;
			}
			else
			{
				MaxCheck = 0;
			}
		}

		if(MinCheck == 1)
		{
			sprintf(MinMountPoint, "%s", pResult);
			//printf("hello world %s\n", MinMountPoint);
		}
		if(MaxCheck == 1)
		{
			sprintf(MaxMountPoint, "%s", pResult);
		}

		pResult = strtok(NULL, " ");
	}
	//printf("\n");
	//printf("%d,%d\n", MinDiskFree, MaxDiskUsed);
	//printf("%s, %s\n", MinMountPoint, MaxMountPoint);
}

void writePool()
{
    time_t updateTime;
    FILE* poolFile;
    char strLine[150];

	//****************************************************
	//Get UpdateTime
	updateTime = time(NULL);


	//*******************************************************
	//Write To the diskMon.pool
	if ((poolFile = fopen("../pool/diskSpaceMon.pool", "w+")) == NULL)
	{
		perror("diskSpaceMon.pool");
		exit(1);
	}
	sprintf(strLine, "diskFreeSpaceMB: %d\n", MinDiskFree);
	fputs(strLine, poolFile);

	sprintf(strLine, "diskUsedSpacePercent: %d\n", MaxDiskUsed);
	fputs(strLine, poolFile);

	sprintf(strLine, "diskFreeSpaceChkSection: %s\n", MinMountPoint);
	fputs(strLine, poolFile);

	sprintf(strLine, "diskUsedSpacePercentChkSection: %s\n", MaxMountPoint);
	fputs(strLine, poolFile);

	sprintf(strLine, "diskSpaceUpdateTime: %ld\n", updateTime);
	fputs(strLine, poolFile);

	fclose(poolFile);
}

main(int argc, char** argv)
{
    int i;

	sprintf(appName, "%s", *argv);
	str_ltrim(appName, "./");

	if(argc > 2)
	{
		printf("error argument.\n");
		printf("Use \"%s -h\" for help.\n", appName);
		exit(0);
	}
	argc--;
	argv++;
	while(argc && *argv[0] == '-')
	{
		if (!strcmp(*argv, "-v"))
		{
			printf("%s Version: %s\n", appName, VERSION);
			exit(0);
		}
		if (!strcmp(*argv, "-h"))
		{
			printf("Use \"%s -v\" for version number.\n", appName, VERSION);
			printf("Use \"%s -h\" for help.\n", appName, VERSION);
			exit(0);
		}
		else
		{
			printf("error argument.\n");
			printf("Use \"%s -h\" for help.\n", appName);
			exit(0);
		}

		argv++;
		argc--;
	}

    getSquidConf();
     mount_list = read_filesystem_list(1, 1);

     struct mount_entry* me;
     if (mount_list == NULL)
     {
         perror("DF");
     }
     else
     {
         me = mount_list;
         //show_dev(me->me_devname, conf_diskdir, me->me_type);
         if(hasConfig == 1)
		 {
		     get_devInfo(me->me_devname, conf_diskdir, me->me_type);
		 }
		 else
		 {
			 get_AllDev(me->me_devname,conf_diskdir,me->me_type);
		 }
         
     }

	 writePool();

    //DisplayFilesystems();
}

#include "conn.h"
#include "moov.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


struct atom_t ftyp_atom;
struct atom_t moov_atom;
struct atom_t mdat_atom;
uint64_t mdat_offset;
int mdat_flag = 0;

void write_char(unsigned char* outbuffer, int value)
{
  outbuffer[0] = (unsigned char)(value);
}

void write_int32(unsigned char* outbuffer, long value)
{
  outbuffer[0] = (unsigned char)((value >> 24) & 0xff);
  outbuffer[1] = (unsigned char)((value >> 16) & 0xff);
  outbuffer[2] = (unsigned char)((value >> 8) & 0xff);
  outbuffer[3] = (unsigned char)((value >> 0) & 0xff);
}

struct atom_t
{
  unsigned char type_[4];
  uint64_t size_;
  uint64_t start_;
  uint64_t end_;
}; 

#define ATOM_PREAMBLE_SIZE 8

unsigned int atom_header_size(unsigned char* atom_bytes)
{
  return (atom_bytes[0] << 24) +
         (atom_bytes[1] << 16) +
         (atom_bytes[2] << 8) +
         (atom_bytes[3]);
}

int atom_read_header(FILE* infile, struct atom_t * atom)
{
  unsigned char atom_bytes[ATOM_PREAMBLE_SIZE];

  atom->start_ = ftell(infile);

  fread(atom_bytes, ATOM_PREAMBLE_SIZE, 1, infile);
  memcpy(&atom->type_[0], &atom_bytes[4], 4);
  atom->size_ = atom_header_size(atom_bytes);
  atom->end_ = atom->start_ + atom->size_;
  if(!mdat_flag)
  	mdat_offset += atom->size_; 
  return 1;
}

void atom_write_header(unsigned char* outbuffer, struct atom_t* atom)
{
  int i;
  write_int32(outbuffer, atom->size_);
  for(i = 0; i != 4; ++i)
    write_char(outbuffer + 4 + i, atom->type_[i]);
}

int atom_is(struct atom_t const* atom, const char* type)
{
  return (atom->type_[0] == type[0] &&
          atom->type_[1] == type[1] &&
          atom->type_[2] == type[2] &&
          atom->type_[3] == type[3])
         ;
}

void atom_skip(FILE* infile, struct atom_t const* atom)
{
  fseek(infile, atom->end_, SEEK_SET);
}

static void atom_print(struct atom_t const* atom)
{
	return;
  printf("------------------------------------------------------------\nAtom(%c%c%c%c,%lld)\n\n", atom->type_[0], atom->type_[1],
          atom->type_[2], atom->type_[3], atom->size_);
}
void handle(FILE * src, FILE * dst, uint64_t size)
{
	char read_buf[4096]; 
	uint64_t read_bytes = 0;
	uint64_t write_bytes = 0;
	uint64_t want_bytes = 0;
	while(size) {
		memset(read_buf, 0 , 4096);
		if(size > 4096)
			want_bytes = 4096;
		else 
			want_bytes = size;
		read_bytes = fread(read_buf, 1, want_bytes, src);
		write_bytes = fwrite(read_buf, 1, read_bytes, dst);
		if(read_bytes != write_bytes) {
			printf("transforation has some error, please try again\n");
			exit(-1);
		}
		size -= write_bytes;
	}
}

void transformation(char *ftyp, char *moov, char *file_name)
{
	char file_buf[2048];
	FILE *fstp = NULL;
	FILE *fdtp = NULL; 
	snprintf(file_buf, 2047, "%s.old", file_name);
	rename(file_name, file_buf);
	fstp = fopen(file_buf, "r");
	fdtp = fopen(file_name, "w");
	fwrite(ftyp, 1, ftyp_atom.size_, fdtp);
	fwrite(moov, 1, moov_atom.size_, fdtp);
//	printf("mdat_offset: %lld\n", mdat_offset);
	fseek(fstp, mdat_offset, SEEK_SET);
	handle(fstp, fdtp, mdat_atom.size_);
	fclose(fstp);
	fclose(fdtp);
	unlink(file_buf);
}

int moov_data_handler(char* obj_path)
{
	FILE *infile;
	unsigned char *moov_data = 0;
	unsigned char *ftyp_data = 0;
	uint64_t filesize = 0;
	int flag = 1;
	int i = 0;
	int j = 0;
	infile = fopen(obj_path, "rb");  
	if (!infile)
		return -1;

	fseek(infile, 0, SEEK_END);
	filesize = ftell(infile);
	 fseek(infile, 0, SEEK_SET);
	{
		struct atom_t leaf_atom;
		while(ftell(infile) < filesize)
		{
			flag++;
			if(!atom_read_header(infile, &leaf_atom))
				break;

			atom_print(&leaf_atom);

			if(atom_is(&leaf_atom, "ftyp"))
			{
				ftyp_atom = leaf_atom;
				ftyp_data = malloc(ftyp_atom.size_);
				fseek(infile, ftyp_atom.start_, SEEK_SET);
				fread(ftyp_data, ftyp_atom.size_, 1, infile);
			}
			else
				if(atom_is(&leaf_atom, "moov"))
				{
					i = flag;
					moov_atom = leaf_atom;
					moov_data = malloc(moov_atom.size_);
					fseek(infile, moov_atom.start_, SEEK_SET);
					fread(moov_data, moov_atom.size_, 1, infile);
				}
				else
					if(atom_is(&leaf_atom, "mdat"))
					{
						j = flag;
						mdat_atom = leaf_atom;
						mdat_flag = 1;
						mdat_offset -= mdat_atom.size_;
					}
					atom_skip(infile, &leaf_atom);
		}
	}
	fclose(infile);
	if(i < j)
		 exit(0); 
	if (!moov_data)
		goto err;
	unsigned int mdat_start = (ftyp_data ? ftyp_atom.size_ : 0) + moov_atom.size_;
	if (!moov_seek(moov_data + ATOM_PREAMBLE_SIZE,
				moov_atom.size_ - ATOM_PREAMBLE_SIZE,
				&mdat_atom.start_, &mdat_atom.size_, mdat_start-mdat_atom.start_)) {
		goto err;
	}

	transformation(ftyp_data, moov_data, obj_path);
	free(ftyp_data);
	ftyp_data = NULL;
	free(moov_data);
	moov_data = NULL;
	return 1;
err:
	if (ftyp_data) {
		free(ftyp_data);
		ftyp_data = NULL;
	}
	if (moov_data) {
		free(moov_data);
		moov_data = NULL;
	}
	return -1;
}

int main (int argc, char **argv) 
{
	moov_data_handler(argv[1]);
	exit(0);
}

#include "inc/inc.h"

void format(char letter, int* ap);
bool string_n_compare(char* str1, char* str2, int n);

int open_file(int owner, int file_cluster, int open_mode);
bool close_file(int FIT_index);
bool write_file(int FIT_index, void* buffer, int offset, int count);
bool read_file(int FIT_index, void* buffer, int offset, int count);

int get_file_FIT_index(int file_cluster);
int get_file_cluster(int FIT_index);
bool load_cluster(int file_cluster, int FIT_index);

bool register_FIT(int owner_pid, int start_cluster, int open_mode, int* registered_index);
bool unregister_FIT(int FIT_index);

int find_file_cluster(char name[FILE_MAX_NAME], char ext[FILE_MAX_EXT], int working_directory_cluster);

int open_serial_port(int owner, int port_num, int open_mode);

file_interaction_table_entry FIT[MAX_FIT_ENTRY];

// -----------------------------------------------------------------------
// Initialization
bool io_init(void)
{
	int unused;

	if(!register_FIT(0x100, FAT_CLUSTER,
		FIT_FLAGS_READ | FIT_FLAGS_WRITE, &unused)) return false;
	if(!load_cluster(FAT_CLUSTER, unused)) return false;
	if(!register_FIT(0x100, ROOT_CLUSTER,
		FIT_FLAGS_READ | FIT_FLAGS_WRITE, &unused)) return false;
	if(!load_cluster(ROOT_CLUSTER, unused)) return false;
	return true;
}


// -----------------------------------------------------------------------
// Interaction
void printchar(int ch)
{
#asm
	push	bp
	mov	bp, sp
	push	ax
	push	bx

	mov	ah, #0x0e
	mov	al, byte ptr [bp + 4]
	xor	bx, bx
	int	0x10

	pop	bx
	pop	ax
	pop	bp
#endasm
}


void printhex(int num, bool uppercase)
{
	char str[8];
	int i;
	int temp;

	//  0   1   2   3   4   5   6   7
	// '0' 'x' '0' '0' '0' '0'  0   0
	for (i = 0; i < 8; i++) str[i] = '0';
	str[1] = 'x';
	str[6] = 0;	

	for (i = 5; i >= 2; i--)
	{
		temp = (num % 16);
		if (temp < 10) str[i] = temp + '0';
		else str[i] = uppercase ? temp - 10 + 'A' : temp - 10 + 'a';
		num /= 16;

		if (num == 0) break;
	}

	printstring(str);
}


void printnumber(int num)
{
	bool is_negative = num < 0 ? true : false;
	char str[8];
	int i;

	str[7] = 0;

	if(num == 0)
	{
		str[6] = '0';
		i = 6;
	}
	else
	{
		if(is_negative) num *= -1;

		for (i = 6; i >= 0; i--)
		{
			str[i] = (num % 10) + '0';
			num /= 10;
	
			if (num == 0) break;
		}

		if(is_negative) str[--i] = '-';
	}
	printstring(&str[i]);
}


void printstring(char* str, ...)
{
	char* ap = (char*)&str + 2;

	while (*str)
	{
		if (*str != '%')
		{
			printchar(*(str++));
			continue;
		}

		// if the letter is %
		// check for next letter *(++str)
		format(*(++str), (int*)ap);
		ap += 2;
		str++;
	}
}

void printline(char* str, ...)
{
	char* ap = (char*)&str + 2;

	while (*str)
	{
		if (*str != '%')
		{
			printchar(*(str++));
			continue;
		}

		// if the letter is %
		// check for next letter *(++str)
		format(*(++str), (int*)ap);
		ap += 2;
		str++;
	}

	printchar('\r');
	printchar('\n');
}


kobj_io io_open(int cs, int flags, kobj_io working_dir, char name[FILE_MAX_NAME], char ext[FILE_MAX_EXT], int open_mode)
{
	int ret;

	char* serial0 = "serial0";
	char* serial1 = "serial1";
	int start_cluster;

	if(string_n_compare(serial0, name, 8))
		return open_serial_port(cs, 0, open_mode);
	if(string_n_compare(serial1, name, 3))
		return open_serial_port(cs, 1, open_mode);

	start_cluster = find_file_cluster(name, ext,
				get_file_cluster(working_dir));
	ret = open_file(cs, start_cluster, open_mode);
	asm("mov	ax, word ptr [bp - 6]");
	syscall_return();
}


bool io_close(int cs, int flags, kobj_io koio)
{

}


bool io_write(int cs, int flags, kobj_io koio, void* buffer, int offset, int count)
{

}


bool io_read(int cs, int flags, kobj_io koio, void* buffer, int offset, int count)
{

}


// -----------------------------------------------------------------------
// Internal
void format(char letter, int* ap)
{
	switch (letter)
	{
		case 'c':
			printchar(*ap);
			break;
		case 'd':
			printnumber(*ap);
			break;
		case 's':
			printstring(*ap);
			break;
		case 'X':
			printhex(*ap, true);
			break;
		case 'x':
			printhex(*ap, false);
			break;
		default:
			printchar(' ');
			break;
	}
}


bool string_n_compare(char* str1, char* str2, int n)
{
	int i = 0;
	bool ret = true;

	while(str1[i] && str2[i] && i <= n)
	{
		if(str1[i] != str2[i])
		{
			ret = false;
			break;
		}
		i++;
	}

	return ret;
}


int open_file(int owner, int file_cluster, int open_mode)
{
	int FIT_index;
	
	if((FIT_index = get_file_FIT_index(file_cluster))
		!= UNREGISTERED_FIT_ENTRY)
		return FIT_index;
	if(!register_FIT(owner, file_cluster, open_mode, &FIT_index))
		return UNREGISTERED_FIT_ENTRY;
	if(!load_cluster(file_cluster, FIT_index))
		return UNREGISTERED_FIT_ENTRY;
	return FIT_index;
}


bool write_file(int FIT_index, void* buffer, int offset, int count)
{

}


bool read_file(int FIT_index, void* buffer, int offset, int count)
{

}


int get_file_FIT_index(int file_cluster)
{
	int i;

	for(i = 0; i < MAX_FIT_ENTRY; i++)
		if((FIT[i].flags & FIT_FLAGS_USED_ENTRY)
			&& FIT[i].start_cluster == file_cluster)
			return i;

	return UNREGISTERED_FIT_ENTRY;
}


int get_file_cluster(int FIT_index)
{
	return FIT[FIT_index].start_cluster;
}


bool load_cluster(int file_cluster, int FIT_index)
{
	int bx = FIT_index * 512;
	int cl = file_cluster & 0xFF;
	int ret;

#asm
	mov	ax, #0x3000
	mov	es, ax

	mov	ax, #0x0201
	mov	bx, word ptr [bp - 6]
	xor	ch, ch
	mov	cl, byte ptr [bp - 8]
	xor	dx, dx
	int	0x13

	jc	load_cluster_carry	; carry == 1 when error
	xor	ax, ax
	inc	ax
	jmp	load_cluster_end
load_cluster_carry:
	xor	ax, ax
load_cluster_end:
	mov	word ptr [bp - 10], ax
#endasm
	return ret;
}


bool register_FIT(int owner_pid, int start_cluster, int open_mode, int* registered_index)
{
	file_interaction_table_entry fite;
	bool empty_entry_found = false;
	int i;

	fite.owner_pid = owner_pid;
	fite.start_cluster = start_cluster;
	fite.flags = open_mode | FIT_FLAGS_USED_ENTRY;
	fite.file_pointer = 0;

	for(i = 0; i < MAX_FIT_ENTRY; i++)
	{
		if(!(FIT[i].flags & FIT_FLAGS_USED_ENTRY))
		{
			empty_entry_found = true;
			break;
		}
	}

	if(!empty_entry_found) return false;

	FIT[i] = fite;
	*registered_index = i;
	return true;
}


int find_file_cluster(char name[FILE_MAX_NAME], char ext[FILE_MAX_EXT], int working_directory_cluster)
{

}


int open_serial_port(int owner, int port_num, int open_mode)
{

}

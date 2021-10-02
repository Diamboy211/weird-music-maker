#include <ncurses.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int min(int a, int b)
{
	int r = a;
	if (a > b) r = b;
	return r;
}

void __set(char *o, char a, char b)
{
	o[0] = a; o[1] = b;
}

void get_note(char *out, int note, int mode)
{
	if (note == -128)
	{
		out[0] = ' ';
		out[1] = ' ';
		out[2] = ' ';
		out[3] = 0;
		return;
	}
	int oct = (note + 57) / 12;
	int nnote = (note + 57) % 12;
	out[3] = 0;
	if (mode)
	{
		switch (nnote)
		{
			case 0: __set(out, ' ', 'C'); break;
			case 1: __set(out, 'C', '#'); break;
			case 2: __set(out, ' ', 'D'); break;
			case 3: __set(out, 'D', '#'); break;
			case 4: __set(out, ' ', 'E'); break;
			case 5: __set(out, ' ', 'F'); break;
			case 6: __set(out, 'F', '#'); break;
			case 7: __set(out, ' ', 'G'); break;
			case 8: __set(out, 'G', '#'); break;
			case 9: __set(out, ' ', 'A'); break;
			case 10: __set(out, 'A', '#'); break;
			case 11: __set(out, ' ', 'B'); break;
		}
	}
	else
	{
		switch (nnote)
		{
			case 0: __set(out, ' ', 'C'); break;
			case 1: __set(out, 'D', 'b'); break;
			case 2: __set(out, ' ', 'D'); break;
			case 3: __set(out, 'E', 'b'); break;
			case 4: __set(out, ' ', 'E'); break;
			case 5: __set(out, ' ', 'F'); break;
			case 6: __set(out, 'G', 'b'); break;
			case 7: __set(out, ' ', 'G'); break;
			case 8: __set(out, 'A', 'b'); break;
			case 9: __set(out, ' ', 'A'); break;
			case 10: __set(out, 'B', 'b'); break;
			case 11: __set(out, ' ', 'B'); break;
		}
	}
	out[2] = '0' + oct;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("usage: sequencer [file-name]\n");
		return 1;
	}
	char *file_name = argv[1];
	char note[4];
	char *dbs_header = malloc(8);
	uint32_t dbs_length, dbs_ticks;
	uint16_t dbs_bpm;
	uint8_t dbs_instr;
	FILE *in_f = fopen(file_name, "rwb");
	fread(dbs_header, sizeof(char), 8, in_f);
	memcpy(&dbs_length, dbs_header, 4);
	memcpy(&dbs_bpm, dbs_header+4, 2);
	memcpy(&dbs_instr, dbs_header+6, 1);
	dbs_ticks = dbs_length / dbs_instr;
	printf("%d\t%d\t%d\n", dbs_length, dbs_bpm, dbs_instr);
	char *dbs_body = malloc(dbs_length);
	fread(dbs_body, sizeof(char), dbs_length, in_f);
	int acc_mode = 1;
	initscr();
	raw();
	noecho();
	int x, y, cx = 0, cy = 0, offset = 0;
	getmaxyx(stdscr, y, x);
	for(;;)
	{
		clear();
		mvprintw(0, 0, "ticks:%d", dbs_ticks);
		mvprintw(0, 12, "size:%d", dbs_length);
		for (int i = 0; i < x-1; i++)
		{
			mvprintw(1, i, "=");
		}
		for (int i = 0; i < min(y-4, dbs_ticks-offset); i++)
		{
			for (int j = 0; j < dbs_instr; j++)
			{
				get_note(note, dbs_body[(i+offset)*dbs_instr+j], acc_mode);
				mvprintw(i+2, 0, "%d", i+offset+1);
				mvprintw(i+2, j*6+5, "%s", note);
			}
		}
		for (int i = 0; i < x-1; i++)
		{
			mvprintw(y-2, i, "=");
		}
		mvprintw((cy-offset)+2, cx*6+4, ">");
		mvprintw(y-1, 0, "bpm:%d", dbs_bpm);
		refresh();

		int c = getch();
		if (c == 'q') break;
		switch (c)
		{
			case 'w':
				if (cy > 0)
				{
					cy--;
				}
				if (offset > 0 && cy - offset < 5)
				{
					offset--;
				}
				break;
			case 'a':
				if (cx > 0)
				{
					cx--;
				}
				break;
			case 's':
				if (cy < dbs_ticks-1)
				{
					cy++;
				}
				if (offset < dbs_ticks-y+4 && cy - offset > y-10)
				{
					offset++;
				}
				break;
			case 'd':
				if (cx < dbs_instr-1)
				{
					  cx++;
				}
				break;
			case 'j': dbs_body[cy*dbs_instr+cx]--; break;
			case 'k': dbs_body[cy*dbs_instr+cx]++; break;
			case 'n': dbs_body[cy*dbs_instr+cx] -= 12; break;
			case 'm': dbs_body[cy*dbs_instr+cx] += 12; break;
			case 'c': dbs_body[cy*dbs_instr+cx] = -128; break;
			case 'f': dbs_body[cy*dbs_instr+cx] = 0; break;
			case 'e':
				mvprintw(y/2, x/2-5, "save? (y/n)");
				dbs_length *= 2;
				memcpy(dbs_header, &dbs_length, 4);
				dbs_length /= 2;
				memcpy(dbs_header+4, &dbs_bpm, 2);
				memcpy(dbs_header+6, &dbs_instr, 1);
				if (getch() == 'y')
				{
					FILE *out_f = fopen("file3.dbs", "wb");
					fwrite(dbs_header, sizeof(char), 8, out_f);
					fwrite(dbs_body, sizeof(char), dbs_length, out_f);
					fwrite(dbs_body, sizeof(char), dbs_length, out_f);
					fclose(out_f);
				}
				break;
			case 'z': if (dbs_bpm > 1) dbs_bpm--; break;
			case 'x': dbs_bpm++; break;
			case 'r': acc_mode = (acc_mode == 1) ? 0 : 1; break; 
			case '*':
				for(;;) break;
				char *temp_buf = malloc(dbs_length + dbs_ticks);
				int i, j;
				for (i = 0; i < dbs_instr; i++)
				{
					for (j = 0; j < dbs_ticks; j++)
					{
						temp_buf[j*dbs_instr+j+i] = dbs_body[j*dbs_instr+i];
					}
					temp_buf[j*dbs_instr+j*2-1] = -128;
				}
				dbs_instr++;
				dbs_length += dbs_ticks;
				dbs_body = temp_buf;
				break;
			case '+':
				dbs_body = realloc(dbs_body, dbs_length+dbs_instr);
				dbs_body[dbs_length] = 0;
				dbs_body[dbs_length+1] = 0;
				dbs_length += dbs_instr;
				dbs_ticks++;
				break;
			case '(':
				for (int i = 0; i < dbs_length; i++)
					if (dbs_body[i] != -128)
						dbs_body[i]--;
				break;
			case ')':
				for (int i = 0; i < dbs_length; i++)
					if (dbs_body[i] != -128)
						dbs_body[i]++;
				break;
		}
	}
	endwin();
	return 0;
}

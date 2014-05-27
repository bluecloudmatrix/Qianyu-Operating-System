#include <stdio.h>
#include "bootpack.h"

#define MEMMAN_ADDR		0x003c0000

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], keybuf[32], mousebuf[128];
	int mx, my, i;
	struct MOUSE_DEC mdec;
	unsigned int memtotal;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse;
	unsigned char *buf_back, buf_mouse[256];

	// interrupt setting
	init_gdtidt();
	init_pic();	
	io_sti();	
	fifo8_init(&keyfifo, 32, keybuf);            // used for receiving interrupt information
	fifo8_init(&mousefifo, 128, mousebuf);	     // used for interrupt interrupt information
	io_out8(PIC0_IMR, 0xf9); // (11111001) 
	io_out8(PIC1_IMR, 0xef); // (11101111) 
	init_keyboard();	
	enable_mouse(&mdec);
	
	// memory operation
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); // 0x00001000 - 0x0009efff 
	memman_free(memman, 0x00400000, memtotal - 0x00400000);	

	// sheet control
	init_palette(); // setting palette 
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny); // setting sheets control structure
	
	// build background sheet
	sht_back  = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); 
	init_screen8(buf_back, binfo->scrnx, binfo->scrny); // initialize the show of screen
	sheet_slide(shtctl, sht_back, 0, 0);
	sheet_updown(shtctl, sht_back,  0);
	sprintf(s, "memory %dMB   free : %dKB",
			memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	putfonts8_asc(buf_back, binfo->scrnx, 8, 56, COL8_FFFFFF, "Hong Kong University");
	
	// build mouse sheet
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);        // initialize the show of mouse
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(shtctl, sht_mouse, mx, my);
	sheet_updown(shtctl, sht_mouse, 1);
	sprintf(s, "(%3d, %3d)", mx, my);
	putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	sheet_refresh(shtctl);  // show the vram


	//test
/*	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny); // initialize the show of screen
	boxfill8(binfo->vram, binfo->scrnx, COL8_0000FF, 0, 0, 79, 15); // hide coordinate
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "hku"); // show coordinate
*/

	
	for (;;) {
		io_hlt();
		io_cli();
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
			io_stihlt();
		} else {
			if(fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(buf_back, binfo->scrnx, COL8_0000FF, 0, 16, 15, 31);
				putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
				sheet_refresh(shtctl);
			} else if (fifo8_status(&mousefifo) != 0) {				
				i = fifo8_get(&mousefifo);
				io_sti();
				if (mouse_decode(&mdec, i) != 0) {
				
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) {
						s[1] = 'L';
					}
					if ((mdec.btn & 0x02) != 0) {
						s[3] = 'R';
					}
					if ((mdec.btn & 0x04) != 0) {
						s[2] = 'C';
					}
					boxfill8(buf_back, binfo->scrnx, COL8_0000FF, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					
					// move mouse cursor
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 16) {
						mx = binfo->scrnx - 16;
					}
					if (my > binfo->scrny - 16) {
						my = binfo->scrny - 16;
					}
					sprintf(s, "(%3d, %3d)", mx, my);
					boxfill8(buf_back, binfo->scrnx, COL8_0000FF, 0, 0, 79, 15); // hide coordinate
					putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);  // show coordinate
					sheet_slide(shtctl, sht_mouse, mx, my);
				}
			}
		}
	}
}

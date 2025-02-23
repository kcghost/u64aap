#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "./gopt.h"
#include "PR/os_vi.h"

#define MAJOR_VERSION 0
#define MINOR_VERSION 4
#define BUGFIX_VERSION 4

#ifdef __unix__
//unix
#include <unistd.h>
#elif defined _WIN32 || defined _WIN64
//windows
#include <Windows.h>
#else
#error "unknown platform"
#endif

#define xstr(s) str(s)
#define str(s) #s

void *options;
long fsize;
int verbosity=0;
int vl_level=0;
unsigned char *rom_blob;

// verbose printf
void vpf(int v_level, const char *fmt, ...) {
	va_list args;
	if(verbosity >= v_level) {
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	}
}

u8 patchDitherFilter_Testing() {
	u8 found=0;
	u32 offset=0;
	u32 prev_stop=0;
	u32 first_const_offset=0;
	u32 first_const_offset_real=0;
	u32 inner_offset_start=0;
	u32 inner_offset_start_real=0;
	u32 inner_offset_start_alt=0;
	u32 inner_offset_start_alt_real=0;
	u32 inner_offset_end=0;
	u32 diff=0;
	u32 last_const_nr=0;

	/*
	  ANDI -- Bitwise and immediate
	  Encoding:
	  0011 00ss ssst tttt iiii iiii iiii iiii
	  andi   t7    t6     4         0

	  andi    $t7, $t6, 0x40
	*/

	/*
	li: 24 REGISTER lower_bytes_a lower_bytes_b

	 24 01 FF EF
	 li $1 0xFFFFFFEF


	/*
	li 0xFFFFFFF7    24 XX FF F7   ->start file offset 88640
	li 0xFFFFFFFB    24 XX FF FB

	andi 0x10
	andi 0x20
	li 0xFFFFFFEF    24 XX FF EF
	->  andi 40      3X XX 00 40 -> 3X XX 00 71
	li 0xFFFFFCFF    24 XX FC FF
	(li 0cFFFEFFFF    34 XX FF FF)  -> end file offset 8871C = 0xDC -> 220 bytes (mario golf)

	star fox -> 22884 - 22784 = 0x100 -> 256 bytes
	duke nuken 71c34 - 71b34 = 0x100 -> 256 bytes

	*/

	unsigned char const nop[4] = { 0x00, 0x00, 0x00, 0x00 };
	unsigned char const FFF7[2] = { 0xFF, 0xF7 };
	unsigned char const FFFB[2] = { 0xFF, 0xFB };
	unsigned char const FFEF[2] = { 0xFF, 0xEF };
	unsigned char const FCFF[2] = { 0xFC, 0xFF };
	unsigned char const FFFF[2] = { 0xFF, 0xFF };
	unsigned char const _0040[2] = { 0x00, 0x40 };
	unsigned char const _0000[2] = { 0x00, 0x00 };
	unsigned char const _24XX[1] = { 0x24 };
	unsigned char const _34XX[1] = { 0x24 };

	for(offset=0; offset < fsize; ++offset) {
		if(!memcmp(FFF7, rom_blob + offset, 2)) {
			if(!memcmp(_24XX, rom_blob + offset-2, 1)) {
				// maybe first const in setViSpecialFeatures found
				vpf(2,"osViSetSpecialFeatures const 1 found?\n");
				last_const_nr=1;
				vpf(3, "diff: 0, offset=%x\n",offset);
				prev_stop=offset;
				first_const_offset=offset;

				for(offset=offset; offset < fsize; ++offset) {
					if(last_const_nr == 4) {
						break;
					}

					if(!memcmp(FFFB, rom_blob + offset, 2)) {
						if(!memcmp(_24XX, rom_blob + offset-2, 1)) {
							vpf(2, "osViSetSpecialFeatures const 2 found?\n");
							last_const_nr=2;

							diff=offset-prev_stop;
							vpf(3, "diff: %d\n",diff);

							inner_offset_start_alt=offset;
							vpf(3, "inner_offset_start_alt: %x\n",offset);

							//not found const to is too far away
							if(diff>100) { //76
								vpf(2, "diff too far\n");
								offset=prev_stop;
								break;
							} else {
								prev_stop=offset;
							}

							for(offset=offset; offset < fsize; ++offset) {
								if(last_const_nr==4) {
									break;
								}

								if(!memcmp(FFEF, rom_blob + offset, 2)) {
									if(!memcmp(_24XX, rom_blob + offset-2, 1)) {
										vpf(2, "osViSetSpecialFeatures const 3 found?\n");
										last_const_nr=3;

										diff=offset-prev_stop;
										vpf(3, "diff: %d\n",offset-prev_stop);

										inner_offset_start=offset; //0x40 between const 3 and const 4

										//not found const to is too far away
										if(diff>100) { //76
											vpf(1, "diff too far\n");
											offset=prev_stop;
											break;
										} else {
											prev_stop=offset;
										}


										for(offset=offset; offset < fsize; ++offset) {
											if(!memcmp(FCFF, rom_blob + offset, 2)) {
												if(!memcmp(_24XX, rom_blob + offset-2, 1)) {
													vpf(2, "osViSetSpecialFeatures const 4 found?\n");
													last_const_nr=4;
													diff=offset-prev_stop;
													vpf(3, "diff: %d\n",offset-prev_stop);
													//prev_stop=offset;

													//not found const to is too far away
													if(diff>100) { //76
														vpf(2, "diff too far\n");
														offset=prev_stop;
														break;
													} else {
														prev_stop=offset;
														vpf(3, "inner end offset found?\n");
														inner_offset_start_alt_real=inner_offset_start_alt;
														inner_offset_start_real=inner_offset_start;
														first_const_offset_real=first_const_offset;
														inner_offset_end=offset; //0x40 between const 3 and const 4
														break;
													}
												}

											}

										}

									}

								}

							}

						}

					}

				}
			}
		}
	}

	vpf(1, "\n\nstage 1 - bitmask\n");

	vpf(2, "inner_offset_start_real %x\n",inner_offset_start_real);
	vpf(2, "inner_offset_end %x\n",inner_offset_end);
	//near search
	if(inner_offset_start_real!=0) {

		for(offset=inner_offset_start_real; offset < inner_offset_end; ++offset) {
			//  printf("+");
			if(!memcmp(_0040, rom_blob + offset, 2)) {
				//fixme additionmal check

				unsigned char test[4] = {
					0x00, 0x00, 0x00, 0x00
				};

				memcpy(test, rom_blob + offset-2, 4);
				vpf(2, "-->> read test: %02x %02x %02x %02x \n", test[0], test[1], test[2], test[3]);

				if((test[0] & 0xFC) == 0x30) {
					vpf(2, "patchoffset found! @%x\n", offset);
					found = 1;
				} else {
					if(verbosity >= 2) {
						printf("patchoffset not found! @%lx\n", offset);
						printf("testing alternative spot :/\n");
					}
				}

				break;
			}
		}

		//30 8C 00 40
		if(found!=1)
			for(offset=inner_offset_start_alt_real; offset < inner_offset_end; ++offset) {
				if(!memcmp(_0040, rom_blob + offset, 2)) {

					unsigned char test[4] = {
						0x00, 0x00, 0x00, 0x00
					};

					memcpy(test, rom_blob + offset-2, 4);
					if(verbosity >= 2) {
						printf("-->> alt offset: %lx\n", offset);
						printf("-->> read test alt: %02x %02x %02x %02x \n", test[0], test[1], test[2], test[3]);
					}
					//  printf("bitshift: c==%x\n",test[0]>>2);

					if((test[0] & 0xFC) == 0x30) {
						//if(test[0]>>2 == 0xc){
						vpf(1, "patchoffset found alt! @%x\n", offset);
						found=1;
					} else {
						vpf(1, "patchoffset not found! @%x\n", offset);
					}

					break;
				}

			}
	}

	if(found==1) {
		vpf(1, " ### patching dither filter bitmask...\n");
		memcpy(rom_blob + offset, _0000, 2);
	}

	//stage 2
	if(inner_offset_start_real!=0 && found==1) {


		u8 branch_offset_found=0;

		u8 b=0;
		u32 branch[8]= {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};

		vpf(1, "\n\nstage 2 - branch\n");
		//for(offset=inner_offset_end+2; offset < inner_offset_end+64; offset+=4){
		for(offset=first_const_offset_real+2-64; offset < inner_offset_end+64; offset+=4) {

			unsigned char test[4] = { 0x00, 0x00, 0x00, 0x00 };

			memcpy(test, rom_blob + offset, 4);


			//  printf("-->> beqz offset: %x\n", offset);
			//  printf("-->> beqz/beqzl read test: %02x %02x %02x %02x \n", test[0], test[1], test[2], test[3]);

			//[  000100]01 10000000 00000000 00001011
			//beqz 0x100
			if((test[0] & 0xFC) == 0x10) { //beqz
				if((test[1] & 0x1F) == 0x00) {
					vpf(2, "-->> beqz/beqzl read: %02x %02x %02x %02x \n", test[0], test[1], test[2], test[3]);
					vpf(2, "++ next beqz found!\n");
					branch_offset_found=offset;
					branch[b++]=offset;
					//  break;

				}
			}

			if((test[0] & 0xFC) == 0x50) { //beqzl
				if((test[1] & 0x1F) == 0x00) { // zero
					vpf(2, "-->> beqz/beqzl read: %02x %02x %02x %02x \n", test[0], test[1], test[2], test[3]);
					vpf(2, "++ next beqzl found!\n");
					branch_offset_found=offset;
					branch[b++]=offset;
					//  break;

				}
			}


		}

		vpf(2, "branches (%d/8)\n",b);

		/*

		noop branch to activate
		zero 's' to deactivate


		Gamma correction ON/OF
		0 VI_GAMMA_ON a
		1 VI_GAMMA_OFF c

		Gamma dithering ON/OFF
		2 VI_GAMMA_DITHER_ON b
		3 VI_GAMMA_DITHER_OFF g

		DIVOT ON/OFF
		4 VI_DIVOT_ON e
		5 VI_DIVOT_OFF d

		Dither filter ON/OFF
		6 VI_DITHER_FILTER_ON j
		7 _VI_DITHER_FILTER_OFF f
		*/

		//0x00 don't touch 0x01 on 0x02 off
		u8 setting[8]= {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

		printf("\nGamma correction:\t");
		if( gopt( options, 'a' ) ) {
			setting[0]=0x01;
			setting[1]=0x02;
			printf("ON");
		} else if( gopt( options, 'c' ) ) {
			setting[0]=0x02;
			setting[1]=0x01;
			printf("OFF");
		} else {
			printf("untouched");
		}
		printf("\n");

		printf("Gamma dithering:\t");
		if( gopt( options, 'b' ) ) {
			setting[2]=0x01;
			setting[3]=0x02;
			printf("ON");
		} else if( gopt( options, 'g' ) ) {
			setting[2]=0x02;
			setting[3]=0x01;
			printf("OFF");
		} else {
			printf("untouched");
		}
		printf("\n");

		printf("DIVOT:\t\t\t");
		if( gopt( options, 'e' ) ) {
			setting[4]=0x01;
			setting[5]=0x02;
			printf("ON");
		} else if( gopt( options, 'd' ) ) {
			setting[4]=0x02;
			setting[5]=0x01;
			printf("OFF");
		} else {
			printf("untouched");
		}
		printf("\n");

		printf("Dither filter:\t\t");
		if( gopt( options, 'j' ) ) {
			setting[6]=0x01;
			setting[7]=0x02;
			printf("ON");
		} else if( gopt( options, 'f' ) ) {
			setting[6]=0x02;
			setting[7]=0x01;
			printf("OFF");
		} else {
			printf("untouched");
		}
		printf("\n\n");


		vpf(1, " ### patching branches...\n");
		if(branch_offset_found!=0) {
			//nop out
			u8 z=0;
			for(z=0; z<8; z++) {

				if(setting[z]==0x1) {
					//noop branche
					memcpy(rom_blob + branch[z], nop, 4);
					vpf(2, "-->> nop: %02x %02x %02x %02x \n", nop[0], nop[1], nop[2], nop[3]);
				} else   if(setting[z]==0x02) {
					//zero s
					unsigned char rom_branch[2] = { 0x00, 0x00};
					memcpy(rom_branch, rom_blob + branch[z], 2);
					rom_branch[0]&=0xFC;
					rom_branch[1]&=0x1F;
					memcpy(rom_blob + branch[z], rom_branch, 2);
					vpf(2, "-->> rb: %02x %02x \n", rom_branch[0], rom_branch[1]);
				}

			}

			found=2;
		}

		/*
		if(branch_offset_found!=0){
		  //nop out
		  printf(" ### patching branch...\n");
		    memcpy(rom_blob + offset, nop, 4);
		    found=2;
		}
		*/

	}
	/*
	pokemon
	beqzl (zero) t9 label
	53 20 00 0F
	0101 0011 0010 0000 0000 0000 000 01111
	0001 00ss ssst tttt iiii iiii iiii iiii

	beqz  (zero) t4 label
	11 80 00 0B
	0001 0001 1000 0000 0000 0000 0000 1011
	0001 00ss ssst tttt iiii iiii iiii iiii

	mario
	beqz
	10 40 00 0C
	0001 0000 0100 0000 0000 0000 0000 1100
	0001 00ss ssst tttt iiii iiii iiii iiii


	after 0xFFFFFCFF in +4bytes sections for the next branch

	search for beqz
	0001 00ss sss0 0000 iiii iiii iiii iiii

	search for beqzl
	0101 00ss sss0 0000 iiii iiii iiii iiii

	before 0xFFFEFFFF

	*/

	return found;
}

void patchCtrl(u8 mode, u32 offset) {
	unsigned char value[4];

	u32 ctrl = osViModeTable[mode].comRegs.ctrl;
	value[0] = ctrl>>24;
	value[1] = ctrl>>16;
	value[2] = ctrl>>8;
	value[3] = ctrl;

	memcpy(rom_blob + offset, value, 4);
}

size_t searchOffset_vl(u8 level, u8 ucode) {
	size_t offset = 0;

	unsigned char testme[4] = {
		//	0x00, 0x00, 0x31, 0x1e
		//  0x00, 0x00, 0x00, 0x00
		0xB9, 0x00, 0x03, 0x1D //fast3d e.g. sm64
	};

	if(ucode == 1) { //F3DEX2 e.g. zelda
		testme[0]==0xE2;
	}

	vpf(1, "\nsearching vl entries...\n");

	//__font_data+array_offset single_character
	for(offset=0; offset < fsize; ++offset) {
		if(!memcmp(testme, rom_blob + offset, 4)) {
			vpf(1, "vl entry %02x->", offset, rom_blob[offset + 7] );
			vpf(2, "vl entry at pos: 0x%x\t%02x->", offset, rom_blob[offset + 7] );

			if(level>4) {
				//patch all location
				rom_blob[offset + 7] &= ~0x8;
			} else {
				if(level>0) {
					if(rom_blob[offset + 7]==0x78) {
						rom_blob[offset + 7] &= ~0x8;
					}
				}

				if(level>1) {
					if(rom_blob[offset + 7]==0x48) {
						rom_blob[offset + 7] &= ~0x8;
					}
				}

				if(level>2) {
					if(rom_blob[offset + 7]==0x58) {
						rom_blob[offset + 7] &= ~0x8;
					}
				}

				if(level>3) {
					if(rom_blob[offset + 7]==0xd8) {
						rom_blob[offset + 7] &= ~0x8;
					}
				}
			}

			vpf(1, "%02x\n", rom_blob[offset + 7] );
		}
	}

	return offset;
}

int compare_u32(u32 reg, const void* p2) {
	u8 test[4];
	test[0]=reg>>24;
	test[1]=reg>>16;
	test[2]=reg>>8;
	test[3]=reg;
	return memcmp(test, p2, 4);
}

size_t searchOffset(u8 scanmode) {
	size_t offset = 0;
	u_int32_t ctrl;
	int count;

	vpf(1, "\nsearching for video table offset in rom...\n");

	ctrl = osViModeTable[scanmode].comRegs.ctrl;

	//__font_data+array_offset single_character
	for(offset=0; offset < fsize; ++offset) {
		if(compare_u32(ctrl, rom_blob + offset) == 0) {
			count = 0;

			if(compare_u32(osViModeTable[scanmode].comRegs.width,    rom_blob + offset + 4)  == 0) {
				count++;
			}
			if(compare_u32(osViModeTable[scanmode].comRegs.burst,    rom_blob + offset + 8)  == 0) {
				count++;
			}
			if(compare_u32(osViModeTable[scanmode].comRegs.vSync,    rom_blob + offset + 12) == 0) {
				count++;
			}
			if(compare_u32(osViModeTable[scanmode].comRegs.hSync,    rom_blob + offset + 16) == 0) {
				count++;
			}
			if(compare_u32(osViModeTable[scanmode].comRegs.leap,     rom_blob + offset + 20) == 0) {
				count++;
			}
			if(compare_u32(osViModeTable[scanmode].comRegs.hStart,   rom_blob + offset + 24) == 0) {
				count++;
			}
			if(compare_u32(osViModeTable[scanmode].comRegs.xScale,   rom_blob + offset + 28) == 0) {
				count++;
			}
			if(compare_u32(osViModeTable[scanmode].comRegs.vCurrent, rom_blob + offset + 32) == 0) {
				count++;
			}

			if(count>=3) {
				if(count==8) {
					vpf(1, "vt entry found!\n");
				} else {
					vpf(1, "vt entry maybe found! (%d/8) trying anyway...\n" ,count);
				}
				break;
			} else {
				printf("video table could not be found!\n");
			}

		}

	}

	return offset;
}

size_t patchMode(u8 from_mode, u8 to_mode) {
	vpf(1, "search for mode: %d\n",from_mode);

	u32 offset = searchOffset(from_mode);

	if(offset == fsize) {
		vpf(1, "error: mode: %d no VT offset found\n", from_mode);
		return -1;
	}

	vpf(2, "found at offset: %08x\n",offset);

	patchCtrl(to_mode, offset);

	return 0;
}

int main(int argc, const char **argv) {
	printf("-== N64 AA-patcher ==- by saturnu\n\n");

	options = gopt_sort( & argc, argv, gopt_start(
		gopt_option( 'h', 0, gopt_shorts( 'h' ), gopt_longs( "help" )),
		gopt_option( 'z', 0, gopt_shorts( 'z' ), gopt_longs( "version" )),
		gopt_option( 'v', GOPT_REPEAT, gopt_shorts( 'v' ), gopt_longs( "verbose" )),

		gopt_option( 'a', 0, gopt_shorts( 'a' ), gopt_longs( "gc-on" )),
		gopt_option( 'c', 0, gopt_shorts( 'c' ), gopt_longs( "gc-off" )),

		gopt_option( 'b', 0, gopt_shorts( 'b' ), gopt_longs( "gd-on" )),
		gopt_option( 'g', 0, gopt_shorts( 'g' ), gopt_longs( "gd-off" )),

		gopt_option( 'e', 0, gopt_shorts( 'e' ), gopt_longs( "di-on" )),
		gopt_option( 'd', 0, gopt_shorts( 'd' ), gopt_longs( "di-off" )),

		gopt_option( 'j', 0, gopt_shorts( 'j' ), gopt_longs( "df-on" )),
		gopt_option( 'f', 0, gopt_shorts( 'f' ), gopt_longs( "df-off" )),

		gopt_option( 'q', 0, gopt_shorts( 'q' ), gopt_longs( "dummy" )),
		//gopt_option( 't', 0, gopt_shorts( 't' ), gopt_longs( "no-table" )),
		//gopt_option( 'l', 0, gopt_shorts( 'l' ), gopt_longs( "videolist" )),
		gopt_option( 'l', GOPT_REPEAT, gopt_shorts( 'l' ), gopt_longs( "videolist" )),

		gopt_option( 'k', 0, gopt_shorts( 'k' ), gopt_longs( "fast3d" )),
		gopt_option( '2', 0, gopt_shorts( '2' ), gopt_longs( "f3dex2" )),

		gopt_option( 's', 0, gopt_shorts( 's' ), gopt_longs( "swap" )),
		gopt_option( 'n', 0, gopt_shorts( 'n' ), gopt_longs( "no-anti-aliasing" )),

		gopt_option( 'o', GOPT_ARG, gopt_shorts( 'o' ), gopt_longs( "output" )),
		gopt_option( 'i', GOPT_ARG, gopt_shorts( 'i' ), gopt_longs( "input" ))  )
	);

	if( gopt( options, 'h' ) ) {

#ifdef __unix__
		//unix
		printf("Syntax: u64aap [options] -i input.z64 -o output.z64\n\n");
#elif defined _WIN32 || defined _WIN64
		//windows
		printf("Syntax: u64aap [options] -i input.z64 -o output.z64\n\n");
#else
#error "unknown platform"
#endif

		printf("u64aap - -== N64 AA-patcher ==-\n" );
		printf("by saturnu <tt@anpa.nl>\n\n" );

		printf("Input/Output: (required)\n");
		printf(" -i, --input=filename.z64\tN64 Rom in z64 format\n" );
		printf(" -o, --output=filename.z64\tN64 Rom in z64 format\n" );


		printf("\nFilter options: (default: untouched)\n");
		printf(" -a, --gc-on\t\tset Gamma correction ON\n" );
		printf(" -c, --gc-off\t\tset Gamma correction OFF\n" );

		printf(" -b, --gd-on\t\tset Gamma dithering ON\n" );
		printf(" -g, --gd-off\t\tset Gamma dithering OFF\n" );

		printf(" -e, --di-on\t\tset DIVOT ON\n" );
		printf(" -d, --di-off\t\tset DIVOT OFF\n" );

		printf(" -j, --df-on\t\tset Dither filter ON\n" );
		printf(" -f, --df-off\t\tset Dither filter OFF\n" );

		printf("\nExtra options:\n");
		printf(" -q, --dummy\t\tjust test - don't output file\n" );
		printf(" -s, --swap\t\tswap VideoTable region (new experimental)\n" );
		printf(" -n, --no-anti-aliasing\tdisable AA in VideoTable, too\n" );
		//printf(" -t, --no-table\t\tdon't touch VideoTable at all\n" );
		printf(" -k, --fast3d\t\tsearch F3D SETOTHERMODE_L (highly experimental)\n" );
		printf(" -2, --f3dex2\t\tsearch F3DEX2 SETOTHERMODE_L (highly experimental)\n" );
		printf(" -l, --videolist\tpatch VideoList - [stackable] (highly experimental)\n" );


		printf("\nInformation:\n");
		printf(" -h, --help\t\tdisplay this help and exit\n" );
		printf(" -v, --verbose\t\tverbose\n" );
		printf(" -z, --version\t\tversion info\n" );

		return 0;
	}


	if( gopt( options, 'z' ) ) {
		printf("u64aap version v%d.%d.%d\n", MAJOR_VERSION, MINOR_VERSION, BUGFIX_VERSION );
		printf("fork version: " xstr(GIT_ORIGIN) " " xstr(GIT_VERSION) "\n");
		return 0;
	}

	/*
	  if(argv[1]==NULL){
			printf("error: please use a z64 rom as parameter\n");
			return 0;
		}
	  */

	if( !gopt( options, 'i' ) ) {
		//needed every time
		printf("input file missing! use 'u64aap -h' for help\n");
		return 0;
	}

	if( !gopt( options, 'o' ) ) {
		//needed every time
		printf("output file missing! use 'u64aap -h' for help\n");
		return 0;
	}

	if( gopt( options, 'l' ) > 0) {
		if( !(gopt( options, 'k' ) || gopt( options, '2' )) ) {

			printf("uCode flag missing! use 'u64aap -h' for help\n");
			return 0;
		}
	}

	if( gopt( options, 'a' ) && gopt( options, 'c' )) {
		printf("error: could not set Gamma correction ON and OFF at the same time\n" );
		return 0;
	}

	if( gopt( options, 'b' ) && gopt( options, 'g' )) {
		printf("error: could not set Gamma dithering ON and OFF at the same time\n" );
		return 0;
	}

	if( gopt( options, 'e' ) && gopt( options, 'd' )) {
		printf("error: could not set DIVOT ON and OFF at the same time\n" );
		return 0;
	}

	if( gopt( options, 'j' ) && gopt( options, 'f' )) {
		printf("error: could not set Dither filter ON and OFF at the same time\n" );
		return 0;
	}

	if(gopt( options, 'v' ) < 10) {
		verbosity = gopt( options, 'v' );
	} else {
		verbosity = 9;
	}

	if(gopt( options, 'l' ) < 10) {
		vl_level = gopt( options, 'l' );
	} else {
		vl_level = 9;
	}

	const char *filename_input;
	const char *filename_output;

	if( gopt( options, 'i' ) ) {

		if( gopt_arg( options, 'i', & filename_input ) && strcmp( filename_input, "-" ) ) {

			FILE *f;
			f=fopen(filename_input, "rb");

			if (f == NULL) {
				printf("error: Faild to open rom\n");
				return 0;
			} else {
				printf("%s found\n", filename_input);
			}

			fseek(f, 0, SEEK_END);
			fsize = ftell(f);
			fseek(f, 0, SEEK_SET);

			vpf(2, "Rom size: %d\n",fsize);

			rom_blob = malloc(fsize + 1);
			fread(rom_blob, fsize, 1, f);
			fclose(f);

		}

	}

	int patch_counter=0;

	if( vl_level ) {
		if( gopt( options, 'k' ) ) {
			printf("\n\nFast3D: \n");
			searchOffset_vl(vl_level, 0);
		}

		if( gopt( options, '2' ) ) {
			printf("F3DEX2: \n");
			searchOffset_vl(vl_level, 1);
		}
	}

	if( gopt( options, 'o' ) ) {
		vpf(1, "\n\nstage 0 - video table\n");

		if( gopt( options, 'n' ) ) {
		//disable anti aliasing

			if( !gopt( options, 's' ) ) {

				vpf(1, "\n\nstage 0 - disable aa in video table\n");

				if(patchMode(OS_VI_NTSC_LAN1, OS_VI_NTSC_LPN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_LAF1, OS_VI_NTSC_LPF1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_LAN2, OS_VI_NTSC_LPN2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_LAF2, OS_VI_NTSC_LPF2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_HAN1, OS_VI_NTSC_HPN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_HAF1, OS_VI_NTSC_HPF1)==0) {
					patch_counter++;
				}

				if(patchMode(OS_VI_PAL_LAN1, OS_VI_PAL_LPN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_LAF1, OS_VI_PAL_LPF1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_LAN2, OS_VI_PAL_LPN2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_LAF2, OS_VI_PAL_LPF2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_HAN1, OS_VI_PAL_HPN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_HAF1, OS_VI_PAL_HPF1)==0) {
					patch_counter++;
				}

				if(patchMode(OS_VI_MPAL_LAN1, OS_VI_MPAL_LPN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_LAF1, OS_VI_MPAL_LPF1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_LAN2, OS_VI_MPAL_LPN2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_LAF2, OS_VI_MPAL_LPF2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_HAN1, OS_VI_MPAL_HPN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_HAF1, OS_VI_MPAL_HPF1)==0) {
					patch_counter++;
				}


			} else { //swap video tables, too

				if(verbosity >= 1) {
					printf("stage 0 - disable aa in video table + region swap\n");
					printf("stage 0.1 ntsc->pal\n");
				}
				if(patchMode(OS_VI_NTSC_LAN1, OS_VI_PAL_LPN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_LAF1, OS_VI_PAL_LPF1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_LAN2, OS_VI_PAL_LPN2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_LAF2, OS_VI_PAL_LPF2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_HAN1, OS_VI_PAL_HPN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_HAF1, OS_VI_PAL_HPF1)==0) {
					patch_counter++;
				}


				vpf(1, "stage 0.2 pal->ntsc\n");
				if(patchMode(OS_VI_PAL_LAN1, OS_VI_NTSC_LPN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_LAF1, OS_VI_NTSC_LPF1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_LAN2, OS_VI_NTSC_LPN2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_LAF2, OS_VI_NTSC_LPF2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_HAN1, OS_VI_NTSC_HPN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_HAF1, OS_VI_NTSC_HPF1)==0) {
					patch_counter++;
				}

				vpf(1, "stage 0.3 - mpal->pal\n");
				if(patchMode(OS_VI_MPAL_LAN1, OS_VI_PAL_LPN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_LAF1, OS_VI_PAL_LPF1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_LAN2, OS_VI_PAL_LPN2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_LAF2, OS_VI_PAL_LPF2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_HAN1, OS_VI_PAL_HPN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_HAF1, OS_VI_PAL_HPF1)==0) {
					patch_counter++;
				}
			}

		//end disable anti aliasing
		} else {
		//normal swap
			if( gopt( options, 's' ) ) {

				if(verbosity >= 1) {
					printf("region swap only\n");
					printf("stage 0.1 ntsc->pal\n");
				}
				if(patchMode(OS_VI_NTSC_LAN1, OS_VI_PAL_LAN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_LAF1, OS_VI_PAL_LAF1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_LAN2, OS_VI_PAL_LAN2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_LAF2, OS_VI_PAL_LAF2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_HAN1, OS_VI_PAL_HAN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_NTSC_HAF1, OS_VI_PAL_HAF1)==0) {
					patch_counter++;
				}

				vpf(1, "stage 0.2 pal->ntsc\n");

				if(patchMode(OS_VI_PAL_LAN1, OS_VI_NTSC_LAN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_LAF1, OS_VI_NTSC_LAF1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_LAN2, OS_VI_NTSC_LAN2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_LAF2, OS_VI_NTSC_LAF2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_HAN1, OS_VI_NTSC_HAN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_PAL_HAF1, OS_VI_NTSC_HAF1)==0) {
					patch_counter++;
				}

				vpf(1, "stage 0.3 mpal->pal\n");
				if(patchMode(OS_VI_MPAL_LAN1, OS_VI_PAL_LAN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_LAF1, OS_VI_PAL_LAF1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_LAN2, OS_VI_PAL_LAN2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_LAF2, OS_VI_PAL_LAF2)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_HAN1, OS_VI_PAL_HAN1)==0) {
					patch_counter++;
				}
				if(patchMode(OS_VI_MPAL_HAF1, OS_VI_PAL_HAF1)==0) {
					patch_counter++;
				}
			}//end swap only

		}
		vpf(1, "=>> %d/18 modes patched!\n\n", patch_counter);
	}

	if(patch_counter>=1) {
		printf("\nVideo Table:\t\tpatched\n");
	} else {
		printf("\nVideo Table:\t\tuntouched\n");
	}

	u8 patches=patchDitherFilter_Testing();
	if(patches>1) {
		if( gopt( options, 'o' ) ) {

			if( gopt_arg( options, 'o', & filename_output ) && strcmp( filename_output, "-" ) ) {

				if( !gopt( options, 'q' ) ) {
					FILE *fpw;
					fpw=fopen(filename_output, "wb");

					fwrite(rom_blob, 1, fsize, fpw);
					fclose(fpw);
				}

			}
		}
		if( gopt( options, 'q' ) ) {
			printf("result: dummy mode - file patched!\n");
		} else {
			printf("result: file patched!\n");
		}

	} else {
		printf("\n\nresult: file not patched!\n");
	}

	printf("done...\n");
	return 0;
}

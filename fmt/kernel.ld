9300 /* Simple linker script for the JOS kernel.
9301    See the GNU ld 'info' manual ("info ld") to learn the syntax. */
9302 
9303 OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
9304 OUTPUT_ARCH(i386)
9305 ENTRY(_start)
9306 
9307 SECTIONS
9308 {
9309 	/* Link the kernel at this address: "." means the current address */
9310         /* Must be equal to KERNLINK */
9311 	. = 0x80100000;
9312 
9313 	.text : AT(0x100000) {
9314 		*(.text .stub .text.* .gnu.linkonce.t.*)
9315 	}
9316 
9317 	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */
9318 
9319 	.rodata : {
9320 		*(.rodata .rodata.* .gnu.linkonce.r.*)
9321 	}
9322 
9323 	/* Include debugging information in kernel memory */
9324 	.stab : {
9325 		PROVIDE(__STAB_BEGIN__ = .);
9326 		*(.stab);
9327 		PROVIDE(__STAB_END__ = .);
9328 	}
9329 
9330 	.stabstr : {
9331 		PROVIDE(__STABSTR_BEGIN__ = .);
9332 		*(.stabstr);
9333 		PROVIDE(__STABSTR_END__ = .);
9334 	}
9335 
9336 	/* Adjust the address for the data segment to the next page */
9337 	. = ALIGN(0x1000);
9338 
9339 	/* Conventionally, Unix linkers provide pseudo-symbols
9340 	 * etext, edata, and end, at the end of the text, data, and bss.
9341 	 * For the kernel mapping, we need the address at the beginning
9342 	 * of the data section, but that's not one of the conventional
9343 	 * symbols, because the convention started before there was a
9344 	 * read-only rodata section between text and data. */
9345 	PROVIDE(data = .);
9346 
9347 
9348 
9349 
9350 	/* The data segment */
9351 	.data : {
9352 		*(.data)
9353 	}
9354 
9355 	PROVIDE(edata = .);
9356 
9357 	.bss : {
9358 		*(.bss)
9359 	}
9360 
9361 	PROVIDE(end = .);
9362 
9363 	/DISCARD/ : {
9364 		*(.eh_frame .note.GNU-stack)
9365 	}
9366 }
9367 
9368 
9369 
9370 
9371 
9372 
9373 
9374 
9375 
9376 
9377 
9378 
9379 
9380 
9381 
9382 
9383 
9384 
9385 
9386 
9387 
9388 
9389 
9390 
9391 
9392 
9393 
9394 
9395 
9396 
9397 
9398 
9399 

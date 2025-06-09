
OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(acs_entry)

MEMORY {
    RAM (rw): ORIGIN = 0x00000000, LENGTH = 0x00001000
}

MEMORY {
    ROM (rx): ORIGIN = 0x00001000, LENGTH = 0x00000100
}

SECTIONS
{
	. = (0x00001000);
	. = ALIGN(4);
	.entry : {
		__RO_START__ = .;
		*acs_entry.o(.text*)
		*(.text*)
		*(.data*)
		*(.bss*)
    } > ROM

	. = 0x00000000;
	.dev_header :
	{
		*(.dev_header*)
	}

	. = 0x00000080;
	.vendor_key . :
	{
		*(.vendor_key*)
	}

	.generic_param . :
	{
		*(.generic_param*)
	}

	.clk_param . :
	{
		*(.clk_param*)
	}

	.misc_param . :
	{
		*(.misc_param*)
	}

	.store_param . :
	{
		*(.store_param*)
	}

	.ddr_param . :
	{
		*(.ddr_param*)
		*(.ddr_2acs_index*)
		*(.ddr_2acs_data*)
	}
    ASSERT(. <= 0x00001c00, "params image has exceeded its limit.")
}

/*
 * The builtin symbol table for use with kallsyms
 *
 * Copyright (c) 2008-2009 Analog Devices Inc.
 * Licensed under the GPL-2 or later.
 */
#ifdef CONFIG_AMLOGIC_MODIFY
const char system_map[] = SYSTEM_MAP;
#else
const char const system_map[] = SYSTEM_MAP;
#endif

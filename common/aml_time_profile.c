#include <common.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_AMLOGIC_TIME_PROFILE
void record_init_call_time(void *func, unsigned int time)
{
	int i;

	for (i = 0; i < INITCALL_CNT; i++) {
		if (!gd->ict[i].func) {
			gd->ict[i].func = func;
			gd->ict[i].time = time;
			return;
		}
	}
	printf("Lost init call %p, time:%d\n", func, time);
}

void dump_initcall_time(void)
{
	int i;
	const char *sym;
	unsigned long base;
	unsigned long end;

	for (i = 0; i < INITCALL_CNT; i++) {
		if (gd->ict[i].func && gd->ict[i].time > 1000) {
			sym = symbol_lookup((unsigned long)gd->ict[i].func,
					    &base, &end);
			if (sym)
				printf("init call time:%8d, func:%s\n",
					gd->ict[i].time, sym);
			else
				printf("init call time:%8d, func:%p\n",
					gd->ict[i].time, gd->ict[i].func);
		}
	}
}

#endif

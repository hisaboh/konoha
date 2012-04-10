#include "vmtest.h"

static KMETHOD int_opadd(CTX, ksfp_t *sfp, long _rix)
{
    RETURNi_(sfp[1].ivalue + sfp[2].ivalue);
}

int main(int argc, const char *argv[])
{
    kopl_t opbuf[128], *pc = opbuf;
    konoha_t konoha = konoha_open();
    {
        klr_THCODE_t *op = OPCAST(THCODE, pc);
        op->th = thcode;
    }
    {
        /* reg3 = 10; */
        klr_NSET_t *op = OPCAST(NSET, pc);
        op->a = 3;
        op->n = 10;
    }

    {
        /* reg5 = 20; */
        klr_NSET_t *op = OPCAST(NSET, pc);
        op->a = 5;
        op->n = 20;
    }

    {
        /* reg1 = Int.add(10,20); */
        klr_SCALL_t *op = OPCAST(SCALL, pc);
        struct _kMethod mtd;
        mtd.fcall_1 = int_opadd;
        op->uline = 1;
        op->thisidx = 0;
        op->espshift = 0;
        op->mtd = &mtd;
    }
    emit_ret(pc);
    {
        /* exit; */
        klr_EXIT_t *op = OPCAST(EXIT, pc);
        (void)op;
    }

    {
        ksfp_t sfp[10];
        pc = VirtualMachine_run(konoha, sfp, opbuf);
        VirtualMachine_run(konoha, sfp+4, pc);
        fprintf(stderr, "%ld\n", sfp[0].ivalue);
        assert(sfp[0].ivalue == 30);
    }
    konoha_close(konoha);
    MODGC_check_malloced_size();
    return 0;
}

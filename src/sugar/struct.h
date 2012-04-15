/****************************************************************************
 * KONOHA2 COPYRIGHT, LICENSE NOTICE, AND DISCRIMER
 *
 * Copyright (c) 2006-2012, Kimio Kuramitsu <kimio at ynu.ac.jp>
 *           (c) 2008-      Konoha Team konohaken@googlegroups.com
 * All rights reserved.
 * You may choose one of the following two licelanges when you use konoha.
 * If you want to use the latter licelange, please contact us.
 *
 * (1) GNU General Public Licelange 3.0 (with K_UNDER_GPL)
 * (2) Konoha Non-Disclosure Licelange 1.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#define PACKSUGAR    .packid = 1, .packdom = 1

/* --------------- */
/* KonohaSpace */

static void KonohaSpace_init(CTX, kObject *o, void *conf)
{
	struct _kKonohaSpace *ks = (struct _kKonohaSpace*)o;
	bzero(&ks->parentNULL, sizeof(kKonohaSpace) - sizeof(kObjectHeader));
	ks->parentNULL = conf;
	ks->static_cid = TY_unknown;
	ks->function_cid = TY_System;
}

static void syntax_reftrace(CTX, kmape_t *p)
{
	ksyntax_t *syn = (ksyntax_t*)p->uvalue;
	BEGIN_REFTRACE(5);
	KREFTRACEn(syn->syntaxRule);
	KREFTRACEn(syn->ParseStmt);
	KREFTRACEn(syn->TopStmtTyCheck);
	KREFTRACEn(syn->StmtTyCheck);
	KREFTRACEn(syn->ExprTyCheck);
	END_REFTRACE();
}

static void KonohaSpace_reftrace(CTX, kObject *o)
{
	kKonohaSpace *ks = (kKonohaSpace*)o;
	if(ks->syntaxMapNN != NULL) {
		kmap_reftrace(ks->syntaxMapNN, syntax_reftrace);
	}
	size_t i, size = KARRAYSIZE(ks->cl.bytesize, kvs);
	BEGIN_REFTRACE(size);
	for(i = 0; i < size; i++) {
		if(FN_isBOXED(ks->cl.kvs[i].key)) {
			KREFTRACEv(ks->cl.kvs[i].oval);
		}
	}
	KREFTRACEn(ks->parentNULL);
	KREFTRACEn(ks->script);
	KREFTRACEn(ks->methodsNULL);
	END_REFTRACE();
}

static void syntax_free(CTX, void *p)
{
	KNH_FREE(p, sizeof(ksyntax_t));
}

static void KonohaSpace_free(CTX, kObject *o)
{
	struct _kKonohaSpace *ks = (struct _kKonohaSpace*)o;
	if(ks->syntaxMapNN != NULL) {
		kmap_free(ks->syntaxMapNN, syntax_free);
	}
	KARRAY_FREE(&ks->cl);
}

static KDEFINE_CLASS KonohaSpaceDef = {
	STRUCTNAME(KonohaSpace), PACKSUGAR,
	.init = KonohaSpace_init,
	.reftrace = KonohaSpace_reftrace,
	.free = KonohaSpace_free,
};

static int comprKeyVal(const void *a, const void *b)
{
	int akey = FN_UNBOX(((kvs_t*)a)->key);
	int bkey = FN_UNBOX(((kvs_t*)b)->key);
	return akey - bkey;
}

static kvs_t* KonohaSpace_getConstNULL(CTX, kKonohaSpace *ks, ksymbol_t ukey)
{
	size_t min = 0, max = KARRAYSIZE(ks->cl.bytesize, kvs);
	while(min < max) {
		size_t p = (max + min) / 2;
		ksymbol_t key = FN_UNBOX(ks->cl.kvs[p].key);
		if(key == ukey) return ks->cl.kvs + p;
		if(key < ukey) {
			min = p + 1;
		}
		else {
			max = p;
		}
	}
	return NULL;
}

static kbool_t checkConflictedConst(CTX, kKonohaSpace *ks, kvs_t *kvs, kline_t pline)
{
	ksymbol_t ukey = FN_UNBOX(kvs->key);
	kvs_t* ksval = KonohaSpace_getConstNULL(_ctx, ks, ukey);
	if(ksval != NULL) {
		if(kvs->ty == ksval->ty && kvs->uval == ksval->uval) {
			return true;  // same value
		}
		kreportf(WARN_, pline, "conflict name: %s", T_UN(ukey));
		return true;
	}
	return false;
}

static void KonohaSpace_mergeConstData(CTX, struct _kKonohaSpace *ks, kvs_t *kvs, size_t nitems, kline_t pline)
{
	size_t i, s = KARRAYSIZE(ks->cl.bytesize, kvs);
	if(s == 0) {
		KARRAY_INIT(&ks->cl, (nitems + 8) * sizeof(kvs_t));
		memcpy(ks->cl.kvs, kvs, nitems * sizeof(kvs_t));
	}
	else {
		kwb_t wb;
		kwb_init(&(ctxsugar->cwb), &wb);
		for(i = 0; i < nitems; i++) {
			if(kvs[i].ty == TY_TYPE) continue;  // class table
			if(checkConflictedConst(_ctx, ks, kvs+i, pline)) continue;
			kwb_write(&wb, (const char*)(kvs+i), sizeof(kvs_t));
		}
		kvs = (kvs_t*)kwb_top(&wb, 0);
		nitems = kwb_bytesize(&wb)/sizeof(kvs_t);
		if(nitems > 0) {
			KARRAY_RESIZE(&ks->cl, (s + nitems + 8) * sizeof(kvs_t));
			memcpy(ks->cl.kvs + s, kvs, nitems * sizeof(kvs_t));
		}
		kwb_free(&wb);
	}
	ks->cl.bytesize = (s + nitems) * sizeof(kvs_t);
	qsort(ks->cl.kvs, s + nitems, sizeof(kvs_t), comprKeyVal);
}

static void KonohaSpace_importClassName(CTX, kKonohaSpace *ks, kpack_t packdom, kline_t pline)
{
	kvs_t kv;
	kwb_t wb;
	kwb_init(&(_ctx->stack->cwb), &wb);
	size_t i, size = KARRAYSIZE(_ctx->share->ca.bytesize, uintptr);
	for(i = 0; i < size; i++) {
		kclass_t *ct = CT_(i);
		if(ct->packdom == packdom) {
			kv.key = ct->nameid;
			kv.ty  = TY_TYPE;
			kv.uval = (uintptr_t)ct;
		}
		kwb_write(&wb, (const char*)(&kv), sizeof(kvs_t));
	}
	size_t nitems = kwb_bytesize(&wb) / sizeof(kvs_t);
	if(nitems > 0) {
		KonohaSpace_mergeConstData(_ctx, (struct _kKonohaSpace*)ks, (kvs_t*)kwb_top(&wb, 0), nitems, pline);
	}
	kwb_free(&wb);
}

static void KonohaSpace_loadConstData(CTX, kKonohaSpace *ks, const char **d, kline_t pline)
{
	INIT_GCSTACK();
	kvs_t kv;
	kwb_t wb;
	kwb_init(&(_ctx->stack->cwb), &wb);
	while(d[0] != NULL) {
		DBG_P("key='%s'", d[0]);
		kv.key = kuname(d[0], strlen(d[0]), SPOL_TEXT|SPOL_ASCII, _NEWID) | FN_BOXED;
		kv.ty  = (ktype_t)(uintptr_t)d[1];
		if(kv.ty == TY_TEXT) {
			kv.ty = TY_String;
			kv.sval = new_kString(d[2], strlen(d[2]), SPOL_TEXT);
			PUSH_GCSTACK(kv.oval);
		}
		else if(TY_isUnbox(kv.ty)) {
			kv.key = FN_UNBOX(kv.key);
			kv.uval = (uintptr_t)d[2];
		}
		else {
			kv.oval = (kObject*)d[2];
		}
		kwb_write(&wb, (const char*)(&kv), sizeof(kvs_t));
		d += 3;
	}
	size_t nitems = kwb_bytesize(&wb) / sizeof(kvs_t);
	if(nitems > 0) {
		KonohaSpace_mergeConstData(_ctx, (struct _kKonohaSpace*)ks, (kvs_t*)kwb_top(&wb, 0), nitems, pline);
	}
	kwb_free(&wb);
	RESET_GCSTACK();
}

static ksyntax_t* KonohaSpace_syntax(CTX, kKonohaSpace *ks0, keyword_t kw, int isnew)
{
	kKonohaSpace *ks = ks0;
	uintptr_t hcode = kw;
	ksyntax_t *parent = NULL;
	while(ks != NULL) {
		if(ks->syntaxMapNN != NULL) {
			kmape_t *e = kmap_get(ks->syntaxMapNN, hcode);
			while(e != NULL) {
				if(e->hcode == hcode) {
					parent = (ksyntax_t*)e->uvalue;
					if(isnew && ks0 != ks) goto L_NEW;
					return parent;
				}
				e = e->next;
			}
		}
		ks = ks->parentNULL;
	}
	L_NEW:;
	if(isnew == 1) {
		if(ks0->syntaxMapNN == NULL) {
			((struct _kKonohaSpace*)ks0)->syntaxMapNN = kmap_init(0);
		}
		kmape_t *e = kmap_newentry(ks0->syntaxMapNN, hcode);
		kmap_add(ks0->syntaxMapNN, e);
		struct _ksyntax *syn = (struct _ksyntax*)KNH_ZMALLOC(sizeof(ksyntax_t));
		e->uvalue = (uintptr_t)syn;
		if(parent != NULL) {  // TODO: RCGC
			memcpy(syn, parent, sizeof(ksyntax_t));
		}
		else {
			syn->kw = kw;
			syn->ty  = TY_unknown;
			syn->op1 = 0;
			syn->op2 = 0;
		}
		//syn->parent = parent;
		return syn;
	}
	return NULL;
}

static ksymbol_t keyword(CTX, const char *name, size_t len, ksymbol_t def);
static void parseSyntaxRule(CTX, const char *rule, kline_t pline, kArray *a);

static void setSyntaxMethod(CTX, knh_Fmethod f, kMethod **synp, knh_Fmethod *p, kMethod **mp)
{
	if(f != NULL) {
		if(f != p[0]) {
			p[0] = f;
			mp[0] = new_kMethod(0, 0, 0, NULL, f);
		}
		KINITv(synp[0], mp[0]);
	}
}

static void KonohaSpace_defineSyntax(CTX, kKonohaSpace *ks, KDEFINE_SYNTAX *syndef)
{
	knh_Fmethod pParseStmt = NULL, pParseExpr = NULL, pStmtTyCheck = NULL, pExprTyCheck = NULL;
	kMethod *mParseStmt = NULL, *mParseExpr = NULL, *mStmtTyCheck = NULL, *mExprTyCheck = NULL;
	while(syndef->name != NULL) {
		keyword_t kw = keyword(_ctx, syndef->name, strlen(syndef->name), FN_NEWID);
		struct _ksyntax* syn = (struct _ksyntax*)KonohaSpace_syntax(_ctx, ks, kw, 1);
		syn->token = syndef->name;
		syn->flag  |= syndef->flag;
		if(syndef->type != 0) {
			syn->ty = syndef->type;
		}
		if(syndef->rule != NULL) {
			KINITv(syn->syntaxRule, new_(Array, 0));
			parseSyntaxRule(_ctx, syndef->rule, 0, syn->syntaxRule);
		}
		if(syndef->op1 != NULL) {
			syn->op1 = (syndef->op1[0] == '*') ? MN_NONAME : ksymbol(syndef->op1, 127, FN_NEWID, SYMPOL_METHOD);
		}
		if(syndef->op2 != NULL) {
			syn->op2 = (syndef->op2[0] == '*') ? MN_NONAME : ksymbol(syndef->op2, 127, FN_NEWID, SYMPOL_METHOD);
			syn->priority = syndef->priority_op2;
			syn->right = syndef->right;
		}
		setSyntaxMethod(_ctx, syndef->ParseStmt, &(syn->ParseStmt), &pParseStmt, &mParseStmt);
		setSyntaxMethod(_ctx, syndef->ParseExpr, &(syn->ParseExpr), &pParseExpr, &mParseExpr);
		setSyntaxMethod(_ctx, syndef->TopStmtTyCheck, &(syn->TopStmtTyCheck), &pStmtTyCheck, &mStmtTyCheck);
		setSyntaxMethod(_ctx, syndef->StmtTyCheck, &(syn->StmtTyCheck), &pStmtTyCheck, &mStmtTyCheck);
		setSyntaxMethod(_ctx, syndef->ExprTyCheck, &(syn->ExprTyCheck), &pExprTyCheck, &mExprTyCheck);
		DBG_ASSERT(syn == SYN_(ks, kw));
		syndef++;
	}
	DBG_P("syntax size=%d, hmax=%d", ks->syntaxMapNN->size, ks->syntaxMapNN->hmax);
}

// KonohaSpace

static kcid_t KonohaSpace_getcid(CTX, kKonohaSpace *ks, const char *name, size_t len, kcid_t def)
{
	uintptr_t hcode = longid(PN_konoha, kuname(name, len, 0, FN_NONAME));
	kclass_t *ct = (kclass_t*)map_getu(_ctx, _ctx->share->lcnameMapNN, hcode, 0);
	return (ct != NULL) ? ct->cid : def;
}

static void KonohaSpace_addMethod(CTX, kKonohaSpace *ks, kMethod *mtd)
{
	if(ks->methodsNULL == NULL) {
		KINITv(((struct _kKonohaSpace*)ks)->methodsNULL, new_(Array, 8));
	}
	kArray_add(ks->methodsNULL, mtd);

}

/* KonohaSpace/Class/Method */
static kMethod* CT_findMethodNULL(CTX, kclass_t *ct, kmethodn_t mn)
{
	kclass_t *p, *t0 = ct;
	do {
		size_t i;
		kArray *a = t0->methods;
		for(i = 0; i < kArray_size(a); i++) {
			kMethod *mtd = a->methods[i];
			if((mtd)->mn == mn) {
				return mtd;
			}
		}
		p = t0;
		t0 = CT_(t0->supcid);
	}
	while(p != t0);
	return NULL;
}

#define kKonohaSpace_getMethodNULL(ns, cid, mn)     KonohaSpace_getMethodNULL(_ctx, ns, cid, mn)
#define kKonohaSpace_getStaticMethodNULL(ns, mn)   KonohaSpace_getStaticMethodNULL(_ctx, ns, mn)
static kMethod* KonohaSpace_getMethodNULL(CTX, kKonohaSpace *ks, kcid_t cid, kmethodn_t mn)
{
	while(ks != NULL) {
		if(ks->methodsNULL != NULL) {
			size_t i;
			kArray *methods = ks->methodsNULL;
			for(i = 0; i < kArray_size(methods); i++) {
				kMethod *mtd = methods->methods[i];
				if(mtd->cid == cid && mtd->mn == mn) {
					return mtd;
				}
			}
		}
		ks = ks->parentNULL;
	}
	return CT_findMethodNULL(_ctx, CT_(cid), mn);
}

static kMethod* KonohaSpace_getStaticMethodNULL(CTX, kKonohaSpace *ks, kmethodn_t mn)
{
	while(ks != NULL) {
		if(ks->static_cid != TY_unknown) {
			kMethod *mtd = kKonohaSpace_getMethodNULL(ks, ks->static_cid, mn);
			if(mtd != NULL && kMethod_isStatic(mtd)) {
				return mtd;
			}
		}
		ks = ks->parentNULL;
	}
	return NULL;
}

#define kKonohaSpace_getCastMethodNULL(ns, cid, tcid)     KonohaSpace_getCastMethodNULL(_ctx, ns, cid, tcid)
static kMethod* KonohaSpace_getCastMethodNULL(CTX, kKonohaSpace *ks, kcid_t cid, kcid_t tcid)
{
	kMethod *mtd = KonohaSpace_getMethodNULL(_ctx, ks, cid, MN_to(tcid));
	if(mtd == NULL) {
		mtd = KonohaSpace_getMethodNULL(_ctx, ks, cid, MN_as(tcid));
	}
	return mtd;
}

#define kKonohaSpace_defineMethod(NS,MTD,UL)  KonohaSpace_defineMethod(_ctx, NS, MTD, UL)

static kbool_t KonohaSpace_defineMethod(CTX, kKonohaSpace *ks, kMethod *mtd, kline_t pline)
{
	if(pline != 0) {
		kMethod *mtdOLD = KonohaSpace_getMethodNULL(_ctx, ks, mtd->cid, mtd->mn);
		if(mtdOLD != NULL) {
			char mbuf[128];
			kreportf(ERR_, pline, "method %s.%s is already defined", T_cid(mtd->cid), T_mn(mbuf, mtd->mn));
			return 0;
		}
	}
	if(mtd->packid == 0) {
		((struct _kMethod*)mtd)->packid = ks->packid;
	}
	kclass_t *ct = CT_(mtd->cid);
	if(ct->packdom == ks->packdom && kMethod_isPublic(mtd)) {
		kArray_add(ct->methods, mtd);
	}
	else {
		KonohaSpace_addMethod(_ctx, ks, mtd);
	}
	return 1;
}

static void KonohaSpace_loadMethodData(CTX, kKonohaSpace *ks, intptr_t *data)
{
	intptr_t *d = data;
	kParam *prev = NULL;
	while(d[0] != -1) {
		uintptr_t flag = (uintptr_t)d[0];
		knh_Fmethod f = (knh_Fmethod)d[1];
		ktype_t rtype = (ktype_t)d[2];
		kcid_t cid  = (kcid_t)d[3];
		kmethodn_t mn = (kmethodn_t)d[4];
		size_t i, psize = (size_t)d[5];
		kparam_t p[psize+1];
		d = d + 6;
		for(i = 0; i < psize; i++) {
			p[i].ty = (ktype_t)d[0];
			p[i].fn = (ksymbol_t)d[1];
			d += 2;
		}
		if(prev != NULL) {
			if (prev->rtype == rtype && prev->psize == psize) {
				for(i = 0; i < psize; i++) {
					if(p[i].ty != prev->p[i].ty || p[i].fn != prev->p[i].fn) {
						prev = NULL;
						break;
					}
				}
			}
			else prev = NULL;
		}
		kParam *pa = (prev == NULL) ? new_kParam(rtype, psize, p) : prev;
		kMethod *mtd = new_kMethod(flag, cid, mn, pa, f);
		if(ks == NULL || kMethod_isPublic(mtd)) {
			kArray_add(CT_(cid)->methods, mtd);
		} else {
			KonohaSpace_addMethod(_ctx, ks, mtd);
		}
		prev = pa;
	}
}

//#define kKonohaSpace_loadGlueFunc(NS, F, OPT, UL)  KonohaSpace_loadGlueFunc(_ctx, NS, F, OPT, UL)
//
//static knh_Fmethod KonohaSpace_loadGlueFunc(CTX, kKonohaSpace *ks, const char *funcname, int DOPTION, kline_t pline)
//{
//	void *f = NULL;
//	if(ks->gluehdr != NULL) {
//		char namebuf[128];
//		snprintf(namebuf, sizeof(namebuf), "D%s", funcname);
//		if(DOPTION) {
//			f = dlsym(ks->gluehdr, (const char*)namebuf);
//		}
//		if(f == NULL) {
//			f = dlsym(ks->gluehdr, (const char*)namebuf+1);
//		}
//		kreportf(WARN_, pline, "glue method function is not found: %s", namebuf + 1);
//	}
//	return f;
//}

/* --------------- */
/* Token */

static void Token_init(CTX, kObject *o, void *conf)
{
	struct _kToken *tk = (struct _kToken*)o;
	tk->uline     =   0;
	tk->tt        =   (ktoken_t)conf;
	tk->kw        =   0;
	tk->topch     =   0;
	tk->lpos      =   -1;
	KINITv(tk->text, TS_EMPTY);
}

static void Token_reftrace(CTX, kObject *o)
{
	kToken *tk = (kToken*)o;
	BEGIN_REFTRACE(1);
	KREFTRACEv(tk->text);
	END_REFTRACE();
}

static KDEFINE_CLASS TokenDef = {
	STRUCTNAME(Token), PACKSUGAR,
	.init = Token_init,
	.reftrace = Token_reftrace,
};

#define kToken_s(tk) kToken_s_(_ctx, tk)
static const char *kToken_s_(CTX, kToken *tk)
{
	switch((int)tk->tt) {
	case AST_PARENTHESIS: return "(... )";
	case AST_BRACE: return "{... }";
	case AST_BRANCET: return "[... ]";
	default:  return S_text(tk->text);
	}
}

static const char *T_tt(ktoken_t t)
{
	static const char* symTKDATA[] = {
		"TK_NONE",
		"TK_INDENT",
		"TK_SYMBOL",
		"TK_USYMBOL",
		"TK_TEXT",
		"TK_STEXT",
		"TK_BTEXT",
		"TK_INT",
		"TK_FLOAT",
		"TK_URN",
		"TK_REGEX",
		"TK_TYPE",
		"AST_()",
		"AST_[]",
		"AST_{}",

		"TK_OPERATOR",
		"TK_CODE",
		"TK_WHITESPACE",
		"TK_METANAME",
		"TK_MN",
		"AST_OPTIONAL[]",
	};
	if(t <= AST_OPTIONAL) {
		return symTKDATA[t];
	}
	return "TK_UNKNOWN";
}

static void dumpToken(CTX, kToken *tk)
{
	if(verbose_sugar) {
		if(tk->tt == TK_MN) {
			char mbuf[128];
			DUMP_P("%s %d+%d: %s(%s)\n", T_tt(tk->tt), (short)tk->uline, tk->lpos, T_mn(mbuf, tk->mn), kToken_s(tk));
		}
		else {
			DUMP_P("%s %d+%d: kw=%s '%s'\n", T_tt(tk->tt), (short)tk->uline, tk->lpos, T_kw(tk->kw), kToken_s(tk));
		}
	}
}

static void dumpIndent(int nest)
{
	if(verbose_sugar) {
		int i;
		for(i = 0; i < nest; i++) {
			DUMP_P("  ");
		}
	}
}

static void dumpTokenArray(CTX, int nest, kArray *a, int s, int e)
{
	if(verbose_sugar) {
		if(nest == 0) DUMP_P("\n");
		while(s < e) {
			kToken *tk = a->toks[s];
			dumpIndent(nest);
			if(IS_Array(tk->sub)) {
				DUMP_P("%c\n", tk->topch);
				dumpTokenArray(_ctx, nest+1, tk->sub, 0, kArray_size(tk->sub));
				dumpIndent(nest);
				DUMP_P("%c\n", tk->closech);
			}
			else {
				DUMP_P("TK(%d) ", s);
				dumpToken(_ctx, tk);
			}
			s++;
		}
		if(nest == 0) DUMP_P("====\n");
	}
}

/* --------------- */
/* Expr */

static void Expr_init(CTX, kObject *o, void *conf)
{
	struct _kExpr *expr      =   (struct _kExpr*)o;
	expr->build      =   TEXPR_UNTYPED;
	expr->ty         =   TY_var;
	KINITv(expr->tk, K_NULLTOKEN);
	KINITv(expr->data, K_NULL);
	expr->syn = (ksyntax_t*)conf;
}

static void Expr_reftrace(CTX, kObject *o)
{
	kExpr *expr = (kExpr*)o;
	BEGIN_REFTRACE(2);
	KREFTRACEv(expr->tk);
	KREFTRACEv(expr->data);
	END_REFTRACE();
}

static KDEFINE_CLASS ExprDef = {
	STRUCTNAME(Expr), PACKSUGAR,
	.init = Expr_init,
	.reftrace = Expr_reftrace,
};

static struct _kExpr* Expr_vadd(CTX, struct _kExpr *expr, int n, va_list ap)
{
	int i;
	KSETv(expr->cons, new_(Array, 8));
	for(i = 0; i < n; i++) {
		kObject *v =  (kObject*)va_arg(ap, kObject*);
		if(v == NULL || v == (kObject*)K_NULLEXPR) return (struct _kExpr*)K_NULLEXPR;
		kArray_add(expr->cons, v);
	}
	return expr;
}

static kExpr* new_ConsExpr(CTX, ksyntax_t *syn, int n, ...)
{
	va_list ap;
	va_start(ap, n);
	DBG_ASSERT(syn != NULL);
	struct _kExpr *expr = new_W(Expr, syn);
	PUSH_GCSTACK(expr);
	expr = Expr_vadd(_ctx, expr, n, ap);
	va_end(ap);
	return (kExpr*)expr;
}

static kExpr* new_TypedConsExpr(CTX, int build, ktype_t ty, int n, ...)
{
	va_list ap;
	va_start(ap, n);
	struct _kExpr *expr = new_W(Expr, NULL);
	PUSH_GCSTACK(expr);
	expr = Expr_vadd(_ctx, expr, n, ap);
	va_end(ap);
	expr->build = build;
	expr->ty = ty;
	return (kExpr*)expr;
}

static kExpr* Expr_add(CTX, kExpr *expr, kExpr *e)
{
	DBG_ASSERT(IS_Array(expr->cons));
	if(expr != K_NULLEXPR && e != NULL && e != K_NULLEXPR) {
		kArray_add(expr->cons, e);
		return expr;
	}
	return K_NULLEXPR;
}

static void dumpExpr(CTX, int n, int nest, kExpr *expr)
{
	if(verbose_sugar) {
		if(nest == 0) DUMP_P("\n");
		dumpIndent(nest);
		if(Expr_isTerm(expr)) {
			DUMP_P("[%d] ExprTerm: kw=%s %s", n, T_kw(expr->tk->kw), kToken_s(expr->tk));
			if(expr->ty != TY_var) {

			}
			DUMP_P("\n");
		}
		else {
			int i;
			DUMP_P("[%d] Cons: kw=%s, size=%ld", n, T_kw(expr->syn->kw), kArray_size(expr->cons));
			if(expr->ty != TY_var) {

			}
			DUMP_P("\n");
			for(i=0; i < kArray_size(expr->cons); i++) {
				kObject *o = expr->cons->list[i];
				if(O_ct(o) == CT_Expr) {
					dumpExpr(_ctx, i, nest+1, (kExpr*)o);
				}
				else {
					dumpIndent(nest+1);
					if(O_ct(o) == CT_Token) {
						kToken *tk = (kToken*)o;
						DUMP_P("[%d] O: %s ", i, T_CT(o->h.ct));
						dumpToken(_ctx, tk);
					}
					else if(o == K_NULL) {
						DUMP_P("[%d] O: null\n", i);
					}
					else {
						DUMP_P("[%d] O: %s\n", i, T_CT(o->h.ct));
					}
				}
			}
		}
	}
}

static kExpr* Expr_setConstValue(CTX, kExpr *expr, ktype_t ty, kObject *o)
{
	if(expr == NULL) expr = new_(Expr, 0);
	W(kExpr, expr);
	Wexpr->ty = ty;
	if(TY_isUnbox(ty)) {
		Wexpr->build = TEXPR_NCONST;
		Wexpr->ndata = N_toint(o);
		KSETv(Wexpr->data, K_NULL);
	}
	else {
		Wexpr->build = TEXPR_CONST;
		KINITv(Wexpr->data, o);
	}
	WASSERT(expr);
	return expr;
}

static kExpr* Expr_setNConstValue(CTX, kExpr *expr, ktype_t ty, uintptr_t ndata)
{
	if(expr == NULL) expr = new_(Expr, 0);
	W(kExpr, expr);
	Wexpr->build = TEXPR_NCONST;
	Wexpr->ndata = ndata;
	KSETv(Wexpr->data, K_NULL);
	Wexpr->ty = ty;
	WASSERT(expr);
	return expr;
}

static kExpr *Expr_setVariable(CTX, kExpr *expr, int build, ktype_t ty, intptr_t index, kGamma *gma)
{
	if(expr == NULL) expr = new_W(Expr, 0);
	W(kExpr, expr);
	Wexpr->build = build;
	Wexpr->ty = ty;
	Wexpr->index = index;
	KSETv(Wexpr->data, K_NULL);
	if(build < TEXPR_UNTYPED) {
		kArray_add(gma->genv->lvarlst, Wexpr);
	}
	WASSERT(expr);
	return expr;
}

/* --------------- */
/* Stmt */

static void Stmt_init(CTX, kObject *o, void *conf)
{
	struct _kStmt *stmt = (struct _kStmt*)o;
	stmt->uline      =   (kline_t)conf;
	stmt->syn = NULL;
	stmt->parentNULL = NULL;
}

static void Stmt_reftrace(CTX, kObject *o)
{
	kStmt *stmt = (kStmt*)o;
	BEGIN_REFTRACE(1);
	KREFTRACEn(stmt->parentNULL);
	END_REFTRACE();
}

static KDEFINE_CLASS StmtDef = {
	STRUCTNAME(Stmt), PACKSUGAR,
	.init = Stmt_init,
	.reftrace = Stmt_reftrace,
};

static void _dumpToken(CTX, void *arg, kvs_t *d)
{
	if((d->key & FN_BOXED) == FN_BOXED) {
		keyword_t key = ~FN_BOXED & d->key;
		DUMP_P("key='%s': ", T_kw(key));
		if(IS_Token(d->oval)) {
			dumpToken(_ctx, (kToken*)d->oval);
		} else if (IS_Expr(d->oval)) {
			dumpExpr(_ctx, 0, 0, (kExpr *) d->oval);
		}
	}
}

static void dumpStmt(CTX, kStmt *stmt)
{
	if(verbose_sugar) {
		if(stmt->syn == NULL) {
			DUMP_P("STMT (DONE)\n");
		}
		else {
			DUMP_P("STMT %s {\n", stmt->syn->token);
			kObject_protoEach(stmt, NULL, _dumpToken);
			DUMP_P("\n}\n");
		}
		fflush(stdout);
	}
}

#define kStmt_ks(STMT)   Stmt_ks(_ctx, STMT)
static inline kKonohaSpace *Stmt_ks(CTX, kStmt *stmt)
{
	return stmt->parentNULL->ks;
}

#define kStmt_typed(STMT, T)  Stmt_typed(STMT, TSTMT_##T)
static inline void Stmt_typed(kStmt *stmt, int build)
{
	((struct _kStmt*)stmt)->build = build;
}

#define kStmt_setsyn(STMT, S)  Stmt_setsyn(_ctx, STMT, S)
#define kStmt_done(STMT)       Stmt_setsyn(_ctx, STMT, NULL)
static void Stmt_setsyn(CTX, kStmt *stmt, ksyntax_t *syn)
{
	if(syn == NULL && stmt->syn != NULL) {
		DBG_P("DONE: STMT='%s'", stmt->syn->token);
	}
	((struct _kStmt*)stmt)->syn = syn;
}

#define kStmt_toERR(STMT, ENO)  Stmt_toERR(_ctx, STMT, ENO)
static void Stmt_toERR(CTX, kStmt *stmt, int eno)
{
	((struct _kStmt*)stmt)->syn = SYN_ERR;
	((struct _kStmt*)stmt)->build = TSTMT_ERR;
	kObject_setObject(stmt, KW_ERR, kstrerror(eno));
}

#define AKEY(T)   T, (sizeof(T)-1)

typedef struct flagop_t {
	const char *key;
	size_t keysize;
	uintptr_t flag;
} flagop_t ;

static uintptr_t Stmt_flag(CTX, kStmt *stmt, flagop_t *fop, uintptr_t flag)
{
	while(fop->key != NULL) {
		keyword_t kw = keyword(_ctx, fop->key, fop->keysize, FN_NONAME);
		if(kw != FN_NONAME) {
			kObject *op = kObject_getObjectNULL(stmt, kw);
			if(op != NULL) {
				DBG_P("found %s", fop->key);
				flag |= fop->flag;
			}
		}
		fop++;
	}
	return flag;
}

#define kStmt_is(STMT, KW) Stmt_is(_ctx, STMT, KW)

static inline kbool_t Stmt_is(CTX, kStmt *stmt, keyword_t kw)
{
	return (kObject_getObjectNULL(stmt, kw) != NULL);
}

static kToken* Stmt_token(CTX, kStmt *stmt, keyword_t kw, kToken *def)
{
	kToken *tk = (kToken*)kObject_getObjectNULL(stmt, kw);
	if(tk != NULL && IS_Token(tk)) {
		return tk;
	}
	return def;
}

static kExpr* Stmt_expr(CTX, kStmt *stmt, keyword_t kw, kExpr *def)
{
	kExpr *expr = (kExpr*)kObject_getObjectNULL(stmt, kw);
	if(expr != NULL && IS_Expr(expr)) {
		return expr;
	}
	return def;
}

static const char* Stmt_text(CTX, kStmt *stmt, keyword_t kw, const char *def)
{
	kExpr *expr = (kExpr*)kObject_getObjectNULL(stmt, kw);
	if(expr != NULL) {
		if(IS_Expr(expr) && Expr_isTerm(expr)) {
			return S_text(expr->tk->text);
		}
		else if(IS_Token(expr)) {
			kToken *tk = (kToken*)expr;
			if(IS_String(tk->text)) return S_text(tk->text);
		}
	}
	return def;
}

static kbool_t Token_toBRACE(CTX, struct _kToken *tk);
static kBlock *new_Block(CTX, kArray *tls, int s, int e, kKonohaSpace* ks);
static kBlock* Stmt_block(CTX, kStmt *stmt, keyword_t kw, kBlock *def)
{
	kBlock *bk = (kBlock*)kObject_getObjectNULL(stmt, kw);
	if(bk != NULL) {
		if(IS_Token(bk)) {
			kToken *tk = (kToken*)bk;
			if (tk->tt == TK_CODE) {
				Token_toBRACE(_ctx, (struct _kToken*)tk);
			}
			if (tk->tt == AST_BRACE) {
				bk = new_Block(_ctx, tk->sub, 0, kArray_size(tk->sub), kStmt_ks(stmt));
				kObject_setObject(stmt, kw, bk);
			}
		}
		if(IS_Block(bk)) return bk;
	}
	return def;
}

/* --------------- */
/* Block */

static void Block_init(CTX, kObject *o, void *conf)
{
	struct _kBlock *bk = (struct _kBlock*)o;
	kKonohaSpace *ks = (conf != NULL) ? (kKonohaSpace*)conf : kmodsugar->rootks;
	bk->parentNULL = NULL;
	KINITv(bk->ks, ks);
	KINITv(bk->blockS, new_(Array, 0));
	KINITv(bk->esp, new_(Expr, 0));
}

static void Block_reftrace(CTX, kObject *o)
{
	kBlock *bk = (kBlock*)o;
	BEGIN_REFTRACE(4);
	KREFTRACEv(bk->ks);
	KREFTRACEv(bk->blockS);
	KREFTRACEv(bk->esp);
	KREFTRACEn(bk->parentNULL);
	END_REFTRACE();
}

static KDEFINE_CLASS BlockDef = {
	STRUCTNAME(Block), PACKSUGAR,
	.init = Block_init,
	.reftrace = Block_reftrace,
};

static void Block_insertAfter(CTX, kBlock *bk, kStmt *target, kStmt *stmt)
{
	DBG_ASSERT(stmt->parentNULL == NULL);
	KSETv(((struct _kStmt*)stmt)->parentNULL, bk);
	size_t i;
	for(i = 0; i < kArray_size(bk->blockS); i++) {
		if(bk->blockS->stmts[i] == target) {
			kArray_insert(bk->blockS, i+1, stmt);
			return;
		}
	}
	DBG_ABORT("target was not found!!");
}

/* --------------- */
/* Block */

static void Gamma_init(CTX, kObject *o, void *conf)
{
	struct _kGamma *gma = (struct _kGamma*)o;
	gma->genv = NULL;
}

static KDEFINE_CLASS GammaDef = {
	STRUCTNAME(Gamma), PACKSUGAR,
	.init = Gamma_init,
};

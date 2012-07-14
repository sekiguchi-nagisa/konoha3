/****************************************************************************
 * Copyright (c) 2012, the Konoha project authors. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */

static KMETHOD UndefinedExprTyCheck(KonohaContext *kctx, KonohaStack *sfp)
{
	VAR_ExprTyCheck(stmt, expr, gma, reqty);
	if(Expr_isTerm(expr)) {
		expr = kToken_p(stmt, expr->termToken, ErrTag, "undefined token type checker: '%s'", kToken_s(expr->termToken));
	}
	else {
		expr = kStmt_p(stmt, ErrTag, "undefined operator type checker: %s%s",  KW_t(expr->syn->keyword));
	}
	RETURN_(expr);
}

static kExpr *ExprTyCheckFunc(KonohaContext *kctx, kFunc *fo, kStmt *stmt, kExpr *expr, kGamma *gma, int reqty)
{
	INIT_GCSTACK();
	BEGIN_LOCAL(lsfp, K_CALLDELTA + 5);
	KSETv(lsfp[K_CALLDELTA+0].o, fo->self);
	KSETv(lsfp[K_CALLDELTA+1].o, (kObject*)stmt);
	KSETv(lsfp[K_CALLDELTA+2].o, (kObject*)expr);
	KSETv(lsfp[K_CALLDELTA+3].o, (kObject*)gma);
	lsfp[K_CALLDELTA+4].ivalue = reqty;
	KCALL(lsfp, 0, fo->mtd, 5, K_NULLEXPR);
	END_LOCAL();
	RESET_GCSTACK();
	DBG_ASSERT(IS_Expr(lsfp[0].toObject));
	return (kExpr*)lsfp[0].toObject;
}

static kExpr *ExprTyCheck(KonohaContext *kctx, kStmt *stmt, kExpr *expr, kGamma *gma, int reqty)
{
	kFunc *fo = expr->syn->ExprTyCheck;
	kExpr *texpr;
	if(IS_Array(fo)) {
		int i;
		kArray *a = (kArray*)fo;
		for(i = kArray_size(a) - 1; i > 0; i--) {
			texpr = ExprTyCheckFunc(kctx, a->funcItems[i], stmt, expr, gma, reqty);
			if(kStmt_isERR(stmt)) return K_NULLEXPR;
			if(texpr->ty != TY_var) return texpr;
		}
		fo = a->funcItems[0];
	}
	DBG_ASSERT(IS_Func(fo));
	texpr = ExprTyCheckFunc(kctx, fo, stmt, expr, gma, reqty);
	if(kStmt_isERR(stmt)) return K_NULLEXPR;
//	FIXME: CHECK ALL VAR_ExprTyCheck
//	if(texpr->ty == TY_var && texpr != K_NULLEXPR) {
//		texpr = kExpr_p(stmt, expr, ErrTag, "typing error");
//	}
	return texpr;
}

static void Expr_putConstValue(KonohaContext *kctx, kExpr *expr, KonohaStack *sfp)
{
	if(expr->build == TEXPR_CONST) {
		KSETv(sfp[0].toObject, expr->objectConstValue);
		sfp[0].unboxValue = O_unbox(expr->objectConstValue);
	}else if(expr->build == TEXPR_NCONST) {
		sfp[0].unboxValue = expr->unboxConstValue;
	}else if(expr->build == TEXPR_NEW) {
		KSETv(sfp[0].toObject, KLIB new_kObject(kctx, CT_(expr->ty), 0));
	}else {
		assert(expr->build == TEXPR_NULL);
		KSETv(sfp[0].toObject, KLIB Knull(kctx, CT_(expr->ty)));
		sfp[0].unboxValue = 0;
	}
}

static kExpr* ExprCall_toConstValue(KonohaContext *kctx, kExpr *expr, kArray *cons, ktype_t rtype)
{
	size_t i, size = kArray_size(cons), psize = size - 2;
	kMethod *mtd = cons->methodItems[0];
	BEGIN_LOCAL(lsfp, K_CALLDELTA + psize);
	for(i = 1; i < size; i++) {
		Expr_putConstValue(kctx, cons->exprItems[i], lsfp + K_CALLDELTA + i - 1);
	}
	KCALL(lsfp, 0, mtd, psize, KLIB Knull(kctx, CT_(expr->ty)));
	END_LOCAL();
	if(TY_isUnbox(rtype) || rtype == TY_void) {
		return SUGARAPI kExpr_setUnboxConstValue(kctx, expr, rtype, lsfp[0].unboxValue);
	}
	return SUGARAPI kExpr_setConstValue(kctx, expr, rtype, lsfp[0].toObject);
}

static kbool_t CT_isa(KonohaContext *kctx, ktype_t cid1, ktype_t cid2)
{
	DBG_ASSERT(cid1 != cid2); // should be checked
	KonohaClass *ct = CT_(cid1), *t = CT_(cid2);
	return ct->isSubType(kctx, ct, t);
}

static kExpr *new_BoxingExpr(KonohaContext *kctx, kExpr *expr, ktype_t reqty)
{
	if(expr->build == TEXPR_NCONST) {
		kExprVar *Wexpr = (kExprVar*)expr;
		Wexpr->build = TEXPR_CONST;
		KINITv(Wexpr->objectConstValue, KLIB new_kObject(kctx, CT_(Wexpr->ty), Wexpr->unboxConstValue));
		Expr_setObjectConstValue(Wexpr, 1);
		Wexpr->ty = reqty;
		return expr;
	}
	else {
		kExprVar *texpr = GCSAFE_new(ExprVar, NULL);
		KINITv(texpr->single, expr);
		texpr->build = TEXPR_BOX;
		texpr->ty = reqty;
		return texpr;
	}
}

static kExpr *Expr_tyCheck(KonohaContext *kctx, kStmt *stmt, kExpr *expr, kGamma *gma, ktype_t reqty, int pol)
{
	kExpr *texpr = expr;
	if(kStmt_isERR(stmt)) texpr = K_NULLEXPR;
	if(expr->ty == TY_var && expr != K_NULLEXPR) {
		if(!IS_Expr(expr)) {
			expr = new_ConstValueExpr(kctx, O_cid(expr), UPCAST(expr));
		}
		texpr = ExprTyCheck(kctx, stmt, expr, gma, reqty);
	}
	if(kStmt_isERR(stmt)) texpr = K_NULLEXPR;
	if(texpr != K_NULLEXPR) {
		//DBG_P("type=%s, reqty=%s", TY_t(expr->ty), TY_t(reqty));
		if(texpr->ty == TY_void) {
			if(!FLAG_is(pol, TPOL_ALLOWVOID)) {
				texpr = kExpr_p(stmt, expr, ErrTag, "void is not acceptable");
			}
			return texpr;
		}
		if(reqty == TY_var || texpr->ty == reqty || FLAG_is(pol, TPOL_NOCHECK)) {
			return texpr;
		}
		if(CT_isa(kctx, texpr->ty, reqty)) {
			if(TY_isUnbox(texpr->ty) && !TY_isUnbox(reqty)) {
				return new_BoxingExpr(kctx, expr, reqty);
			}
			return texpr;
		}
		kMethod *mtd = kNameSpace_getCastMethodNULL(gma->genv->ns, texpr->ty, reqty);
		DBG_P("finding cast %s => %s: %p", TY_t(texpr->ty), TY_t(reqty), mtd);
		if(mtd != NULL && (kMethod_isCoercion(mtd) || FLAG_is(pol, TPOL_COERCION))) {
			return new_TypedMethodCall(kctx, stmt, reqty, mtd, gma, 1, texpr);
		}
		return kExpr_p(stmt, expr, ErrTag, "%s is requested, but %s is given", TY_t(reqty), TY_t(texpr->ty));
	}
	return texpr;
}

static kExpr* Expr_tyCheckAt(KonohaContext *kctx, kStmt *stmt, kExpr *exprP, size_t pos, kGamma *gma, ktype_t reqty, int pol)
{
	if(!Expr_isTerm(exprP) && pos < kArray_size(exprP->cons)) {
		kExpr *expr = exprP->cons->exprItems[pos];
		expr = Expr_tyCheck(kctx, stmt, expr, gma, reqty, pol);
		KSETv(exprP->cons->exprItems[pos], expr);
		return expr;
	}
	return K_NULLEXPR;
}

static kbool_t Stmt_tyCheckExpr(KonohaContext *kctx, kStmt *stmt, ksymbol_t nameid, kGamma *gma, ktype_t reqty, int pol)
{
	kExpr *expr = (kExpr*)kStmt_getObjectNULL(kctx, stmt, nameid);
	if(expr != NULL && IS_Expr(expr)) {
		kExpr *texpr = Expr_tyCheck(kctx, stmt, expr, gma, reqty, pol);
//		DBG_P("reqty=%s, texpr->ty=%s isnull=%d", TY_t(reqty), TY_t(texpr->ty), (texpr == K_NULLEXPR));
		if(texpr != K_NULLEXPR) {
			if(texpr != expr) {
				KLIB kObject_setObject(kctx, stmt, nameid, TY_Expr, texpr);
			}
			return 1;
		}
	}
	return 0;
}

/* ------------------------------------------------------------------------ */

static KMETHOD UndefinedStmtTyCheck(KonohaContext *kctx, KonohaStack *sfp)  // $expr
{
	VAR_StmtTyCheck(stmt, gma);
	const char *location = kGamma_isTOPLEVEL(gma) ? "at the top level" : "inside the function";
	kStmt_p(stmt, ErrTag, "%s%s is not available %s", T_statement(stmt->syn->keyword), location);
	RETURNb_(false);
}

static kbool_t Stmt_TyCheckFunc(KonohaContext *kctx, kFunc *fo, kStmt *stmt, kGamma *gma)
{
	BEGIN_LOCAL(lsfp, K_CALLDELTA + 3);
	KSETv(lsfp[K_CALLDELTA+0].o, (kObject*)fo->self);
	KSETv(lsfp[K_CALLDELTA+1].o, (kObject*)stmt);
	KSETv(lsfp[K_CALLDELTA+2].o, (kObject*)gma);
	KCALL(lsfp, 0, fo->mtd, 3, K_FALSE);
	END_LOCAL();
	return lsfp[0].bvalue;
}

static kbool_t Stmt_TyCheck(KonohaContext *kctx, SugarSyntax *syn, kStmt *stmt, kGamma *gma)
{
	kFunc *fo = kGamma_isTOPLEVEL(gma) ? syn->TopStmtTyCheck : syn->StmtTyCheck;
	kbool_t result;
	if(IS_Array(fo)) { // @Future
		int i;
		kArray *a = (kArray*)fo;
		for(i = kArray_size(a) - 1; i > 0; i--) {
			result = Stmt_TyCheckFunc(kctx, a->funcItems[i], stmt, gma);
			if(stmt->syn == NULL) return true;
			if(stmt->build != TSTMT_UNDEFINED) return result;
		}
		fo = a->funcItems[0];
	}
	DBG_ASSERT(IS_Func(fo));
	result = Stmt_TyCheckFunc(kctx, fo, stmt, gma);
	if(stmt->syn == NULL) return true; // this means done;
	if(result == false && stmt->build == TSTMT_UNDEFINED) {
		kStmt_p(stmt, ErrTag, "statement typecheck error: %s%s", T_statement(syn->keyword));
	}
	return result;
}

static kbool_t Block_tyCheckAll(KonohaContext *kctx, kBlock *bk, kGamma *gma)
{
	int i, result = true, lvarsize = gma->genv->localScope.varsize;
	for(i = 0; i < kArray_size(bk->stmtList); i++) {
		kStmt *stmt = (kStmt*)bk->stmtList->objectItems[i];
		SugarSyntax *syn = stmt->syn;
		dumpStmt(kctx, stmt);
		if(syn == NULL) continue; /* This means 'done' */
		if(kStmt_isERR(stmt) || !Stmt_TyCheck(kctx, syn, stmt, gma)) {
			DBG_ASSERT(kStmt_isERR(stmt));
			kGamma_setERROR(gma, 1);
			result = false;
			break;
		}
	}
	if(bk != K_NULLBLOCK) {
		SUGARAPI kExpr_setVariable(kctx, bk->esp, gma, TEXPR_LOCAL, TY_void, gma->genv->localScope.varsize);
	}
	if(lvarsize < gma->genv->localScope.varsize) {
		gma->genv->localScope.varsize = lvarsize;
	}
	return result;
}

/* ------------------------------------------------------------------------ */

static GammaAllocaData *Gamma_push(KonohaContext *kctx, kGamma *gma, GammaAllocaData *newone)
{
	GammaAllocaData *oldone = gma->genv;
	gma->genv = newone;
	return oldone;
}

static GammaAllocaData *Gamma_pop(KonohaContext *kctx, kGamma *gma, GammaAllocaData *oldone, GammaAllocaData *checksum)
{
	GammaAllocaData *newone = gma->genv;
	assert(checksum == newone);
	gma->genv = oldone;
	if(newone->localScope.allocsize > 0) {
		KFREE(newone->localScope.vars, newone->localScope.allocsize);
	}
	return newone;
}

#define GAMMA_PUSH(G,B) GammaAllocaData *oldbuf_ = Gamma_push(kctx, G, B)
#define GAMMA_POP(G,B)  Gamma_pop(kctx, G, oldbuf_, B)

// --------------------------------------------------------------------------

static kBlock* Method_newBlock(KonohaContext *kctx, kMethod *mtd, kString *source, kfileline_t uline)
{
	const char *script = S_text(source);
	if(IS_NULL(source) || script[0] == 0) {
		DBG_ASSERT(IS_Token(mtd->tcode));
		script = S_text(mtd->tcode->text);
		uline = mtd->tcode->uline;
	}
	kArray *tls = ctxsugar->preparedTokenList;
	size_t pos = kArray_size(tls);
	NameSpace_tokenize(kctx, KNULL(NameSpace), script, uline, tls); //FIXME: ks
	kBlock *bk = new_Block(kctx, KNULL(NameSpace), NULL, tls, pos, kArray_size(tls), ';');
	KLIB kArray_clear(kctx, tls, pos);
	return bk;
}

static void Gamma_initParam(KonohaContext *kctx, GammaAllocaData *genv, kParam *pa)
{
	int i, psize = (pa->psize + 1 < genv->localScope.capacity) ? pa->psize : genv->localScope.capacity - 1;
	for(i = 0; i < psize; i++) {
		genv->localScope.vars[i+1].fn = pa->p[i].fn;
		genv->localScope.vars[i+1].ty = pa->p[i].ty;
	}
	if(!kMethod_isStatic(genv->mtd)) {
		genv->localScope.vars[0].fn = FN_this;
		genv->localScope.vars[0].ty = genv->this_cid;
	}
	genv->localScope.varsize = psize+1;
}

static kbool_t Method_compile(KonohaContext *kctx, kMethod *mtd, kString *text, kfileline_t uline, kNameSpace *ns)
{
	INIT_GCSTACK();
	kGamma *gma = ctxsugar->gma;
	kBlock *bk = Method_newBlock(kctx, mtd, text, uline);
	GammaStackDecl lvars[32] = {};
	GammaAllocaData newgma = {
		.mtd = mtd,
		.ns = ns,
		.this_cid = (mtd)->cid,
		.localScope.vars = lvars, .localScope.capacity = 32, .localScope.varsize = 0, .localScope.allocsize = 0,
	};
	GAMMA_PUSH(gma, &newgma);
	Gamma_initParam(kctx, &newgma, kMethod_param(mtd));
	Block_tyCheckAll(kctx, bk, gma);
	KLIB kMethod_genCode(kctx, mtd, bk);
	GAMMA_POP(gma, &newgma);
	RESET_GCSTACK();
	return 1;
}

/* ------------------------------------------------------------------------ */
// eval

static void Gamma_initIt(KonohaContext *kctx, GammaAllocaData *genv, kParam *pa)
{
	KonohaContextRuntimeVar *base = kctx->stack;
	genv->localScope.varsize = 0;
	if(base->evalty != TY_void) {
		genv->localScope.vars[1].fn = FN_("it");
		genv->localScope.vars[1].ty = base->evalty;
		genv->localScope.varsize = 1;
	}
}

static kstatus_t Method_runEval(KonohaContext *kctx, kMethod *mtd, ktype_t rtype)
{
	BEGIN_LOCAL(lsfp, K_CALLDELTA);
	KonohaContextRuntimeVar *base = kctx->stack;
	kstatus_t result = K_CONTINUE;
	//DBG_P("TY=%s, running EVAL..", TY_t(rtype));
	if(base->evalty != TY_void) {
		KSETv(lsfp[K_CALLDELTA+1].o, base->stack[base->evalidx].o);
		lsfp[K_CALLDELTA+1].ivalue = base->stack[base->evalidx].ivalue;
	}
	KCALL(lsfp, 0, mtd, 0, KLIB Knull(kctx, CT_(rtype)));
	base->evalty = rtype;
	base->evalidx = (lsfp - kctx->stack->stack);
	END_LOCAL();
	return result;
}

static ktype_t Stmt_checkReturnType(KonohaContext *kctx, kStmt *stmt)
{
	if(stmt->syn->keyword == KW_ExprPattern) {
		kExpr *expr = (kExpr*)kStmt_getObjectNULL(kctx, stmt, KW_ExprPattern);
		DBG_ASSERT(expr != NULL);
		if(expr->ty != TY_void) {
			kStmt_setsyn(stmt, SYN_(kStmt_nameSpace(stmt), KW_return));
			kStmt_typed(stmt, RETURN);
			return expr->ty;
		}
	}
	return TY_void;
}

static ktype_t Gamma_evalMethod(KonohaContext *kctx, kGamma *gma, kBlock *bk, kMethod *mtd)
{
	kStmt *stmt = bk->stmtList->stmtItems[0];
	if(stmt->syn == NULL) {
		kctx->stack->evalty = TY_void;
		return K_CONTINUE;
	}
	if(stmt->syn->keyword == KW_ERR) return K_FAILED;
	ktype_t rtype = Stmt_checkReturnType(kctx, stmt);
	KLIB kMethod_genCode(kctx, mtd, bk);
	return Method_runEval(kctx, mtd, rtype);
}

static kstatus_t SingleBlock_eval(KonohaContext *kctx, kBlock *bk, kMethod *mtd, kNameSpace *ns)
{
	kstatus_t result;
	kGamma *gma = ctxsugar->gma;
	GammaStackDecl lvars[32] = {};
	GammaAllocaData newgma = {
		.flag = kGamma_TOPLEVEL,
		.mtd = mtd,
		.ns = ns,
		.this_cid     = TY_System,
		.localScope.vars = lvars, .localScope.capacity = 32, .localScope.varsize = 0, .localScope.allocsize = 0,
	};
	GAMMA_PUSH(gma, &newgma);
	Gamma_initIt(kctx, &newgma, kMethod_param(mtd));
	Block_tyCheckAll(kctx, bk, gma);
	if(kGamma_isERROR(gma)) {
		result = K_BREAK;
		kctx->stack->evalty = TY_void;
	}
	else {
		result = Gamma_evalMethod(kctx, gma, bk, mtd);
	}
	GAMMA_POP(gma, &newgma);
	return result;
}

static kstatus_t Block_eval(KonohaContext *kctx, kBlock *bk)
{
	INIT_GCSTACK();
	BEGIN_LOCAL(lsfp, 0);
	kBlock *bk1 = ctxsugar->singleBlock;
	kMethod *mtd = KLIB new_kMethod(kctx, kMethod_Static, 0, 0, NULL);
	PUSH_GCSTACK(mtd);
	KLIB kMethod_setParam(kctx, mtd, TY_Object, 0, NULL);
	int i, jmpresult;
	kstatus_t result = K_CONTINUE;
	KonohaContextRuntimeVar *base = kctx->stack;
	jmpbuf_i lbuf = {};
	if(base->evaljmpbuf == NULL) {
		base->evaljmpbuf = (jmpbuf_i*)KCALLOC(sizeof(jmpbuf_i), 1);
	}
	memcpy(&lbuf, base->evaljmpbuf, sizeof(jmpbuf_i));
	if((jmpresult = PLATAPI setjmp_i(*base->evaljmpbuf)) == 0) {
		for(i = 0; i < kArray_size(bk->stmtList); i++) {
			KSETv(bk1->stmtList->objectItems[0], bk->stmtList->objectItems[i]);
			KSETv(((kBlockVar*)bk1)->blockNameSpace, bk->blockNameSpace);
			KLIB kArray_clear(kctx, bk1->stmtList, 1);
			result = SingleBlock_eval(kctx, bk1, mtd, bk->blockNameSpace);
			if(result == K_FAILED) break;
		}
	}
	else {
		DBG_P("Catch eval exception jmpresult=%d", jmpresult);
		base->evalty = TY_void;
		result = K_FAILED;
	}
	memcpy(base->evaljmpbuf, &lbuf, sizeof(jmpbuf_i));
	END_LOCAL();
	RESET_GCSTACK();
	return result;
}

/* ------------------------------------------------------------------------ */

#ifdef __cplusplus
}
#endif

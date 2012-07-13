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

#ifndef FUNCTION_GLUE_H_
#define FUNCTION_GLUE_H_

#include<float.h>

typedef kFunc kFunc;
kFuncVar {
	KonohaObjectHeader h;
	kObject *self;
	kMethod *mtd;
};

// Int
static void Func_init(KonohaContext *kctx, kObject *o, void *conf)
{
	kFuncVar *fo = (kFuncVar*)o;
	KINITv(fo->self, K_NULL);
	KINITv(fo->mtd, conf != NULL ? KNULL(Method) : (kMethod*)conf);
}

static void Func_reftrace(KonohaContext *kctx, kObject *o)
{
	BEGIN_REFTRACE(4);
	kFunc *fo = (kFunc*)o;
	KREFTRACEv(fo->self);
	KREFTRACEv(fo->mtd);
	END_REFTRACE();
}

/* ------------------------------------------------------------------------ */
//## This Func.new(Object self, Method mtd);

static KMETHOD Func_new(KonohaContext *kctx, KonohaStack *sfp)
{
	kFunc *fo = sfp[0].fo;
	KSETv(fo->self, sfp[1].toObject);
	KSETv(fo->mtd, sfp[2].toMethod);
	RETURN_(fo);
}

/* ------------------------------------------------------------------------ */
//## @Hidden T0 Func.invoke();

static KMETHOD Func_invoke(KonohaContext *kctx, KonohaStack *sfp)
{
	kFunc* fo = sfp[0].fo;
	KSETv(sfp[0].toObject, fo->self);
	klr_setmtdNC(kctx, sfp[K_MTDIDX], fo->mtd);
	KCALL(kctx, sfp, fo->mtd, (-(K_CALLDELTA)));
}

#define _Public   kMethod_Public
#define _Const    kMethod_Const
#define _Im       kMethod_Immutable
#define _Coercion kMethod_Coercion
#define _F(F)   (intptr_t)(F)

static	kbool_t function_initPackage(KonohaContext *kctx, kNameSpace *ns, int argc, const char**args, kfileline_t pline)
{
	kmodfunction_t *base = (kmodfunction_t*)KCALLOC(sizeof(kmodfunction_t), 1);
	base->h.name     = "function";
	base->h.setup    = kmodfunction_setup;
	base->h.reftrace = kmodfunction_reftrace;
	base->h.free     = kmodfunction_free;
	Konoha_setModule(MOD_function, &base->h, pline);

	KDEFINE_TY defFloat = {
		STRUCTNAME(Float),
		.cflag = CFLAG_Int,
		.init = Float_init,
		.p     = Float_p,
	};
	base->cFloat = KLIB Konoha_defineClass(kctx, ns->packageId, PN_konoha, NULL, &defFloat, pline);
	int FN_x = FN_("x");
	KDEFINE_METHOD MethodData[] = {
//		_Public|_Const|_Im, _F(Float_opADD), TY_Float, TY_Float, MN_("opADD"), 1, TY_Float, FN_x,
//		_Public|_Const|_Im, _F(Float_opSUB), TY_Float, TY_Float, MN_("opSUB"), 1, TY_Float, FN_x,
//		_Public|_Const|_Im, _F(Float_opMUL), TY_Float, TY_Float, MN_("opMUL"), 1, TY_Float, FN_x,
//		_Public|_Im, _F(Float_opDIV), TY_Float, TY_Float, MN_("opDIV"), 1, TY_Float, FN_x,
//		_Public|_Const|_Im, _F(Float_opEQ),  TY_Boolean, TY_Float, MN_("opEQ"), 1, TY_Float, FN_x,
//		_Public|_Const|_Im, _F(Float_opNEQ), TY_Boolean, TY_Float, MN_("opNEQ"), 1, TY_Float, FN_x,
//		_Public|_Const|_Im, _F(Float_opLT),  TY_Boolean, TY_Float, MN_("opLT"), 1, TY_Float, FN_x,
//		_Public|_Const|_Im, _F(Float_opLTE), TY_Boolean, TY_Float, MN_("opLTE"), 1, TY_Float, FN_x,
//		_Public|_Const|_Im, _F(Float_opGT),  TY_Boolean, TY_Float, MN_("opGT"), 1, TY_Float, FN_x,
//		_Public|_Const|_Im, _F(Float_opGTE), TY_Boolean, TY_Float, MN_("opGTE"), 1, TY_Float, FN_x,
//		_Public|_Const|_Im|_Coercion, _F(Float_toInt), TY_Int, TY_Float, MN_to(TY_Int), 0,
//		_Public|_Const|_Im|_Coercion, _F(Int_toFloat), TY_Float, TY_Int, MN_to(TY_Float), 0,
//		_Public|_Const|_Im, _F(Float_toString), TY_String, TY_Float, MN_to(TY_String), 0,
//		_Public|_Const|_Im, _F(String_toFloat), TY_Float, TY_String, MN_to(TY_Float), 0,
//		DEND,
	};
	KLIB kNameSpace_loadMethodData(kctx, ns, MethodData);
	KDEFINE_FLOAT_CONST FloatData[] = {
		{"FLOAT_EPSILON", TY_Float, DBL_EPSILON},
		{}
	};
	KLIB kNameSpace_loadConstData(kctx, ns, KonohaConst_(FloatData), pline);
	return true;
}

static kbool_t function_setupPackage(KonohaContext *kctx, kNameSpace *ns, kfileline_t pline)
{
	return true;
}

//----------------------------------------------------------------------------

static KMETHOD ExprTyCheck_Float(KonohaContext *kctx, KonohaStack *sfp)
{
	USING_SUGAR;
	VAR_ExprTyCheck(stmt, expr, gma, reqty);
	kToken *tk = expr->tk;
	sfp[4].fvalue = strtod(S_text(tk->text), NULL);
	RETURN_(kExpr_setNConstValue(expr, TY_Float, sfp[4].ndata));
}

static kbool_t function_initNameSpace(KonohaContext *kctx,  kNameSpace *ns, kfileline_t pline)
{
	USING_SUGAR;
	KDEFINE_SYNTAX SYNTAX[] = {
		{ TOKEN("function"), .
			rule = "function $SYMBOL $params $block",
			TopStmtTyCheck_(FunctionDecl), ParseExpr_(function), },
		{ TOKEN("$param"), ExprTyCheck_(FuncStyleCall), },
		{ .keyword = KW_END, },
	};
	SUGAR NameSpace_defineSyntax(kctx, ns, SYNTAX);
	return true;
}

static kbool_t function_setupNameSpace(KonohaContext *kctx, kNameSpace *ns, kfileline_t pline)
{
	return true;
}

#endif /* FUNCTION_GLUE_H_ */
